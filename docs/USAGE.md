# KinematicsTracker — Usage Guide

## How to Build

### Prerequisites

- **CMake** 3.21+
- **vcpkg** (with `VCPKG_ROOT` environment variable set)
- **C++17** compiler (MSVC 2019+, GCC 9+, Clang 10+, AppleClang 12+)

### Steps

```powershell
# 1. Configure (downloads & builds dependencies on first run — ~10-30 min)
cmake --preset debug

# 2. Build
cmake --build --preset debug

# 3. Run
.\build\debug\Debug\KinematicsTracker.exe
```

For an optimized build, replace `debug` with `release` in all steps.

---

## How to Run & Use

### 1. Launch

```powershell
.\build\debug\Debug\KinematicsTracker.exe
```

The application opens a Dear ImGui window with three dockable panels: **Controls**, **Joint Configuration**, and camera viewports.

You can optionally pass a custom config file:

```powershell
.\build\debug\Debug\KinematicsTracker.exe path\to\my_config.toml
```

---

### 2a. Load a Video File

Instead of (or in addition to) a live camera, you can analyze pre-recorded video:

1. In the **Controls** panel, enter the full path in the **Video Path** field (e.g., `C:\videos\session1.mp4`)
2. Click **Load Video** — a viewport window appears with the video
3. Click **Start Capture** — the video plays back at its native FPS while the detection pipeline processes every frame
4. When the video ends, capture **auto-stops** and all data is flushed to the CSV/JSON export

Supported formats: anything OpenCV can decode (`.mp4`, `.avi`, `.mkv`, `.mov`, etc.)

You can load multiple videos simultaneously, or mix live cameras with video files.

---

### 2b. Add a Live Camera

In the **Controls** panel:

1. Set **Device Index** to your camera number (`0` = default webcam, `1` = second camera, etc.)
2. Set desired **Width**, **Height**, and **FPS**
3. Click **Add Camera**

A new **Camera 0** viewport window appears. Repeat for additional cameras — they run simultaneously.

---

### 3. Prepare AprilTags

- Print AprilTags from the `tag36h11` family (default). Printable PDFs are available at [apriltag-imgs](https://github.com/AprilRobotics/apriltag-imgs).
- Mount tags on the body segments you want to track (e.g., one on the thigh, one on the shin).
- Measure each tag's physical size in **meters** (e.g., a 6cm tag = `0.06`).

---

### 4. Define Joints

In the **Joint Configuration** panel:

1. Click **Add Joint**
2. Expand the new joint and set:
   - **Name** — e.g., `right_knee`
   - **Proximal Tag ID** — the tag on the upper segment (e.g., `1`)
   - **Distal Tag ID** — the tag on the lower segment (e.g., `2`)
   - **Proximal/Distal Size (mm)** — physical tag dimensions
3. Repeat for each joint. Changes apply live.

Joints can also be predefined in the config file (see [Configuration](#7-configuration)).

---

### 5. Start Capture

1. Click **Start Capture** in the Controls panel
2. The full pipeline activates: camera threads → detection pool → tracking → data logging
3. The viewport shows:
   - **Green quads** around detected tags
   - **ID labels** on each tag
   - **RGB axes** (X/Y/Z) at tag centers when calibrated
   - **Tag count** and **detection latency** in the HUD
4. Toast notifications appear for warnings (e.g., "No Tags Detected")

---

### 6. Stop & Export

1. Click **Stop Capture**
2. All buffered data flushes to disk automatically
3. Output file is saved to `./data/` as `capture_YYYYMMDD_HHMMSS.csv` (or `.ndjson`)

#### CSV Output Schema

```
timestamp_us, camera_id, tag_id, tx, ty, tz, qw, qx, qy, qz, joint_name, joint_angle_deg
```

| Column | Description |
|---|---|
| `timestamp_us` | Microsecond-precision timestamp |
| `camera_id` | Source camera device index |
| `tag_id` | Detected AprilTag ID (-1 for joint-only rows) |
| `tx, ty, tz` | 3D translation (meters) |
| `qw, qx, qy, qz` | Rotation as quaternion |
| `joint_name` | Joint name (empty for tag-only rows) |
| `joint_angle_deg` | Joint angle in degrees |

---

### 7. Configuration

Edit `config/default_config.toml` to customize behavior:

```toml
[camera]
default_width  = 640
default_height = 480
default_fps    = 120        # Target framerate
ring_buffer_capacity = 8

[detection]
tag_family     = "tag36h11" # or "tagStandard41h12", "tagCircle21h7"
quad_decimate  = 2.0        # Lower = more accurate, higher = faster
quad_sigma     = 0.0
refine_edges   = true
detection_threads = 0       # 0 = auto (hardware_concurrency - 2)

[tracking]
filter_min_cutoff = 1.0     # 1€ filter: lower = smoother
filter_beta       = 0.007   # 1€ filter: higher = less lag during fast motion
filter_d_cutoff   = 1.0

[export]
format           = "csv"    # "csv" or "json"
directory        = "./data"
flush_interval_ms = 100
flush_batch_size  = 1000

[ui]
scale = 1.0

# Joint definitions (repeatable)
[[joints]]
name              = "right_knee"
proximal_tag_id   = 1
distal_tag_id     = 2
proximal_tag_size = 0.06    # meters
distal_tag_size   = 0.06
```

---

### 8. Camera Calibration (for 3D Pose)

Without calibration, you get 2D tag detection only (corners and IDs). For accurate 6-DOF pose estimation and joint angle calculation, you need to calibrate each camera.

1. Print a checkerboard calibration pattern
2. Click **Calibrate Selected Camera** in the Controls panel
3. Show the checkerboard to the camera from various angles (minimum 15 frames)
4. Calibration profiles are saved per-camera in the `config/` directory

---

## Project Structure

```
src/
├── main.cpp                     # Entry point
├── common/Types.h               # Shared data types
├── app/
│   ├── Application.h/.cpp       # Top-level lifecycle & main loop
│   └── SessionManager.h/.cpp    # Capture session orchestration
├── camera/
│   ├── CameraDevice.h/.cpp      # Single camera abstraction
│   ├── CameraManager.h/.cpp     # Multi-camera registry
│   └── FrameRingBuffer.h        # Lock-free SPSC ring buffer
├── calibration/
│   ├── Calibrator.h/.cpp        # Zhang's method calibration
│   └── CalibrationStore.h/.cpp  # Persistent calibration profiles
├── detection/
│   ├── TagDetector.h/.cpp       # AprilTag 3 C-library wrapper
│   ├── PoseEstimator.h/.cpp     # solvePnP → 6-DOF Eigen transforms
│   └── DetectionPool.h/.cpp     # Multi-threaded detection workers
├── tracking/
│   ├── OneEuroFilter.h          # 1€ adaptive low-pass filter
│   └── JointLinkage.h/.cpp      # Relative transform & joint angle solver
├── ui/
│   ├── Renderer.h/.cpp          # Dear ImGui + GLFW + OpenGL3 bootstrap
│   ├── Viewport.h/.cpp          # Camera feed textures + overlays
│   ├── ControlPanel.h/.cpp      # Session & camera controls
│   ├── JointConfigEditor.h/.cpp # Live joint definition editor
│   └── NotificationManager.h/.cpp # Timed toast notifications
└── io/
    ├── ConfigManager.h/.cpp     # TOML config read/write
    ├── TimeSeriesLogger.h/.cpp  # Async MPSC queue → disk
    ├── CsvWriter.h/.cpp         # CSV export
    └── JsonWriter.h/.cpp        # NDJSON export
```
