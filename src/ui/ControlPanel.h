#pragma once

#include "common/Types.h"

#include <functional>
#include <vector>

namespace kt {

/// ImGui panel for session and camera controls.
class ControlPanel {
public:
    enum class Action {
        None,
        StartCapture,
        StopCapture,
        StartCalibration,
        AddCamera
    };

    struct State {
        bool   capturing      = false;
        int    activeCameras  = 0;
        float  fps            = 0.0f;
        int    newCameraIndex = 0;
        int    newCameraWidth = 640;
        int    newCameraHeight = 480;
        int    newCameraFps   = 120;
    };

    /// Draw the control panel. Returns the action requested by the user.
    Action draw(State& state);
};

} // namespace kt
