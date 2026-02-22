#pragma once

#include "common/Types.h"

#include <string>
#include <unordered_map>

namespace kt {

/// Persists and loads per-camera calibration profiles.
class CalibrationStore {
public:
    bool save(const std::string& directory, const CameraCalibration& calib);
    bool load(const std::string& directory, int cameraId, CameraCalibration& out);

    void cache(const CameraCalibration& calib);
    const CameraCalibration* get(int cameraId) const;

private:
    std::string profilePath(const std::string& directory, int cameraId) const;
    std::unordered_map<int, CameraCalibration> m_cache;
};

} // namespace kt
