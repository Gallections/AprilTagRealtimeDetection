#include "calibration/CalibrationStore.h"

#include <opencv2/core/persistence.hpp>
#include <spdlog/spdlog.h>

#include <filesystem>

namespace kt {

bool CalibrationStore::save(const std::string& directory, const CameraCalibration& calib) {
    namespace fs = std::filesystem;
    fs::create_directories(directory);

    std::string path = profilePath(directory, calib.cameraId);
    cv::FileStorage fs_out(path, cv::FileStorage::WRITE);
    if (!fs_out.isOpened()) {
        spdlog::error("CalibrationStore: cannot write to '{}'", path);
        return false;
    }

    fs_out << "camera_id"         << calib.cameraId;
    fs_out << "image_width"       << calib.imageWidth;
    fs_out << "image_height"      << calib.imageHeight;
    fs_out << "camera_matrix"     << calib.cameraMatrix;
    fs_out << "dist_coeffs"       << calib.distCoeffs;
    fs_out << "reprojection_error" << calib.reprojectionError;
    fs_out.release();

    cache(calib);
    spdlog::info("CalibrationStore: saved profile for camera {} to '{}'", calib.cameraId, path);
    return true;
}

bool CalibrationStore::load(const std::string& directory, int cameraId, CameraCalibration& out) {
    std::string path = profilePath(directory, cameraId);
    cv::FileStorage fs_in(path, cv::FileStorage::READ);
    if (!fs_in.isOpened()) {
        spdlog::warn("CalibrationStore: no profile found at '{}'", path);
        return false;
    }

    fs_in["camera_id"]          >> out.cameraId;
    fs_in["image_width"]        >> out.imageWidth;
    fs_in["image_height"]       >> out.imageHeight;
    fs_in["camera_matrix"]      >> out.cameraMatrix;
    fs_in["dist_coeffs"]        >> out.distCoeffs;
    fs_in["reprojection_error"] >> out.reprojectionError;
    fs_in.release();

    cache(out);
    spdlog::info("CalibrationStore: loaded profile for camera {} from '{}'", cameraId, path);
    return true;
}

void CalibrationStore::cache(const CameraCalibration& calib) {
    m_cache[calib.cameraId] = calib;
}

const CameraCalibration* CalibrationStore::get(int cameraId) const {
    auto it = m_cache.find(cameraId);
    return (it != m_cache.end()) ? &it->second : nullptr;
}

std::string CalibrationStore::profilePath(const std::string& directory, int cameraId) const {
    return directory + "/calibration_cam" + std::to_string(cameraId) + ".yml";
}

} // namespace kt
