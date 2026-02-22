#include "app/Application.h"

#include <spdlog/spdlog.h>
#include <imgui.h>

namespace kt {

Application::Application() = default;
Application::~Application() { shutdown(); }

bool Application::initialize(const std::string& configPath) {
    // Load config
    if (!m_configManager.load(configPath)) {
        spdlog::warn("Using default configuration (failed to load '{}')", configPath);
    }

    auto& cfg = m_configManager.config();

    // Initialize renderer
    Renderer::Config rCfg;
    rCfg.width   = 1280;
    rCfg.height  = 720;
    rCfg.title   = "KinematicsTracker v0.1.0";
    rCfg.uiScale = cfg.uiScale;

    if (!m_renderer.initialize(rCfg)) {
        spdlog::critical("Renderer initialization failed");
        return false;
    }

    // Camera manager
    m_cameraManager = std::make_unique<CameraManager>(cfg.ringBufferCapacity);

    // Create session manager
    m_session = std::make_unique<SessionManager>(
        *m_cameraManager, m_calibStore, m_tracking,
        m_viewport, m_notifications, cfg);

    // Init control panel state
    m_cpState.newCameraWidth  = cfg.defaultWidth;
    m_cpState.newCameraHeight = cfg.defaultHeight;
    m_cpState.newCameraFps    = cfg.defaultFps;

    spdlog::info("Application initialized");
    return true;
}

void Application::run() {
    while (m_renderer.beginFrame()) {
        float dt = 1.0f / ImGui::GetIO().Framerate;

        // Update notifications
        m_notifications.update(dt);

        // Update control panel state
        m_cpState.capturing     = m_session && m_session->isCapturing();
        m_cpState.activeCameras = static_cast<int>(m_cameraManager->cameraCount());
        m_cpState.fps           = m_renderer.fps();

        // Draw UI panels
        auto action = m_controlPanel.draw(m_cpState);
        handleAction(action);

        auto& joints = m_configManager.config().joints;
        if (m_jointEditor.draw(joints)) {
            // Joint config changed — reconfigure tracking live
            OneEuroFilter::Settings fs;
            fs.minCutoff = static_cast<double>(m_configManager.config().filterMinCutoff);
            fs.beta      = static_cast<double>(m_configManager.config().filterBeta);
            fs.dCutoff   = static_cast<double>(m_configManager.config().filterDCutoff);
            m_tracking.configure(joints, fs);
        }

        // Feed latest camera frames to viewport for display
        feedViewportFrames();

        m_viewport.draw();
        m_notifications.draw();

        m_renderer.endFrame();
    }
}

void Application::shutdown() {
    if (m_session && m_session->isCapturing())
        m_session->stopCapture();

    m_cameraManager->closeAll();
    m_renderer.shutdown();

    // Save config on exit
    if (!m_configManager.loadedPath().empty())
        m_configManager.save(m_configManager.loadedPath());

    spdlog::info("Application shut down");
}

void Application::handleAction(ControlPanel::Action action) {
    auto& cfg = m_configManager.config();

    switch (action) {
    case ControlPanel::Action::AddCamera: {
        CameraDevice::Settings cs;
        cs.deviceIndex = m_cpState.newCameraIndex;
        cs.width       = m_cpState.newCameraWidth;
        cs.height      = m_cpState.newCameraHeight;
        cs.fps         = m_cpState.newCameraFps;

        if (m_cameraManager->addCamera(cs)) {
            // Try loading saved calibration
            CameraCalibration calib;
            if (m_calibStore.load("config", cs.deviceIndex, calib)) {
                auto* cam = m_cameraManager->camera(cs.deviceIndex);
                if (cam) cam->setCalibration(calib);
            }
            m_notifications.push("Camera " + std::to_string(cs.deviceIndex) + " added");
        } else {
            m_notifications.push("Failed to open camera " + std::to_string(cs.deviceIndex),
                                 NotificationManager::Level::Error);
        }
        break;
    }
    case ControlPanel::Action::StartCapture:
        if (m_session) m_session->startCapture();
        break;

    case ControlPanel::Action::StopCapture:
        if (m_session) m_session->stopCapture();
        break;

    case ControlPanel::Action::StartCalibration:
        m_notifications.push("Calibration not yet wired to UI — use config file",
                             NotificationManager::Level::Warning);
        break;

    default:
        break;
    }
}

void Application::feedViewportFrames() {
    // Grab the latest frame from each camera's ring buffer for display.
    // We peek (non-destructive) so the detection pool can still consume.
    for (int camId : m_cameraManager->activeCameraIds()) {
        auto* rb = m_cameraManager->ringBuffer(camId);
        if (!rb) continue;

        auto frame = rb->peekNewest();
        if (frame.has_value())
            m_viewport.updateFrame(camId, frame->image);
    }
}

} // namespace kt
