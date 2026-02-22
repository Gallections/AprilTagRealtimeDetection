#include "tracking/JointLinkage.h"

#include <Eigen/Geometry>
#include <spdlog/spdlog.h>

#include <cmath>

namespace {
constexpr double kPi = 3.14159265358979323846;
}

namespace kt {

void JointLinkage::configure(const std::vector<JointDefinition>& joints,
                              const OneEuroFilter::Settings& filterSettings) {
    m_joints         = joints;
    m_filterSettings = filterSettings;
    m_tagFilters.clear();
    spdlog::info("JointLinkage: configured with {} joint(s)", joints.size());
}

std::vector<JointResult> JointLinkage::update(const DetectionResult& detection) {
    double timestamp = static_cast<double>(toMicroseconds(detection.timestamp)) * 1e-6;

    // Filter each detected tag
    for (const auto& raw : detection.tags) {
        auto& tf = getOrCreateFilter(raw.tagId);
        applyFilter(tf, raw, timestamp);
    }

    // Compute joint angles for fully-observed pairs
    std::vector<JointResult> results;

    for (const auto& jd : m_joints) {
        auto itP = m_tagFilters.find(jd.proximalTagId);
        auto itD = m_tagFilters.find(jd.distalTagId);

        if (itP == m_tagFilters.end() || !itP->second.hasData)
            continue;
        if (itD == m_tagFilters.end() || !itD->second.hasData)
            continue;

        const auto& proximal = itP->second.lastFiltered;
        const auto& distal   = itD->second.lastFiltered;

        // Relative transform: T_joint = T_proximal^-1 * T_distal
        Eigen::Isometry3d relativeT = proximal.transform.inverse() * distal.transform;

        // Extract angle from rotation component
        Eigen::AngleAxisd aa(relativeT.rotation());

        JointResult jr;
        jr.name              = jd.name;
        jr.proximalTagId     = jd.proximalTagId;
        jr.distalTagId       = jd.distalTagId;
        jr.relativeTransform = relativeT;
        jr.angleDegrees      = aa.angle() * 180.0 / kPi;
        jr.axis              = aa.axis();

        results.push_back(std::move(jr));
    }

    return results;
}

const TagPose* JointLinkage::filteredPose(int tagId) const {
    auto it = m_tagFilters.find(tagId);
    if (it == m_tagFilters.end() || !it->second.hasData)
        return nullptr;
    return &it->second.lastFiltered;
}

void JointLinkage::reset() {
    m_tagFilters.clear();
}

JointLinkage::TagFilter& JointLinkage::getOrCreateFilter(int tagId) {
    auto it = m_tagFilters.find(tagId);
    if (it != m_tagFilters.end())
        return it->second;

    TagFilter tf;
    tf.fx  = OneEuroFilter(m_filterSettings);
    tf.fy  = OneEuroFilter(m_filterSettings);
    tf.fz  = OneEuroFilter(m_filterSettings);
    tf.fqw = OneEuroFilter(m_filterSettings);
    tf.fqx = OneEuroFilter(m_filterSettings);
    tf.fqy = OneEuroFilter(m_filterSettings);
    tf.fqz = OneEuroFilter(m_filterSettings);
    m_tagFilters[tagId] = std::move(tf);
    return m_tagFilters[tagId];
}

TagPose JointLinkage::applyFilter(TagFilter& tf, const TagPose& raw, double timestamp) {
    TagPose filtered = raw;

    if (raw.translation != Eigen::Vector3d::Zero()) {
        filtered.translation.x() = tf.fx.filter(raw.translation.x(), timestamp);
        filtered.translation.y() = tf.fy.filter(raw.translation.y(), timestamp);
        filtered.translation.z() = tf.fz.filter(raw.translation.z(), timestamp);

        // Filter quaternion components (simple component-wise, then normalize)
        double qw = tf.fqw.filter(raw.rotation.w(), timestamp);
        double qx = tf.fqx.filter(raw.rotation.x(), timestamp);
        double qy = tf.fqy.filter(raw.rotation.y(), timestamp);
        double qz = tf.fqz.filter(raw.rotation.z(), timestamp);

        filtered.rotation = Eigen::Quaterniond(qw, qx, qy, qz).normalized();

        // Rebuild transform
        filtered.transform = Eigen::Isometry3d::Identity();
        filtered.transform.linear()      = filtered.rotation.toRotationMatrix();
        filtered.transform.translation() = filtered.translation;
    }

    tf.lastFiltered = filtered;
    tf.hasData = true;
    return filtered;
}

} // namespace kt
