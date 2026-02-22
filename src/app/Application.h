#pragma once

#include "camera/CameraManager.h"
#include "calibration/Calibrator.h"
#include "calibration/CalibrationStore.h"
#include "tracking/JointLinkage.h"
#include "io/ConfigManager.h"
#include "ui/Renderer.h"
#include "ui/Viewport.h"
#include "ui/ControlPanel.h"
#include "ui/JointConfigEditor.h"
#include "ui/NotificationManager.h"
#include "app/SessionManager.h"

#include <memory>
#include <string>

namespace kt {

/// Top-level application: owns all subsystems, runs the main loop.
class Application {
public:
    Application();
    ~Application();

    bool initialize(const std::string& configPath);
    void run();
    void shutdown();

private:
    void handleAction(ControlPanel::Action action);
    void feedViewportFrames();

    // Config
    ConfigManager       m_configManager;

    // Subsystems
    Renderer                            m_renderer;
    std::unique_ptr<CameraManager>      m_cameraManager;
    CalibrationStore                    m_calibStore;
    JointLinkage        m_tracking;

    // UI panels
    Viewport            m_viewport;
    ControlPanel        m_controlPanel;
    JointConfigEditor   m_jointEditor;
    NotificationManager m_notifications;

    // Session
    std::unique_ptr<SessionManager> m_session;

    // Control panel state
    ControlPanel::State m_cpState;
};

} // namespace kt
