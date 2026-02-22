# Realtime Object Detection PRD

# Product Requirements Document: Real-Time AprilTag Joint Kinematics Tracker

## 1. Project Overview

The objective is to develop a high-performance, real-time desktop application capable of detecting and tracking AprilTags via a camera feed. The primary use case is measuring joint movement and biomechanical kinematics. The system must prioritize high-frequency data capture, sub-millimeter precision in 3D pose estimation, and a streamlined user interface.

---

## 2. Functional Requirements

### 2.1 Camera and Calibration

- **Multi-Camera Support:** The system must interface with standard UVC cameras, industrial USB3 cameras, and high-speed global shutter cameras.
- **Intrinsic Calibration:** Implementation of a calibration routine (e.g., Zhang’s method) using a checkerboard or circle grid to determine camera matrix and distortion coefficients.
- **Resolution/FPS Control:** Users must be able to specify camera resolution and target frame rate (e.g., 60, 120, or 240 FPS) based on hardware capability.

### 2.2 Real-Time Detection Engine

- **Algorithm:** Integration of the AprilTag 3 library for robust detection under varying lighting and motion blur.
- **Pose Estimation:** Calculation of 6-DOF (Degrees of Freedom) transformation matrices for every detected tag relative to the camera center.
- **Tag Families:** Configurable support for `tag36h11`, `tagStandard41h12`, and `tagCircle21h7`.
- **Multi-Tag Tracking:** Simultaneous tracking of multiple unique IDs without significant latency degradation.

### 2.3 User Interface (UI)

- **Live Viewport:** A real-time video overlay displaying:
    - Detected tag boundaries and ID labels.
    - 3D coordinate axes ($X, Y, Z$) projected onto the tag centers.
    - Current processing latency (ms) and effective FPS.
- **Session Controls:**
    - **Start Capture:** Initializes the camera and detector; begins data buffering.
    - **End Capture:** Stops the stream, flushes buffered data to a file, and terminates the session.
- **Notification System:** * Visual "Toast" or pop-up alerts for "No Tags Detected."
    - Warnings for "High Motion Blur Detected" or "Inconsistent Lighting."

### 2.4 Data Management and Output

- **Time-Series Logging:** Data must be logged at the highest possible frequency allowed by the hardware.
- **Output Schema:** A CSV/JSON export containing:
    - `Timestamp` (Microsecond precision).
    - `Tag_ID`.
    - `Translation` ($x, y, z$).
    - `Rotation` (Quaternion: $w, x, y, z$ or Euler angles).
- **Joint Linkage Logic:** Calculation of relative transforms between specific Tag IDs (e.g., calculating the angle between a "thigh" tag and "shin" tag).

---

## 3. Non-Functional Requirements

### 3.1 Performance and Accuracy

- **High Framerate:** The detection pipeline must be optimized for low-latency processing to support 120+ FPS tracking on modern multi-core CPUs.
- **Sub-Millimeter Precision:** Pose estimation jitter must be minimized through temporal filtering (e.g., Kalmar Filter or 1€ Filter).
- **Concurrency:** The UI thread must remain responsive; image acquisition and tag detection should occur on separate high-priority threads.

### 3.2 Usability

- **Clean Interface:** Minimalist design with high-contrast elements for visibility in laboratory or gym environments.
- **Portability:** The application should be packaged as a standalone executable with minimal external dependencies.

---

## 4. Technical Constraints

- **Language:** C++17 or higher for performance-critical components.
- **Vision Backend:** OpenCV 4.x for image handling and AprilTag 3 C-library.
- **GUI Framework:** Lightweight framework (e.g., Dear ImGui or Qt) to ensure minimal CPU overhead for the interface.
- **Math:** Utilization of high-performance linear algebra libraries (e.g., Eigen) for transformation matrix calculations.

---

## 5. Success Metrics

- **Zero Dropped Frames:** The system maintains the target camera FPS during the entire capture duration.
- **Detection Robustness:** Successful tracking at angles up to 60 degrees relative to the camera plane.
- **Data Integrity:** Exported time-series data shows continuous, non-stepped movement curves.

---