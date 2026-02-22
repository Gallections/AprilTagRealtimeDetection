#include "ui/Renderer.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

namespace kt {

static void glfwErrorCallback(int error, const char* description) {
    spdlog::error("GLFW error {}: {}", error, description);
}

bool Renderer::initialize(const Config& cfg) {
    m_uiScale = cfg.uiScale;

    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) {
        spdlog::critical("Failed to initialize GLFW");
        return false;
    }

    // GL 3.3 Core
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    m_window = glfwCreateWindow(cfg.width, cfg.height, cfg.title.c_str(), nullptr, nullptr);
    if (!m_window) {
        spdlog::critical("Failed to create GLFW window");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(0);  // No vsync — we need uncapped FPS for low-latency updates

    // ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.FontGlobalScale = m_uiScale;

    applyStyle();

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    spdlog::info("Renderer initialized ({}x{}, scale={:.1f})", cfg.width, cfg.height, m_uiScale);
    return true;
}

void Renderer::shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();

    spdlog::info("Renderer shut down");
}

bool Renderer::beginFrame() {
    if (glfwWindowShouldClose(m_window))
        return false;

    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    m_fps = ImGui::GetIO().Framerate;

    // Full-window dockspace
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

    return true;
}

void Renderer::endFrame() {
    ImGui::Render();

    int displayW, displayH;
    glfwGetFramebufferSize(m_window, &displayW, &displayH);
    glViewport(0, 0, displayW, displayH);
    glClearColor(0.06f, 0.06f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(m_window);
}

bool Renderer::shouldClose() const {
    return m_window == nullptr || glfwWindowShouldClose(m_window);
}

void Renderer::applyStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding   = 4.0f;
    style.FrameRounding    = 3.0f;
    style.GrabRounding     = 2.0f;
    style.ScrollbarRounding = 3.0f;
    style.WindowPadding    = ImVec2(8, 8);
    style.FramePadding     = ImVec2(6, 4);

    // High-contrast dark theme for lab/gym visibility
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg]       = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    colors[ImGuiCol_TitleBg]        = ImVec4(0.10f, 0.10f, 0.14f, 1.00f);
    colors[ImGuiCol_TitleBgActive]  = ImVec4(0.14f, 0.14f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBg]        = ImVec4(0.14f, 0.14f, 0.18f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.28f, 1.00f);
    colors[ImGuiCol_Button]         = ImVec4(0.20f, 0.40f, 0.70f, 1.00f);
    colors[ImGuiCol_ButtonHovered]  = ImVec4(0.28f, 0.50f, 0.82f, 1.00f);
    colors[ImGuiCol_ButtonActive]   = ImVec4(0.16f, 0.34f, 0.60f, 1.00f);
    colors[ImGuiCol_Header]         = ImVec4(0.20f, 0.40f, 0.70f, 0.50f);
    colors[ImGuiCol_HeaderHovered]  = ImVec4(0.26f, 0.48f, 0.78f, 0.60f);
    colors[ImGuiCol_Tab]            = ImVec4(0.14f, 0.14f, 0.20f, 1.00f);
    colors[ImGuiCol_TabActive]      = ImVec4(0.20f, 0.40f, 0.70f, 1.00f);
    colors[ImGuiCol_TabHovered]     = ImVec4(0.28f, 0.50f, 0.82f, 1.00f);
}

} // namespace kt
