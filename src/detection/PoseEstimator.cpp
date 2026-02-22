#include "detection/PoseEstimator.h"

#include <opencv2/calib3d.hpp>
#include <Eigen/Geometry>
#include <spdlog/spdlog.h>

namespace kt {

void PoseEstimator::estimate(std::vector<TagPose>& tags, const CameraCalibration& calib) {
    if (calib.cameraMatrix.empty()) {
        spdlog::warn("PoseEstimator: no calibration available, skipping pose estimation");
        return;
    }

    for (auto& tp : tags) {
        auto objPts = tagObjectPoints(tp.tagSize);

        std::vector<cv::Point2d> imgPts(tp.corners.begin(), tp.corners.end());

        cv::Mat rvec, tvec;
        bool ok = cv::solvePnP(objPts, imgPts, calib.cameraMatrix, calib.distCoeffs,
                                rvec, tvec, false, cv::SOLVEPNP_IPPE_SQUARE);
        if (!ok) {
            spdlog::debug("PoseEstimator: solvePnP failed for tag {}", tp.tagId);
            continue;
        }

        // Convert to rotation matrix
        cv::Mat rmat;
        cv::Rodrigues(rvec, rmat);

        // Fill Eigen types
        Eigen::Matrix3d R;
        for (int r = 0; r < 3; ++r)
            for (int c = 0; c < 3; ++c)
                R(r, c) = rmat.at<double>(r, c);

        Eigen::Vector3d t(tvec.at<double>(0), tvec.at<double>(1), tvec.at<double>(2));

        tp.translation = t;
        tp.rotation    = Eigen::Quaterniond(R);
        tp.transform   = Eigen::Isometry3d::Identity();
        tp.transform.linear()      = R;
        tp.transform.translation() = t;
    }
}

std::vector<cv::Point3d> PoseEstimator::tagObjectPoints(double tagSize) {
    double half = tagSize / 2.0;
    return {
        {-half,  half, 0.0},   // top-left
        { half,  half, 0.0},   // top-right
        { half, -half, 0.0},   // bottom-right
        {-half, -half, 0.0}    // bottom-left
    };
}

} // namespace kt
