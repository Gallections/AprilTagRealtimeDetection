#pragma once

#include "common/Types.h"

#include <string>
#include <vector>

namespace kt {

struct AppConfig {
    // Camera
    int defaultWidth          = 640;
    int defaultHeight         = 480;
    int defaultFps            = 120;
    int ringBufferCapacity    = 8;

    // Detection
    std::string tagFamily     = "tag36h11";
    float quadDecimate        = 2.0f;
    float quadSigma           = 0.0f;
    bool  refineEdges         = true;
    int   detectionThreads    = 0;  // 0 = auto

    // Tracking
    float filterMinCutoff     = 1.0f;
    float filterBeta          = 0.007f;
    float filterDCutoff       = 1.0f;

    // Joints
    std::vector<JointDefinition> joints;

    // Export
    std::string exportFormat    = "csv";
    std::string exportDirectory = "./data";
    int flushIntervalMs         = 100;
    int flushBatchSize          = 1000;

    // UI
    float uiScale = 1.0f;
};

class ConfigManager {
public:
    bool load(const std::string& path);
    bool save(const std::string& path) const;

    const AppConfig& config() const { return m_config; }
    AppConfig&       config()       { return m_config; }

    const std::string& loadedPath() const { return m_loadedPath; }

private:
    AppConfig   m_config;
    std::string m_loadedPath;
};

} // namespace kt
