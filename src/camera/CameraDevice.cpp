#include "camera/CameraDevice.h"
#include "camera/FrameRingBuffer.h"

#include <spdlog/spdlog.h>

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

    spdlog::info("Camera {} opened: {}x{} @ {:.0f} FPS (requested {}x{} @ {})",
                 settings.deviceIndex, actualW, actualH, actualFps,
                 settings.width, settings.height, settings.fps);
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
    m_frameSeq = 0;
    m_thread = std::thread(&CameraDevice::captureLoop, this, std::ref(buffer));

    spdlog::info("Camera {} capture started", m_settings.deviceIndex);
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
    while (m_capturing.load(std::memory_order_acquire)) {
        if (!m_capture.read(raw) || raw.empty())
            continue;

        Frame f;
        f.image          = raw.clone();
        f.timestamp      = Clock::now();
        f.cameraId       = m_settings.deviceIndex;
        f.sequenceNumber = m_frameSeq++;

        buffer.pushOverwrite(std::move(f));
    }
}

} // namespace kt
