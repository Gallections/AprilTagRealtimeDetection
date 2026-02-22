# KinematicsTracker — Real-Time AprilTag Joint Kinematics Tracker

A high-performance desktop application for detecting and tracking AprilTags via camera feed, measuring joint movement and biomechanical kinematics in real time.

## Features

- **Multi-camera simultaneous capture** at 120+ FPS
- **AprilTag 3 detection** with configurable tag families (`tag36h11`, `tagStandard41h12`, `tagCircle21h7`)
- **6-DOF pose estimation** with sub-millimeter precision (1€ temporal filtering)
- **Joint angle tracking** from relative transforms between tag pairs
- **Live viewport** with tag boundary overlays, 3D axis projection, and latency HUD
- **Time-series export** to CSV or NDJSON with microsecond timestamps
- **Camera calibration** (Zhang's method) with persistent profiles

## Prerequisites

- **CMake** 3.21+
- **vcpkg** (with `VCPKG_ROOT` environment variable set)
- **C++17** compiler (MSVC 2019+, GCC 9+, Clang 10+, AppleClang 12+)

## Build

```bash
# Configure
cmake --preset debug      # or: cmake --preset release

# Build
cmake --build --preset debug
```

## Run

```bash
./build/debug/KinematicsTracker                       # uses config/default_config.toml
./build/debug/KinematicsTracker path/to/config.toml   # custom config
```

## Configuration

Edit `config/default_config.toml` to configure cameras, detection parameters, tracking filters, export format, and joint definitions. Joints can also be added/edited live via the in-app Joint Configuration panel.

## Project Structure

```
src/
├── main.cpp                    # Entry point
├── common/Types.h              # Shared data types
├── app/
│   ├── Application.h/.cpp      # Top-level lifecycle & main loop
│   └── SessionManager.h/.cpp   # Capture session orchestration
├── camera/
│   ├── CameraDevice.h/.cpp     # Single camera abstraction
│   ├── CameraManager.h/.cpp    # Multi-camera registry
│   └── FrameRingBuffer.h       # Lock-free SPSC ring buffer
├── calibration/
│   ├── Calibrator.h/.cpp       # Zhang's method calibration
│   └── CalibrationStore.h/.cpp # Persistent calibration profiles
├── detection/
│   ├── TagDetector.h/.cpp      # AprilTag 3 C-library wrapper
│   ├── PoseEstimator.h/.cpp    # solvePnP → 6-DOF Eigen transforms
│   └── DetectionPool.h/.cpp    # Multi-threaded detection workers
├── tracking/
│   ├── OneEuroFilter.h         # 1€ adaptive low-pass filter
│   └── JointLinkage.h/.cpp     # Relative transform & joint angle solver
├── ui/
│   ├── Renderer.h/.cpp         # Dear ImGui + GLFW + OpenGL3 bootstrap
│   ├── Viewport.h/.cpp         # Camera feed textures + overlays
│   ├── ControlPanel.h/.cpp     # Session & camera controls
│   ├── JointConfigEditor.h/.cpp# Live joint definition editor
│   └── NotificationManager.h/  # Timed toast notifications
└── io/
    ├── ConfigManager.h/.cpp    # TOML config read/write
    ├── TimeSeriesLogger.h/.cpp # Async MPSC queue → disk
    ├── CsvWriter.h/.cpp        # CSV export
    └── JsonWriter.h/.cpp       # NDJSON export
```
