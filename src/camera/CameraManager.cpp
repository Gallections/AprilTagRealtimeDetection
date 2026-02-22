#include "camera/CameraManager.h"

#include <spdlog/spdlog.h>

namespace kt {

CameraManager::CameraManager(int ringBufferCapacity)
    : m_ringBufferCapacity(ringBufferCapacity) {}

CameraManager::~CameraManager() {
    closeAll();
}

bool CameraManager::addCamera(const CameraDevice::Settings& settings) {
    auto device = std::make_unique<CameraDevice>();
    if (!device->open(settings))
        return false;

    auto buffer = std::make_unique<FrameRingBuffer>(
        static_cast<size_t>(m_ringBufferCapacity));

    int id = settings.deviceIndex;
    m_cameras[id] = {std::move(device), std::move(buffer)};
    spdlog::info("CameraManager: added camera {}", id);
    return true;
}

void CameraManager::startAll() {
    for (auto& [id, entry] : m_cameras) {
        if (entry.device && entry.buffer)
            entry.device->startCapture(*entry.buffer);
    }
}

void CameraManager::stopAll() {
    for (auto& [id, entry] : m_cameras) {
        if (entry.device)
            entry.device->stopCapture();
    }
}

void CameraManager::closeAll() {
    stopAll();
    m_cameras.clear();
}

CameraDevice* CameraManager::camera(int deviceIndex) {
    auto it = m_cameras.find(deviceIndex);
    return (it != m_cameras.end()) ? it->second.device.get() : nullptr;
}

FrameRingBuffer* CameraManager::ringBuffer(int deviceIndex) {
    auto it = m_cameras.find(deviceIndex);
    return (it != m_cameras.end()) ? it->second.buffer.get() : nullptr;
}

std::vector<int> CameraManager::activeCameraIds() const {
    std::vector<int> ids;
    ids.reserve(m_cameras.size());
    for (const auto& [id, entry] : m_cameras)
        ids.push_back(id);
    return ids;
}

} // namespace kt
