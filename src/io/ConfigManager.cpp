#include "io/ConfigManager.h"

#include <toml++/toml.hpp>
#include <spdlog/spdlog.h>

#include <fstream>

namespace kt {

bool ConfigManager::load(const std::string& path) {
    try {
        auto tbl = toml::parse_file(path);

        // Camera
        if (auto cam = tbl["camera"]) {
            m_config.defaultWidth        = cam["default_width"].value_or(m_config.defaultWidth);
            m_config.defaultHeight       = cam["default_height"].value_or(m_config.defaultHeight);
            m_config.defaultFps          = cam["default_fps"].value_or(m_config.defaultFps);
            m_config.ringBufferCapacity  = cam["ring_buffer_capacity"].value_or(m_config.ringBufferCapacity);
        }

        // Detection
        if (auto det = tbl["detection"]) {
            m_config.tagFamily        = det["tag_family"].value_or(m_config.tagFamily);
            m_config.quadDecimate     = det["quad_decimate"].value_or(m_config.quadDecimate);
            m_config.quadSigma        = det["quad_sigma"].value_or(m_config.quadSigma);
            m_config.refineEdges      = det["refine_edges"].value_or(m_config.refineEdges);
            m_config.detectionThreads = det["detection_threads"].value_or(m_config.detectionThreads);
        }

        // Tracking
        if (auto trk = tbl["tracking"]) {
            m_config.filterMinCutoff = trk["filter_min_cutoff"].value_or(m_config.filterMinCutoff);
            m_config.filterBeta      = trk["filter_beta"].value_or(m_config.filterBeta);
            m_config.filterDCutoff   = trk["filter_d_cutoff"].value_or(m_config.filterDCutoff);
        }

        // Export
        if (auto exp = tbl["export"]) {
            m_config.exportFormat    = exp["format"].value_or(m_config.exportFormat);
            m_config.exportDirectory = exp["directory"].value_or(m_config.exportDirectory);
            m_config.flushIntervalMs = exp["flush_interval_ms"].value_or(m_config.flushIntervalMs);
            m_config.flushBatchSize  = exp["flush_batch_size"].value_or(m_config.flushBatchSize);
        }

        // UI
        if (auto ui = tbl["ui"]) {
            m_config.uiScale = ui["scale"].value_or(m_config.uiScale);
        }

        // Joints (array of tables)
        m_config.joints.clear();
        if (auto joints = tbl["joints"]) {
            if (auto arr = joints.as_array()) {
                for (const auto& elem : *arr) {
                    if (auto jt = elem.as_table()) {
                        JointDefinition jd;
                        jd.name            = (*jt)["name"].value_or(std::string("unnamed"));
                        jd.proximalTagId   = (*jt)["proximal_tag_id"].value_or(-1);
                        jd.distalTagId     = (*jt)["distal_tag_id"].value_or(-1);
                        jd.proximalTagSize = (*jt)["proximal_tag_size"].value_or(0.06);
                        jd.distalTagSize   = (*jt)["distal_tag_size"].value_or(0.06);
                        m_config.joints.push_back(std::move(jd));
                    }
                }
            }
        }

        m_loadedPath = path;
        spdlog::info("Config loaded from: {}", path);
        return true;

    } catch (const toml::parse_error& err) {
        spdlog::error("Failed to parse config '{}': {}", path, err.what());
        return false;
    }
}

bool ConfigManager::save(const std::string& path) const {
    try {
        toml::table tbl;

        // Camera
        tbl.insert("camera", toml::table{
            {"default_width",        m_config.defaultWidth},
            {"default_height",       m_config.defaultHeight},
            {"default_fps",          m_config.defaultFps},
            {"ring_buffer_capacity", m_config.ringBufferCapacity}
        });

        // Detection
        tbl.insert("detection", toml::table{
            {"tag_family",        m_config.tagFamily},
            {"quad_decimate",     static_cast<double>(m_config.quadDecimate)},
            {"quad_sigma",        static_cast<double>(m_config.quadSigma)},
            {"refine_edges",      m_config.refineEdges},
            {"detection_threads", m_config.detectionThreads}
        });

        // Tracking
        tbl.insert("tracking", toml::table{
            {"filter_min_cutoff", static_cast<double>(m_config.filterMinCutoff)},
            {"filter_beta",       static_cast<double>(m_config.filterBeta)},
            {"filter_d_cutoff",   static_cast<double>(m_config.filterDCutoff)}
        });

        // Export
        tbl.insert("export", toml::table{
            {"format",           m_config.exportFormat},
            {"directory",        m_config.exportDirectory},
            {"flush_interval_ms", m_config.flushIntervalMs},
            {"flush_batch_size",  m_config.flushBatchSize}
        });

        // UI
        tbl.insert("ui", toml::table{
            {"scale", static_cast<double>(m_config.uiScale)}
        });

        // Joints
        toml::array jarr;
        for (const auto& jd : m_config.joints) {
            jarr.push_back(toml::table{
                {"name",              jd.name},
                {"proximal_tag_id",   jd.proximalTagId},
                {"distal_tag_id",     jd.distalTagId},
                {"proximal_tag_size", jd.proximalTagSize},
                {"distal_tag_size",   jd.distalTagSize}
            });
        }
        tbl.insert("joints", std::move(jarr));

        std::ofstream ofs(path);
        if (!ofs.is_open()) {
            spdlog::error("Cannot open '{}' for writing", path);
            return false;
        }
        ofs << tbl;
        spdlog::info("Config saved to: {}", path);
        return true;

    } catch (const std::exception& ex) {
        spdlog::error("Failed to save config: {}", ex.what());
        return false;
    }
}

} // namespace kt
