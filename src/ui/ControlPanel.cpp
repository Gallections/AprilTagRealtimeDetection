#include "ui/ControlPanel.h"

#include <imgui.h>

namespace kt {

ControlPanel::Action ControlPanel::draw(State& state) {
    Action action = Action::None;

    ImGui::Begin("Controls");

    // Status
    ImGui::Text("Status: %s", state.capturing ? "CAPTURING" : "IDLE");
    ImGui::Text("Cameras: %d | UI FPS: %.0f", state.activeCameras, state.fps);
    ImGui::Separator();

    // Session controls
    if (!state.capturing) {
        if (ImGui::Button("Start Capture", ImVec2(-1, 36)))
            action = Action::StartCapture;
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("Stop Capture", ImVec2(-1, 36)))
            action = Action::StopCapture;
        ImGui::PopStyleColor();
    }

    ImGui::Separator();

    // Add camera
    ImGui::Text("Add Camera");
    ImGui::InputInt("Device Index", &state.newCameraIndex);
    ImGui::InputInt("Width",  &state.newCameraWidth);
    ImGui::InputInt("Height", &state.newCameraHeight);
    ImGui::InputInt("FPS",    &state.newCameraFps);

    if (ImGui::Button("Add Camera", ImVec2(-1, 0)))
        action = Action::AddCamera;

    ImGui::Separator();

    // Calibration
    if (ImGui::Button("Calibrate Selected Camera", ImVec2(-1, 0)))
        action = Action::StartCalibration;

    ImGui::End();

    return action;
}

} // namespace kt
