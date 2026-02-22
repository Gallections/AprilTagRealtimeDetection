#pragma once

#include "common/Types.h"
#include "tracking/OneEuroFilter.h"

#include <unordered_map>
#include <vector>

namespace kt {

/// Computes joint angles from relative transforms between tag pairs.
/// Applies temporal filtering per tag.
class JointLinkage {
public:
    void configure(const std::vector<JointDefinition>& joints,
                   const OneEuroFilter::Settings& filterSettings);

    /// Process a new detection result: filter tag poses, compute joint angles.
    /// Returns joint results for any fully-observed joints.
    std::vector<JointResult> update(const DetectionResult& detection);

    /// Get the most recent filtered pose for a given tag (if available).
    const TagPose* filteredPose(int tagId) const;

    void reset();

private:
    struct TagFilter {
        OneEuroFilter fx, fy, fz;       // translation
        OneEuroFilter fqw, fqx, fqy, fqz; // quaternion components
        TagPose       lastFiltered;
        bool          hasData = false;
    };

    TagFilter& getOrCreateFilter(int tagId);

    TagPose applyFilter(TagFilter& tf, const TagPose& raw, double timestamp);

    std::vector<JointDefinition>           m_joints;
    OneEuroFilter::Settings                m_filterSettings;
    std::unordered_map<int, TagFilter>     m_tagFilters;
};

} // namespace kt
