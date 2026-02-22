#pragma once

#include "common/Types.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// Forward-declare AprilTag C types
struct apriltag_detector;
struct apriltag_family;

namespace kt {

/// Thin C++ wrapper around the AprilTag 3 C-library detector.
class TagDetector {
public:
    struct Settings {
        std::string tagFamily  = "tag36h11";
        float       quadDecimate = 2.0f;
        float       quadSigma    = 0.0f;
        bool        refineEdges  = true;
        int         nThreads     = 1;
    };

    TagDetector();
    ~TagDetector();

    TagDetector(const TagDetector&) = delete;
    TagDetector& operator=(const TagDetector&) = delete;

    bool initialize(const Settings& settings);
    void destroy();

    /// Run detection on a grayscale image.
    /// Returns detected tag corners and IDs (no pose yet).
    std::vector<TagPose> detect(const cv::Mat& grayImage);

    /// Register known tag sizes (metres) per tag ID.
    /// Tags not in this map will use defaultTagSize.
    void setTagSize(int tagId, double sizeMetres);
    void setDefaultTagSize(double sizeMetres);

    double tagSizeFor(int tagId) const;

private:
    apriltag_detector* m_detector = nullptr;
    apriltag_family*   m_family   = nullptr;
    Settings           m_settings;
    std::string        m_familyName;

    std::unordered_map<int, double> m_tagSizes;
    double m_defaultTagSize = 0.06;
};

} // namespace kt
