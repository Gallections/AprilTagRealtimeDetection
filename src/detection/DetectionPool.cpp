#include "detection/DetectionPool.h"

#include <opencv2/imgproc.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>

namespace kt {

DetectionPool::DetectionPool() = default;

DetectionPool::~DetectionPool() {
    stop();
}

bool DetectionPool::initialize(const Settings& settings) {
    m_settings = settings;
    if (m_settings.threadCount <= 0) {
        int hw = static_cast<int>(std::thread::hardware_concurrency());
        m_settings.threadCount = std::max(1, hw - 2);
    }
    spdlog::info("DetectionPool: {} worker threads configured", m_settings.threadCount);
    return true;
}

void DetectionPool::addSource(int cameraId, FrameRingBuffer* buffer, const CameraCalibration* calib) {
    std::lock_guard<std::mutex> lock(m_sourcesMutex);
    m_sources.push_back({cameraId, buffer, calib});
}

void DetectionPool::setResultCallback(ResultCallback cb) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_callback = std::move(cb);
}

void DetectionPool::start() {
    if (m_running.load())
        return;

    m_running.store(true, std::memory_order_release);
    m_workers.reserve(static_cast<size_t>(m_settings.threadCount));

    for (int i = 0; i < m_settings.threadCount; ++i) {
        m_workers.emplace_back(&DetectionPool::workerLoop, this, i);
    }
    spdlog::info("DetectionPool: started {} workers", m_settings.threadCount);
}

void DetectionPool::stop() {
    m_running.store(false, std::memory_order_release);
    for (auto& w : m_workers) {
        if (w.joinable())
            w.join();
    }
    m_workers.clear();
}

void DetectionPool::workerLoop(int workerId) {
    // Each worker gets its own detector instance (not thread-safe internally)
    TagDetector detector;
    auto detSettings = m_settings.detectorSettings;
    detSettings.nThreads = 1;  // single-threaded within each worker
    if (!detector.initialize(detSettings)) {
        spdlog::error("DetectionPool worker {}: failed to init detector", workerId);
        return;
    }

    while (m_running.load(std::memory_order_acquire)) {
        bool didWork = false;

        std::vector<Source> sources;
        {
            std::lock_guard<std::mutex> lock(m_sourcesMutex);
            sources = m_sources;
        }

        for (auto& src : sources) {
            auto frameOpt = src.buffer->tryPop();
            if (!frameOpt.has_value())
                continue;

            didWork = true;
            Frame& frame = *frameOpt;

            auto t0 = Clock::now();

            // Convert to grayscale
            cv::Mat gray;
            if (frame.image.channels() == 3)
                cv::cvtColor(frame.image, gray, cv::COLOR_BGR2GRAY);
            else
                gray = frame.image;

            // Detect tags
            auto tags = detector.detect(gray);

            // Estimate pose if calibration available
            if (src.calib && !src.calib->cameraMatrix.empty()) {
                m_poseEstimator.estimate(tags, *src.calib);
            }

            auto t1 = Clock::now();
            double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

            DetectionResult result;
            result.cameraId        = src.cameraId;
            result.timestamp       = frame.timestamp;
            result.frameSequence   = frame.sequenceNumber;
            result.tags            = std::move(tags);
            result.detectionTimeMs = ms;

            {
                std::lock_guard<std::mutex> lock(m_callbackMutex);
                if (m_callback)
                    m_callback(std::move(result));
            }
        }

        if (!didWork) {
            // Brief sleep to avoid busy-spinning when no frames available
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }
}

} // namespace kt
