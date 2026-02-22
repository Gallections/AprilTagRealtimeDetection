#include "app/Application.h"

#include <spdlog/spdlog.h>

int main(int argc, char* argv[]) {
    spdlog::set_level(spdlog::level::info);
    spdlog::info("KinematicsTracker v0.1.0 starting...");

    std::string configPath = "config/default_config.toml";
    if (argc > 1)
        configPath = argv[1];

    try {
        kt::Application app;
        if (!app.initialize(configPath)) {
            spdlog::critical("Failed to initialize application");
            return 1;
        }
        app.run();
        app.shutdown();
    } catch (const std::exception& e) {
        spdlog::critical("Unhandled exception: {}", e.what());
        return 1;
    }

    spdlog::info("KinematicsTracker shut down cleanly.");
    return 0;
}
