#pragma once

#include "common/Types.h"

#include <opencv2/core.hpp>

#include <vector>

namespace kt {

/// Camera intrinsic calibration via Zhang's method.
class Calibrator {
public:
    enum class PatternType { Checkerboard, CircleGrid };

    struct Settings {
        PatternType pattern    = PatternType::Checkerboard;
        cv::Size    boardSize  = {9, 6};    // inner corners
        float       squareSize = 0.025f;    // metres
        int         minSamples = 15;
    };

    explicit Calibrator(const Settings& settings = {});

    /// Feed a calibration frame. Returns true if the pattern was found.
    bool addFrame(const cv::Mat& image);

    /// Run calibration. Returns true on success.
    bool calibrate(int imageWidth, int imageHeight);

    /// Retrieve the result (valid only after calibrate() returns true).
    const CameraCalibration& result() const { return m_result; }

    int  sampleCount() const { return static_cast<int>(m_imagePoints.size()); }
    bool isCalibrated() const { return m_calibrated; }

    void reset();

private:
    Settings m_settings;
    CameraCalibration m_result;
    bool m_calibrated = false;

    std::vector<std::vector<cv::Point2f>> m_imagePoints;
    std::vector<std::vector<cv::Point3f>> m_objectPoints;
};

} // namespace kt
