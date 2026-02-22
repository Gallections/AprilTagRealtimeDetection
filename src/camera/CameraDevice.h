#pragma once

#include "common/Types.h"

#include <opencv2/videoio.hpp>

#include <atomic>
#include <string>
#include <thread>

namespace kt {

class FrameRingBuffer;

class CameraDevice {
public:
    struct Settings {
        int deviceIndex = 0;
        int width       = 640;
        int height      = 480;
        int fps         = 120;
    };

    CameraDevice() = default;
    ~CameraDevice();

    CameraDevice(const CameraDevice&) = delete;
    CameraDevice& operator=(const CameraDevice&) = delete;

    bool open(const Settings& settings);
    void close();

    /// Start the capture thread writing to the given ring buffer.
    void startCapture(FrameRingBuffer& buffer);
    void stopCapture();

    bool isOpen()      const { return m_capture.isOpened(); }
    bool isCapturing() const { return m_capturing.load(std::memory_order_acquire); }
    int  cameraId()    const { return m_settings.deviceIndex; }

    const Settings& settings() const { return m_settings; }

    /// Apply calibration for undistortion (stored, not applied per-frame yet).
    void setCalibration(const CameraCalibration& calib);
    const CameraCalibration* calibration() const;

private:
    void captureLoop(FrameRingBuffer& buffer);

    Settings          m_settings;
    cv::VideoCapture  m_capture;
    std::thread       m_thread;
    std::atomic<bool> m_capturing{false};
    uint64_t          m_frameSeq = 0;

    CameraCalibration        m_calibration;
    bool                     m_hasCalibration = false;
};

} // namespace kt
