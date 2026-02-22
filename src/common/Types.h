#pragma once

#include <opencv2/core.hpp>
#include <Eigen/Core>
#include <Eigen/Geometry>

#include <array>
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace kt {

// ── Clock ────────────────────────────────────────────────────
using Clock     = std::chrono::steady_clock;
using TimePoint = Clock::time_point;
using Duration  = Clock::duration;

inline uint64_t toMicroseconds(TimePoint tp) {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            tp.time_since_epoch())
            .count());
}

// ── Frame ────────────────────────────────────────────────────
struct Frame {
    cv::Mat   image;
    TimePoint timestamp;
    int       cameraId       = -1;
    uint64_t  sequenceNumber = 0;
};

// ── Camera calibration ───────────────────────────────────────
struct CameraCalibration {
    int    cameraId    = -1;
    cv::Mat cameraMatrix;          // 3×3 double
    cv::Mat distCoeffs;            // 1×5 or 1×8 double
    int    imageWidth  = 0;
    int    imageHeight = 0;
    double reprojectionError = 0.0;
};

// ── Tag detection & pose ─────────────────────────────────────
struct TagPose {
    int    tagId   = -1;
    double tagSize = 0.0;  // metres
    Eigen::Isometry3d  transform  = Eigen::Isometry3d::Identity();
    Eigen::Vector3d    translation = Eigen::Vector3d::Zero();
    Eigen::Quaterniond rotation    = Eigen::Quaterniond::Identity();
    std::array<cv::Point2d, 4> corners{};
    double decisionMargin = 0.0;
};

struct DetectionResult {
    int       cameraId      = -1;
    TimePoint timestamp;
    uint64_t  frameSequence = 0;
    std::vector<TagPose> tags;
    double    detectionTimeMs = 0.0;
};

// ── Joint definitions ────────────────────────────────────────
struct JointDefinition {
    std::string name;
    int    proximalTagId   = -1;
    int    distalTagId     = -1;
    double proximalTagSize = 0.06;  // metres
    double distalTagSize   = 0.06;
};

struct JointResult {
    std::string name;
    int proximalTagId = -1;
    int distalTagId   = -1;
    Eigen::Isometry3d  relativeTransform = Eigen::Isometry3d::Identity();
    double             angleDegrees      = 0.0;
    Eigen::Vector3d    axis              = Eigen::Vector3d::UnitZ();
};

// ── Time-series record (one row of exported data) ────────────
struct TimeSeriesRecord {
    uint64_t           timestampUs  = 0;
    int                cameraId     = -1;
    int                tagId        = -1;
    Eigen::Vector3d    translation  = Eigen::Vector3d::Zero();
    Eigen::Quaterniond rotation     = Eigen::Quaterniond::Identity();
    std::string        jointName;
    double             jointAngleDeg = 0.0;
};

} // namespace kt
