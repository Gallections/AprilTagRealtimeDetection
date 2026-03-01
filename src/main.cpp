#include "app/Application.h"

#include <spdlog/spdlog.h>

#include <filesystem>

static std::string resolveConfigPath(const char* argv0, const std::string& relative) {
    namespace fs = std::filesystem;
    // Try relative to CWD first
    if (fs::exists(relative))
        return relative;
    // Try relative to the executable's directory
    fs::path exeDir = fs::path(argv0).parent_path();
    fs::path candidate = exeDir / relative;
    if (fs::exists(candidate))
        return candidate.string();
    // Fallback
    return relative;
}

int main(int argc, char* argv[]) {
    spdlog::set_level(spdlog::level::info);
    spdlog::info("KinematicsTracker v0.1.0 starting...");

    std::string configPath = "config/default_config.toml";
    if (argc > 1)
        configPath = argv[1];
    else
        configPath = resolveConfigPath(argv[0], configPath);

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
