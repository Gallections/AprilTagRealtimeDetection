#pragma once

#include <functional>
#include <string>

struct GLFWwindow;

namespace kt {

class Renderer {
public:
    struct Config {
        int         width  = 1280;
        int         height = 720;
        std::string title  = "KinematicsTracker";
        float       uiScale = 1.0f;
    };

    bool initialize(const Config& cfg);
    void shutdown();

    /// Returns true if the window should remain open.
    bool beginFrame();
    void endFrame();

    bool shouldClose() const;

    GLFWwindow* window() const { return m_window; }
    float       fps()    const { return m_fps; }

private:
    void applyStyle();

    GLFWwindow* m_window = nullptr;
    float       m_fps    = 0.0f;
    float       m_uiScale = 1.0f;
};

} // namespace kt
