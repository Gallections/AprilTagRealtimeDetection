#pragma once

#include "camera/CameraDevice.h"
#include "camera/FrameRingBuffer.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace kt {

/// Manages multiple cameras and their associated ring buffers.
class CameraManager {
public:
    explicit CameraManager(int ringBufferCapacity = 8);
    ~CameraManager();

    CameraManager(const CameraManager&) = delete;
    CameraManager& operator=(const CameraManager&) = delete;

    /// Open a camera and create its ring buffer.
    bool addCamera(const CameraDevice::Settings& settings);

    /// Open a video file as a source and create its ring buffer.
    bool addVideo(const std::string& videoPath, int sourceId = -1000);

    /// Start capture on all opened cameras.
    void startAll();

    /// Stop capture on all cameras.
    void stopAll();

    /// Close and remove all cameras.
    void closeAll();

    CameraDevice*    camera(int deviceIndex);
    FrameRingBuffer* ringBuffer(int deviceIndex);

    std::vector<int> activeCameraIds() const;
    size_t           cameraCount() const { return m_cameras.size(); }

private:
    struct CameraEntry {
        std::unique_ptr<CameraDevice>    device;
        std::unique_ptr<FrameRingBuffer> buffer;
    };

    int m_ringBufferCapacity;
    std::unordered_map<int, CameraEntry> m_cameras;
};

} // namespace kt
