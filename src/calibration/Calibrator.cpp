#include "calibration/Calibrator.h"

#include <opencv2/calib3d.hpp>
#include <opencv2/imgproc.hpp>
#include <spdlog/spdlog.h>

namespace kt {

Calibrator::Calibrator(const Settings& settings)
    : m_settings(settings) {}

bool Calibrator::addFrame(const cv::Mat& image) {
    cv::Mat gray;
    if (image.channels() == 3)
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    else
        gray = image;

    std::vector<cv::Point2f> corners;
    bool found = false;

    if (m_settings.pattern == PatternType::Checkerboard) {
        found = cv::findChessboardCorners(gray, m_settings.boardSize, corners,
            cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE | cv::CALIB_CB_FAST_CHECK);
        if (found) {
            cv::cornerSubPix(gray, corners, cv::Size(11, 11), cv::Size(-1, -1),
                cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::MAX_ITER, 30, 0.001));
        }
    } else {
        found = cv::findCirclesGrid(gray, m_settings.boardSize, corners);
    }

    if (!found)
        return false;

    m_imagePoints.push_back(corners);

    // Build corresponding 3D object points
    std::vector<cv::Point3f> objPts;
    for (int r = 0; r < m_settings.boardSize.height; ++r)
        for (int c = 0; c < m_settings.boardSize.width; ++c)
            objPts.emplace_back(c * m_settings.squareSize, r * m_settings.squareSize, 0.0f);
    m_objectPoints.push_back(objPts);

    spdlog::debug("Calibrator: added sample {}", m_imagePoints.size());
    return true;
}

bool Calibrator::calibrate(int imageWidth, int imageHeight) {
    if (static_cast<int>(m_imagePoints.size()) < m_settings.minSamples) {
        spdlog::warn("Calibrator: not enough samples ({}/{})",
                     m_imagePoints.size(), m_settings.minSamples);
        return false;
    }

    cv::Mat cameraMatrix, distCoeffs;
    std::vector<cv::Mat> rvecs, tvecs;

    double rms = cv::calibrateCamera(
        m_objectPoints, m_imagePoints, cv::Size(imageWidth, imageHeight),
        cameraMatrix, distCoeffs, rvecs, tvecs);

    m_result.cameraMatrix      = cameraMatrix;
    m_result.distCoeffs        = distCoeffs;
    m_result.imageWidth        = imageWidth;
    m_result.imageHeight       = imageHeight;
    m_result.reprojectionError = rms;
    m_calibrated = true;

    spdlog::info("Calibration complete: RMS reprojection error = {:.4f}", rms);
    return true;
}

void Calibrator::reset() {
    m_imagePoints.clear();
    m_objectPoints.clear();
    m_calibrated = false;
    m_result = {};
}

} // namespace kt
