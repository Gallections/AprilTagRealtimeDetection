#include "camera/CameraDevice.h"
#include "camera/FrameRingBuffer.h"

#include <spdlog/spdlog.h>

#include <chrono>

namespace kt {

CameraDevice::~CameraDevice() {
    stopCapture();
    close();
}

bool CameraDevice::open(const Settings& settings) {
    m_settings = settings;

    if (!m_capture.open(settings.deviceIndex, cv::CAP_ANY)) {
        spdlog::error("Failed to open camera {}", settings.deviceIndex);
        return false;
    }

    m_capture.set(cv::CAP_PROP_FRAME_WIDTH,  settings.width);
    m_capture.set(cv::CAP_PROP_FRAME_HEIGHT, settings.height);
    m_capture.set(cv::CAP_PROP_FPS,          settings.fps);

    int actualW = static_cast<int>(m_capture.get(cv::CAP_PROP_FRAME_WIDTH));
    int actualH = static_cast<int>(m_capture.get(cv::CAP_PROP_FRAME_HEIGHT));
    double actualFps = m_capture.get(cv::CAP_PROP_FPS);

    m_settings.width  = actualW;
    m_settings.height = actualH;

    spdlog::info("Camera {} opened: {}x{} @ {:.0f} FPS (requested {}x{} @ {})",
                 settings.deviceIndex, actualW, actualH, actualFps,
                 settings.width, settings.height, settings.fps);
    return true;
}

bool CameraDevice::openVideo(const std::string& path, int sourceId) {
    if (!m_capture.open(path)) {
        spdlog::error("Failed to open video file: {}", path);
        return false;
    }

    m_settings.videoPath   = path;
    m_settings.isVideoFile = true;
    m_settings.deviceIndex = (sourceId >= 0) ? sourceId : -1000;  // negative IDs for video sources
    m_settings.width  = static_cast<int>(m_capture.get(cv::CAP_PROP_FRAME_WIDTH));
    m_settings.height = static_cast<int>(m_capture.get(cv::CAP_PROP_FRAME_HEIGHT));
    m_settings.fps    = static_cast<int>(m_capture.get(cv::CAP_PROP_FPS));
    if (m_settings.fps <= 0) m_settings.fps = 30;

    int totalFrames = static_cast<int>(m_capture.get(cv::CAP_PROP_FRAME_COUNT));
    spdlog::info("Video opened: '{}' ({}x{} @ {} FPS, {} frames)",
                 path, m_settings.width, m_settings.height, m_settings.fps, totalFrames);
    return true;
}

void CameraDevice::close() {
    stopCapture();
    if (m_capture.isOpened())
        m_capture.release();
}

void CameraDevice::startCapture(FrameRingBuffer& buffer) {
    if (m_capturing.load())
        return;

    m_capturing.store(true, std::memory_order_release);
    m_finished.store(false, std::memory_order_release);
    m_frameSeq = 0;
    m_thread = std::thread(&CameraDevice::captureLoop, this, std::ref(buffer));

    spdlog::info("{} {} capture started",
                 m_settings.isVideoFile ? "Video" : "Camera",
                 m_settings.isVideoFile ? m_settings.videoPath : std::to_string(m_settings.deviceIndex));
}

void CameraDevice::stopCapture() {
    m_capturing.store(false, std::memory_order_release);
    if (m_thread.joinable())
        m_thread.join();
}

void CameraDevice::setCalibration(const CameraCalibration& calib) {
    m_calibration    = calib;
    m_hasCalibration = true;
}

const CameraCalibration* CameraDevice::calibration() const {
    return m_hasCalibration ? &m_calibration : nullptr;
}

void CameraDevice::captureLoop(FrameRingBuffer& buffer) {
    cv::Mat raw;

    // For video files, pace playback at the video's native FPS
    const auto frameDuration = m_settings.isVideoFile
        ? std::chrono::microseconds(static_cast<int64_t>(1000000.0 / m_settings.fps))
        : std::chrono::microseconds(0);

    while (m_capturing.load(std::memory_order_acquire)) {
        auto frameStart = Clock::now();

        if (!m_capture.read(raw) || raw.empty()) {
            if (m_settings.isVideoFile) {
                spdlog::info("Video playback finished: {}", m_settings.videoPath);
                m_finished.store(true, std::memory_order_release);
                m_capturing.store(false, std::memory_order_release);
            }
            continue;
        }

        Frame f;
        f.image          = raw.clone();
        f.timestamp      = Clock::now();
        f.cameraId       = m_settings.deviceIndex;
        f.sequenceNumber = m_frameSeq++;

        buffer.pushOverwrite(std::move(f));

        // Pace video playback to match original framerate
        if (m_settings.isVideoFile && frameDuration.count() > 0) {
            auto elapsed = Clock::now() - frameStart;
            auto sleepTime = frameDuration - std::chrono::duration_cast<std::chrono::microseconds>(elapsed);
            if (sleepTime.count() > 0)
                std::this_thread::sleep_for(sleepTime);
        }
    }
}

} // namespace kt
