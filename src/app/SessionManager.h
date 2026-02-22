#pragma once

#include "common/Types.h"
#include "camera/CameraManager.h"
#include "calibration/CalibrationStore.h"
#include "detection/DetectionPool.h"
#include "tracking/JointLinkage.h"
#include "io/TimeSeriesLogger.h"
#include "io/ConfigManager.h"
#include "ui/Viewport.h"
#include "ui/NotificationManager.h"

#include <atomic>
#include <mutex>

namespace kt {

/// Orchestrates the full capture session lifecycle:
/// cameras → detection pool → tracking → logger + UI.
class SessionManager {
public:
    SessionManager(CameraManager& cameras,
                   CalibrationStore& calibStore,
                   JointLinkage& tracking,
                   Viewport& viewport,
                   NotificationManager& notifications,
                   AppConfig& config);

    /// Start a capture session on all registered cameras.
    bool startCapture();

    /// Stop the session, flush data, tear down pipeline.
    void stopCapture();

    bool isCapturing() const { return m_capturing.load(std::memory_order_acquire); }

    uint64_t recordsWritten() const;

private:
    void onDetectionResult(DetectionResult result);

    CameraManager&       m_cameras;
    CalibrationStore&    m_calibStore;
    JointLinkage&        m_tracking;
    Viewport&            m_viewport;
    NotificationManager& m_notifications;
    AppConfig&           m_config;

    DetectionPool        m_detectionPool;
    TimeSeriesLogger     m_logger;

    std::atomic<bool>    m_capturing{false};
    std::mutex           m_resultMutex;

    int m_noTagFrameCount = 0;
    static constexpr int kNoTagWarningThreshold = 60;
};

} // namespace kt
