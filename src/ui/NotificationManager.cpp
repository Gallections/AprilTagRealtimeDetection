#include "ui/NotificationManager.h"

#include <imgui.h>

#include <algorithm>

namespace kt {

void NotificationManager::push(const std::string& message, Level level, float durationSec) {
    m_toasts.push_back({message, level, durationSec});
}

void NotificationManager::update(float deltaTime) {
    for (auto& t : m_toasts)
        t.remaining -= deltaTime;

    m_toasts.erase(
        std::remove_if(m_toasts.begin(), m_toasts.end(),
                       [](const Toast& t) { return t.remaining <= 0.0f; }),
        m_toasts.end());
}

void NotificationManager::draw() {
    if (m_toasts.empty())
        return;

    // Draw from top-right corner
    const float PADDING = 12.0f;
    ImVec2 vpSize = ImGui::GetMainViewport()->Size;
    float y = PADDING;

    for (const auto& t : m_toasts) {
        ImU32 col;
        switch (t.level) {
            case Level::Warning: col = IM_COL32(255, 200, 50, 220); break;
            case Level::Error:   col = IM_COL32(255, 60, 60, 220);  break;
            default:             col = IM_COL32(60, 180, 255, 220);  break;
        }

        ImVec2 textSize = ImGui::CalcTextSize(t.message.c_str());
        float boxW = textSize.x + 20.0f;
        float boxH = textSize.y + 12.0f;
        float x = vpSize.x - boxW - PADDING;

        ImDrawList* dl = ImGui::GetForegroundDrawList();
        dl->AddRectFilled(ImVec2(x, y), ImVec2(x + boxW, y + boxH), col, 4.0f);
        dl->AddText(ImVec2(x + 10, y + 6), IM_COL32(0, 0, 0, 255), t.message.c_str());

        y += boxH + 4.0f;
    }
}

} // namespace kt
