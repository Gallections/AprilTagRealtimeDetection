#include "ui/Viewport.h"

#include <imgui.h>
#include <GLFW/glfw3.h>
#include <opencv2/imgproc.hpp>
#include <spdlog/spdlog.h>

#include <cstdio>

namespace kt {

Viewport::Viewport() = default;

Viewport::~Viewport() {
    for (auto& [id, view] : m_views) {
        if (view.texCreated)
            glDeleteTextures(1, &view.textureId);
    }
}

void Viewport::updateFrame(int cameraId, const cv::Mat& frame) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto& view = m_views[cameraId];
    frame.copyTo(view.pendingFrame);
    view.frameDirty = true;
}

void Viewport::updateDetections(int cameraId, const DetectionResult& result) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_views[cameraId].lastResult = result;
}

void Viewport::draw() {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& [camId, view] : m_views) {
        if (view.frameDirty) {
            uploadTexture(view);
            view.frameDirty = false;
        }

        char title[64];
        std::snprintf(title, sizeof(title), "Camera %d", camId);

        ImGui::Begin(title);

        if (view.texCreated && view.texWidth > 0) {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            float aspect = static_cast<float>(view.texWidth) / static_cast<float>(view.texHeight);
            float drawW  = avail.x;
            float drawH  = drawW / aspect;
            if (drawH > avail.y) {
                drawH = avail.y;
                drawW = drawH * aspect;
            }

            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImGui::Image(static_cast<ImTextureID>(view.textureId),
                         ImVec2(drawW, drawH));

            float scaleX = drawW / static_cast<float>(view.texWidth);
            float scaleY = drawH / static_cast<float>(view.texHeight);
            drawOverlays(view, pos.x, pos.y, scaleX, scaleY);

            // HUD: FPS + latency
            const auto& r = view.lastResult;
            ImGui::Text("Tags: %zu | Latency: %.1f ms", r.tags.size(), r.detectionTimeMs);
        } else {
            ImGui::TextDisabled("No feed");
        }

        ImGui::End();
    }
}

void Viewport::uploadTexture(CameraView& view) {
    if (view.pendingFrame.empty())
        return;

    cv::Mat rgb;
    if (view.pendingFrame.channels() == 3)
        cv::cvtColor(view.pendingFrame, rgb, cv::COLOR_BGR2RGB);
    else if (view.pendingFrame.channels() == 1)
        cv::cvtColor(view.pendingFrame, rgb, cv::COLOR_GRAY2RGB);
    else
        rgb = view.pendingFrame;

    if (!view.texCreated) {
        glGenTextures(1, &view.textureId);
        view.texCreated = true;
    }

    glBindTexture(GL_TEXTURE_2D, view.textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (rgb.cols != view.texWidth || rgb.rows != view.texHeight) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, rgb.cols, rgb.rows, 0,
                     GL_RGB, GL_UNSIGNED_BYTE, rgb.data);
        view.texWidth  = rgb.cols;
        view.texHeight = rgb.rows;
    } else {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, rgb.cols, rgb.rows,
                        GL_RGB, GL_UNSIGNED_BYTE, rgb.data);
    }
}

void Viewport::drawOverlays(const CameraView& view, float ox, float oy, float sx, float sy) {
    ImDrawList* dl = ImGui::GetWindowDrawList();

    for (const auto& tag : view.lastResult.tags) {
        // Draw quad boundary
        for (int i = 0; i < 4; ++i) {
            int j = (i + 1) % 4;
            ImVec2 a(ox + static_cast<float>(tag.corners[i].x) * sx,
                     oy + static_cast<float>(tag.corners[i].y) * sy);
            ImVec2 b(ox + static_cast<float>(tag.corners[j].x) * sx,
                     oy + static_cast<float>(tag.corners[j].y) * sy);
            dl->AddLine(a, b, IM_COL32(0, 255, 0, 255), 2.0f);
        }

        // Tag center
        double cx = 0, cy = 0;
        for (const auto& c : tag.corners) { cx += c.x; cy += c.y; }
        cx /= 4.0; cy /= 4.0;

        ImVec2 center(ox + static_cast<float>(cx) * sx,
                      oy + static_cast<float>(cy) * sy);

        // ID label
        char label[32];
        std::snprintf(label, sizeof(label), "ID:%d", tag.tagId);
        dl->AddText(ImVec2(center.x + 6, center.y - 10), IM_COL32(255, 255, 0, 255), label);

        // 3D axes if pose estimated (R,G,B = X,Y,Z)
        if (tag.translation != Eigen::Vector3d::Zero()) {
            float axisLen = 20.0f;  // pixels
            dl->AddLine(center, ImVec2(center.x + axisLen, center.y), IM_COL32(255, 0, 0, 255), 2.0f);   // X
            dl->AddLine(center, ImVec2(center.x, center.y - axisLen), IM_COL32(0, 255, 0, 255), 2.0f);   // Y
            dl->AddCircleFilled(center, 3.0f, IM_COL32(0, 100, 255, 255));                                // Z dot
        }
    }
}

} // namespace kt
