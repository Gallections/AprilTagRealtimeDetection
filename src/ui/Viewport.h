#pragma once

#include "common/Types.h"

#include <opencv2/core.hpp>

#include <mutex>
#include <unordered_map>

namespace kt {

/// Renders camera feeds as OpenGL textures with tag overlays.
class Viewport {
public:
    Viewport();
    ~Viewport();

    /// Upload a new frame for a camera (thread-safe).
    void updateFrame(int cameraId, const cv::Mat& frame);

    /// Update detection results for overlay drawing (thread-safe).
    void updateDetections(int cameraId, const DetectionResult& result);

    /// Draw all viewports as ImGui windows.
    void draw();

private:
    struct CameraView {
        unsigned int textureId   = 0;
        int          texWidth    = 0;
        int          texHeight   = 0;
        bool         texCreated  = false;
        cv::Mat      pendingFrame;
        bool         frameDirty  = false;
        DetectionResult lastResult;
    };

    void uploadTexture(CameraView& view);
    void drawOverlays(const CameraView& view, float offsetX, float offsetY, float scaleX, float scaleY);

    std::mutex                                m_mutex;
    std::unordered_map<int, CameraView>       m_views;
};

} // namespace kt
