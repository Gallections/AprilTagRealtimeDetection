#pragma once

#include "common/Types.h"

#include <vector>

namespace kt {

/// Computes 6-DOF pose from detected tag corners + camera intrinsics.
class PoseEstimator {
public:
    /// Estimate pose for each detected tag using solvePnP.
    /// Modifies tags in-place, filling transform/translation/rotation.
    void estimate(std::vector<TagPose>& tags, const CameraCalibration& calib);

private:
    /// Build the 3D object points for a square tag of the given size.
    static std::vector<cv::Point3d> tagObjectPoints(double tagSize);
};

} // namespace kt
