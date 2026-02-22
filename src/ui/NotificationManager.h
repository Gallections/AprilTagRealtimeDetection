#pragma once

#include <string>
#include <vector>

namespace kt {

/// Simple timed-toast notification overlay.
class NotificationManager {
public:
    enum class Level { Info, Warning, Error };

    void push(const std::string& message, Level level = Level::Info, float durationSec = 3.0f);

    /// Draw active notifications as ImGui overlay.
    void draw();

    /// Call once per frame to expire old notifications.
    void update(float deltaTime);

private:
    struct Toast {
        std::string message;
        Level       level;
        float       remaining;
    };

    std::vector<Toast> m_toasts;
};

} // namespace kt
