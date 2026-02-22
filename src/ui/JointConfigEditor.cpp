#include "ui/JointConfigEditor.h"

#include <imgui.h>

#include <cstdio>
#include <cstring>

namespace kt {

bool JointConfigEditor::draw(std::vector<JointDefinition>& joints) {
    bool modified = false;

    ImGui::Begin("Joint Configuration");

    if (ImGui::Button("Add Joint")) {
        JointDefinition jd;
        jd.name = "joint_" + std::to_string(joints.size());
        joints.push_back(jd);
        modified = true;
    }

    ImGui::Separator();

    int removeIdx = -1;
    for (int i = 0; i < static_cast<int>(joints.size()); ++i) {
        ImGui::PushID(i);
        auto& jd = joints[static_cast<size_t>(i)];

        char nameBuf[128] = {};
        std::strncpy(nameBuf, jd.name.c_str(), sizeof(nameBuf) - 1);

        if (ImGui::TreeNode("##joint", "%s", jd.name.c_str())) {
            if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
                jd.name = nameBuf;
                modified = true;
            }
            if (ImGui::InputInt("Proximal Tag ID", &jd.proximalTagId)) modified = true;
            if (ImGui::InputInt("Distal Tag ID",   &jd.distalTagId))   modified = true;

            double proxSize = jd.proximalTagSize * 1000.0;  // display in mm
            double distSize = jd.distalTagSize   * 1000.0;
            if (ImGui::InputDouble("Proximal Size (mm)", &proxSize, 1.0, 5.0, "%.1f")) {
                jd.proximalTagSize = proxSize / 1000.0;
                modified = true;
            }
            if (ImGui::InputDouble("Distal Size (mm)", &distSize, 1.0, 5.0, "%.1f")) {
                jd.distalTagSize = distSize / 1000.0;
                modified = true;
            }

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.15f, 0.15f, 1.0f));
            if (ImGui::Button("Remove"))
                removeIdx = i;
            ImGui::PopStyleColor();

            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    if (removeIdx >= 0) {
        joints.erase(joints.begin() + removeIdx);
        modified = true;
    }

    ImGui::End();

    return modified;
}

} // namespace kt
