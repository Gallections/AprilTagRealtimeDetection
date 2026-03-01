#include "app/SessionManager.h"

#include <opencv2/core.hpp>
#include <spdlog/spdlog.h>

#include <cmath>

namespace kt {

namespace {

/// Estimate camera intrinsics from resolution assuming ~60° horizontal FOV.
/// This gives approximate but usable pose data without manual calibration.
CameraCalibration estimateIntrinsics(int cameraId, int width, int height) {
    CameraCalibration calib;
    calib.cameraId    = cameraId;
    calib.imageWidth  = width;
    calib.imageHeight = height;

    // Assume ~60° horizontal FOV (typical for webcams)
    constexpr double hfovDeg = 60.0;
    constexpr double kPi = 3.14159265358979323846;
    double fx = width / (2.0 * std::tan(hfovDeg * kPi / 360.0));
    double fy = fx;  // square pixels
    double cx = width  / 2.0;
    double cy = height / 2.0;

    calib.cameraMatrix = (cv::Mat_<double>(3, 3) <<
        fx, 0,  cx,
        0,  fy, cy,
        0,  0,  1);
    calib.distCoeffs = cv::Mat::zeros(1, 5, CV_64F);
    calib.reprojectionError = -1.0;  // mark as estimated

    spdlog::info("Auto-estimated intrinsics for camera {} ({}x{}, fx={:.1f})",
                 cameraId, width, height, fx);
    return calib;
}

} // anonymous namespace

SessionManager::SessionManager(CameraManager& cameras,
                               CalibrationStore& calibStore,
                               JointLinkage& tracking,
                               Viewport& viewport,
                               NotificationManager& notifications,
                               AppConfig& config)
    : m_cameras(cameras)
    , m_calibStore(calibStore)
    , m_tracking(tracking)
    , m_viewport(viewport)
    , m_notifications(notifications)
    , m_config(config) {}

bool SessionManager::startCapture() {
    if (m_capturing.load())
        return false;

    if (m_cameras.cameraCount() == 0) {
        m_notifications.push("No cameras registered", NotificationManager::Level::Error);
        return false;
    }

    // Configure detection pool
    DetectionPool::Settings poolSettings;
    poolSettings.threadCount = m_config.detectionThreads;
    poolSettings.detectorSettings.tagFamily    = m_config.tagFamily;
    poolSettings.detectorSettings.quadDecimate = m_config.quadDecimate;
    poolSettings.detectorSettings.quadSigma    = m_config.quadSigma;
    poolSettings.detectorSettings.refineEdges  = m_config.refineEdges;

    if (!m_detectionPool.initialize(poolSettings)) {
        m_notifications.push("Failed to init detection pool", NotificationManager::Level::Error);
        return false;
    }

    // Register camera ring buffers as detection sources
    // Auto-estimate intrinsics for cameras without calibration
    for (int camId : m_cameras.activeCameraIds()) {
        auto* buffer = m_cameras.ringBuffer(camId);
        const CameraCalibration* calib = m_calibStore.get(camId);

        if (!calib) {
            // No saved calibration — estimate from resolution
            auto* cam = m_cameras.camera(camId);
            if (cam) {
                auto estimated = estimateIntrinsics(
                    camId, cam->settings().width, cam->settings().height);
                m_calibStore.cache(estimated);
                calib = m_calibStore.get(camId);
                m_notifications.push(
                    "Camera " + std::to_string(camId) + ": using estimated intrinsics (calibrate for accuracy)",
                    NotificationManager::Level::Warning, 5.0f);
            }
        }

        if (buffer)
            m_detectionPool.addSource(camId, buffer, calib);
    }

    m_detectionPool.setResultCallback(
        [this](DetectionResult result) { onDetectionResult(std::move(result)); });

    // Configure tracking
    OneEuroFilter::Settings filterSettings;
    filterSettings.minCutoff = static_cast<double>(m_config.filterMinCutoff);
    filterSettings.beta      = static_cast<double>(m_config.filterBeta);
    filterSettings.dCutoff   = static_cast<double>(m_config.filterDCutoff);
    m_tracking.configure(m_config.joints, filterSettings);

    // Start logger
    TimeSeriesLogger::Settings logSettings;
    logSettings.format          = m_config.exportFormat;
    logSettings.directory       = m_config.exportDirectory;
    logSettings.flushIntervalMs = m_config.flushIntervalMs;
    logSettings.flushBatchSize  = m_config.flushBatchSize;
    m_logger.start(logSettings);

    // Start pipeline: cameras first, then detection pool
    m_cameras.startAll();
    m_detectionPool.start();

    m_noTagFrameCount = 0;
    m_capturing.store(true, std::memory_order_release);

    m_notifications.push("Capture started", NotificationManager::Level::Info);
    spdlog::info("SessionManager: capture started");
    return true;
}

void SessionManager::stopCapture() {
    if (!m_capturing.load())
        return;

    m_capturing.store(false, std::memory_order_release);

    m_detectionPool.stop();
    m_cameras.stopAll();
    m_logger.stop();
    m_tracking.reset();

    m_notifications.push("Capture stopped", NotificationManager::Level::Info);
    spdlog::info("SessionManager: capture stopped ({} records written)", m_logger.recordsWritten());
}

uint64_t SessionManager::recordsWritten() const {
    return m_logger.recordsWritten();
}

void SessionManager::onDetectionResult(DetectionResult result) {
    std::lock_guard<std::mutex> lock(m_resultMutex);

    // Update viewport with latest frame data
    // (The viewport also needs the raw frame for display, but we
    //  push detection overlays here; frame upload happens from camera feed.)
    m_viewport.updateDetections(result.cameraId, result);

    // "No Tags Detected" warning
    if (result.tags.empty()) {
        ++m_noTagFrameCount;
        if (m_noTagFrameCount == kNoTagWarningThreshold) {
            m_notifications.push("No Tags Detected", NotificationManager::Level::Warning, 5.0f);
        }
    } else {
        m_noTagFrameCount = 0;
    }

    // Run tracking (filter + joint solve)
    auto jointResults = m_tracking.update(result);

    // Log tag poses
    for (const auto& tag : result.tags) {
        TimeSeriesRecord rec;
        rec.timestampUs  = toMicroseconds(result.timestamp);
        rec.cameraId     = result.cameraId;
        rec.tagId        = tag.tagId;
        rec.translation  = tag.translation;
        rec.rotation     = tag.rotation;
        m_logger.log(std::move(rec));
    }

    // Log joint results
    for (const auto& jr : jointResults) {
        TimeSeriesRecord rec;
        rec.timestampUs   = toMicroseconds(result.timestamp);
        rec.cameraId      = result.cameraId;
        rec.tagId         = -1;
        rec.jointName     = jr.name;
        rec.jointAngleDeg = jr.angleDegrees;
        m_logger.log(std::move(rec));
    }
}

} // namespace kt
