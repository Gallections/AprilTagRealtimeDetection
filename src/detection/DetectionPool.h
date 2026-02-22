#pragma once

#include "common/Types.h"
#include "camera/FrameRingBuffer.h"
#include "detection/TagDetector.h"
#include "detection/PoseEstimator.h"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace kt {

/// Thread pool that pulls frames from ring buffers, runs detection + pose.
class DetectionPool {
public:
    using ResultCallback = std::function<void(DetectionResult)>;

    struct Settings {
        int threadCount = 0;  // 0 = auto
        TagDetector::Settings detectorSettings;
    };

    DetectionPool();
    ~DetectionPool();

    DetectionPool(const DetectionPool&) = delete;
    DetectionPool& operator=(const DetectionPool&) = delete;

    bool initialize(const Settings& settings);

    /// Register a ring buffer to pull frames from.
    void addSource(int cameraId, FrameRingBuffer* buffer, const CameraCalibration* calib);

    /// Set callback for detection results.
    void setResultCallback(ResultCallback cb);

    void start();
    void stop();

    bool isRunning() const { return m_running.load(std::memory_order_acquire); }

private:
    struct Source {
        int                      cameraId;
        FrameRingBuffer*         buffer;
        const CameraCalibration* calib;
    };

    void workerLoop(int workerId);

    Settings                        m_settings;
    std::vector<std::thread>        m_workers;
    std::atomic<bool>               m_running{false};

    std::mutex                      m_sourcesMutex;
    std::vector<Source>             m_sources;

    std::mutex                      m_callbackMutex;
    ResultCallback                  m_callback;

    PoseEstimator                   m_poseEstimator;
};

} // namespace kt
