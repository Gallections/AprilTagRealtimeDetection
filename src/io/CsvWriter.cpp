#include "io/CsvWriter.h"

#include <spdlog/spdlog.h>

#include <iomanip>

namespace kt {

bool CsvWriter::open(const std::string& path) {
    m_file.open(path, std::ios::out | std::ios::trunc);
    if (!m_file.is_open()) {
        spdlog::error("CsvWriter: cannot open '{}'", path);
        return false;
    }
    m_headerWritten = false;
    return true;
}

void CsvWriter::write(const TimeSeriesRecord& rec) {
    if (!m_file.is_open())
        return;

    if (!m_headerWritten) {
        m_file << "timestamp_us,camera_id,tag_id,"
               << "tx,ty,tz,"
               << "qw,qx,qy,qz,"
               << "joint_name,joint_angle_deg\n";
        m_headerWritten = true;
    }

    m_file << rec.timestampUs << ","
           << rec.cameraId   << ","
           << rec.tagId      << ","
           << std::fixed << std::setprecision(6)
           << rec.translation.x() << ","
           << rec.translation.y() << ","
           << rec.translation.z() << ","
           << rec.rotation.w() << ","
           << rec.rotation.x() << ","
           << rec.rotation.y() << ","
           << rec.rotation.z() << ","
           << rec.jointName   << ","
           << std::setprecision(3)
           << rec.jointAngleDeg << "\n";
}

void CsvWriter::flush() {
    if (m_file.is_open())
        m_file.flush();
}

void CsvWriter::close() {
    if (m_file.is_open())
        m_file.close();
}

} // namespace kt
