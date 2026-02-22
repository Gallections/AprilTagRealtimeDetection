#include "io/JsonWriter.h"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace kt {

bool JsonWriter::open(const std::string& path) {
    m_file.open(path, std::ios::out | std::ios::trunc);
    if (!m_file.is_open()) {
        spdlog::error("JsonWriter: cannot open '{}'", path);
        return false;
    }
    return true;
}

void JsonWriter::write(const TimeSeriesRecord& rec) {
    if (!m_file.is_open())
        return;

    nlohmann::json j;
    j["timestamp_us"]   = rec.timestampUs;
    j["camera_id"]      = rec.cameraId;
    j["tag_id"]         = rec.tagId;
    j["translation"]    = {rec.translation.x(), rec.translation.y(), rec.translation.z()};
    j["rotation"]       = {rec.rotation.w(), rec.rotation.x(), rec.rotation.y(), rec.rotation.z()};
    j["joint_name"]     = rec.jointName;
    j["joint_angle_deg"] = rec.jointAngleDeg;

    m_file << j.dump() << "\n";
}

void JsonWriter::flush() {
    if (m_file.is_open())
        m_file.flush();
}

void JsonWriter::close() {
    if (m_file.is_open())
        m_file.close();
}

} // namespace kt
