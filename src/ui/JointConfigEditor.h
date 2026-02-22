#pragma once

#include "common/Types.h"

#include <vector>

namespace kt {

/// ImGui panel for defining and editing joint linkage configurations.
class JointConfigEditor {
public:
    /// Draw the editor. Returns true if the joint list was modified.
    bool draw(std::vector<JointDefinition>& joints);
};

} // namespace kt
