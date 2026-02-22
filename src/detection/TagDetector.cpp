#include "detection/TagDetector.h"

#include <apriltag/apriltag.h>
#include <apriltag/tag36h11.h>
#include <apriltag/tagStandard41h12.h>
#include <apriltag/tagCircle21h7.h>

#include <opencv2/imgproc.hpp>
#include <spdlog/spdlog.h>

namespace kt {

TagDetector::TagDetector() = default;

TagDetector::~TagDetector() {
    destroy();
}

bool TagDetector::initialize(const Settings& settings) {
    destroy();
    m_settings = settings;

    // Create tag family
    if (settings.tagFamily == "tag36h11") {
        m_family = tag36h11_create();
    } else if (settings.tagFamily == "tagStandard41h12") {
        m_family = tagStandard41h12_create();
    } else if (settings.tagFamily == "tagCircle21h7") {
        m_family = tagCircle21h7_create();
    } else {
        spdlog::error("TagDetector: unknown tag family '{}'", settings.tagFamily);
        return false;
    }
    m_familyName = settings.tagFamily;

    // Create detector
    m_detector = apriltag_detector_create();
    apriltag_detector_add_family(m_detector, m_family);

    m_detector->quad_decimate = settings.quadDecimate;
    m_detector->quad_sigma    = settings.quadSigma;
    m_detector->refine_edges  = settings.refineEdges ? 1 : 0;
    m_detector->nthreads      = settings.nThreads;

    spdlog::info("TagDetector initialized: family={}, decimate={:.1f}, threads={}",
                 m_familyName, settings.quadDecimate, settings.nThreads);
    return true;
}

void TagDetector::destroy() {
    if (m_detector) {
        apriltag_detector_destroy(m_detector);
        m_detector = nullptr;
    }
    if (m_family) {
        if (m_familyName == "tag36h11")
            tag36h11_destroy(m_family);
        else if (m_familyName == "tagStandard41h12")
            tagStandard41h12_destroy(m_family);
        else if (m_familyName == "tagCircle21h7")
            tagCircle21h7_destroy(m_family);
        m_family = nullptr;
    }
}

std::vector<TagPose> TagDetector::detect(const cv::Mat& grayImage) {
    std::vector<TagPose> results;

    if (!m_detector || grayImage.empty())
        return results;

    // Wrap cv::Mat as AprilTag image_u8
    image_u8_t img = {
        grayImage.cols,
        grayImage.rows,
        static_cast<int32_t>(grayImage.step[0]),
        grayImage.data
    };

    zarray_t* detections = apriltag_detector_detect(m_detector, &img);

    int n = zarray_size(detections);
    results.reserve(static_cast<size_t>(n));

    for (int i = 0; i < n; ++i) {
        apriltag_detection_t* det = nullptr;
        zarray_get(detections, i, &det);

        TagPose tp;
        tp.tagId          = det->id;
        tp.tagSize        = tagSizeFor(det->id);
        tp.decisionMargin = det->decision_margin;

        for (int c = 0; c < 4; ++c) {
            tp.corners[static_cast<size_t>(c)] = cv::Point2d(det->p[c][0], det->p[c][1]);
        }

        results.push_back(std::move(tp));
    }

    apriltag_detections_destroy(detections);
    return results;
}

void TagDetector::setTagSize(int tagId, double sizeMetres) {
    m_tagSizes[tagId] = sizeMetres;
}

void TagDetector::setDefaultTagSize(double sizeMetres) {
    m_defaultTagSize = sizeMetres;
}

double TagDetector::tagSizeFor(int tagId) const {
    auto it = m_tagSizes.find(tagId);
    return (it != m_tagSizes.end()) ? it->second : m_defaultTagSize;
}

} // namespace kt
