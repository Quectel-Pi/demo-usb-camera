# USB Camera Architecture

[中文版](ARCHITECTURE.md)

## Project Requirements

USB Camera is a Linux desktop camera application that displays a live camera feed and supports photo capture, video recording, multi-camera switching, and device rescanning.

The architecture is designed to be clear and extensible:

- QML handles the user interface and interaction.
- C++ handles application control and state management.
- V4L2 discovers cameras and probes their capabilities.
- GStreamer handles capture, image conversion, encoding, and file muxing.

## System Architecture

### Hardware Architecture

The application runs on a Linux device and connects to one or more UVC cameras over USB. Linux exposes each camera as a V4L2 video device. The application accesses devices through `/dev/video*` or `/dev/v4l/by-id/*`.

The camera must support MJPEG or YUYV capture. Video recording additionally requires an available H.264 encoder, either software-based or platform-specific hardware encoding.

```text
USB UVC camera
      │ USB
      ▼
Linux V4L2 device (/dev/video*)
      │
      ▼
USB Camera application
```

### Software Architecture and Data Flow

```text
QML user interface
  Main.qml / preview / photo / recording / device selection
                       │ Q_PROPERTY / Q_INVOKABLE / signals
                       ▼
CameraController
  lifecycle / state / photo / recording / switching / errors
          │                         │
          ▼                         ▼
CameraEnumerator                 CameraRecorder
V4L2 enumeration                 appsrc / H.264 / MP4
and mode selection
          │                         │
          ▼                         ▼
Linux V4L2 device              GStreamer pipeline

v4l2src → conversion → appsink → QImage → preview
appsrc → encoder → mp4mux → MP4
```

At startup, `main.cpp` creates the Qt application and QML engine, exposes `CameraController` as the `camera` property, and registers `CameraPreview` as a QML type. The controller then enumerates cameras, selects the best capture mode, and starts the GStreamer capture pipeline.

## Software Components

1. **Qt Quick / QML**: windows, controls, live status, and user interaction.
2. **Qt C++**: application control, signal/slot communication, file paths, and thread-safe frame exchange.
3. **Linux V4L2**: camera discovery, device capabilities, pixel formats, resolutions, and frame rates.
4. **GStreamer 1.0**: capture, decoding, pixel conversion, preview branching, H.264 encoding, and MP4 muxing.
5. **qmake / pkg-config**: project configuration, dependency discovery, and building.

## Detailed Design

### 1. User Interface Layer

The interface is implemented in `qml/Main.qml` with Qt Quick Controls 2. It contains:

- A central live preview area with aspect-ratio-preserving rendering.
- Status labels showing resolution, FPS, and LIVE/REC state.
- Controls for taking photos, recording video, selecting a camera, and rescanning devices.
- Toast messages for saved photos, recording state, device errors, and missing plugins.

QML does not access V4L2 or GStreamer directly. It calls backend methods such as `camera.takePhoto()` and `camera.startRecording()`, and observes properties such as `camera.running`, `camera.recording`, and `camera.errorMessage`.

### 2. Application Control Layer

`CameraController` is the central coordinator between the UI, camera enumerator, and recorder.

Responsibilities include:

- Starting, stopping, rescanning, and switching cameras.
- Exposing the camera list, current mode, live FPS, and recording duration to QML.
- Creating and destroying the preview pipeline.
- Converting captured frames to `QImage` and emitting them through `frameReady`.
- Delegating video recording to `CameraRecorder`.
- Generating photo and video output paths.
- Converting permission, device, pipeline, and encoder failures into readable messages.

### 3. Camera Enumeration and Mode Selection

`CameraEnumerator` probes devices through V4L2. It first scans stable paths under `/dev/v4l/by-id/*video-index0`, then scans `/dev/video*`, and removes duplicates using the device's real path.

The application supports MJPG and YUYV capture formats. Mode selection uses a score-based strategy:

1. Prefer MJPEG.
2. Prefer modes at 30 FPS or above.
3. Consider frame rate and resolution together.
4. Select the highest-scoring mode from the camera capabilities.

### 4. Live Preview

For MJPEG cameras, the pipeline is split into preview and photo branches with `tee`:

```text
v4l2src
  → image/jpeg
  → tee
      ├→ jpegdec → videoconvert → appsink(previewSink) → QImage → CameraPreview
      └→ appsink(jpegSink) → save the original JPEG
```

YUYV cameras go through `videoconvert`, are converted to BGRx, and are delivered to the preview appsink. `CameraPreview` inherits `QQuickPaintedItem`, protects the latest frame with a mutex, and scales and centers it during `paint()`.

### 5. Photo Capture

When the user clicks the photo button, the backend first saves the latest original JPEG from the MJPEG pipeline to avoid re-encoding. In YUYV mode, it saves the latest `QImage` as JPG.

Photos are saved under the `Camera` subdirectory of the system pictures directory:

```text
IMG_yyyyMMdd_HHmmss_zzz.jpg
```

### 6. Video Recording

During recording, preview frames are also pushed to `CameraRecorder` through `appsrc`. The recording pipeline converts frames to the encoder's required format, encodes them as H.264, and muxes them into an MP4 file:

```text
QImage
  → appsrc
  → queue
  → videoconvert
  → NV12
  → H.264 encoder
  → h264parse
  → mp4mux
  → filesink
```

Encoder selection follows this order:

1. If `CAMAPP_VIDEO_ENCODER` is set, use the specified encoder.
2. Otherwise try `mpph264enc`, `x264enc`, `openh264enc`, and `avenc_h264` in order.
3. If no encoder is available, report a clear error and do not enter the recording state.

Videos are saved under the `Camera` subdirectory of the system videos directory:

```text
VID_yyyyMMdd_HHmmss.mp4
```

### 7. Multi-Camera Switching

After enumeration, the controller exposes the device names to the QML `ComboBox`. When the user selects another device, the controller stops the current pipeline, updates the selected index, detects the new mode, and starts a new pipeline.

Camera switching is disabled during recording to keep the recording file and capture state consistent.

### 8. Error Handling

The backend reports errors through the `errorMessage` property and the `error` signal. QML observes property changes and displays a Toast.

Important error cases include:

- No camera device found.
- Camera access permission denied.
- Camera does not support MJPG or YUYV.
- GStreamer is missing `v4l2src`, `appsink`, `videoconvert`, or another required plugin.
- Pipeline startup failed or the device is already in use.
- No H.264 encoder is available.

## Expected User Experience

After startup, the application enters live preview mode and shows the camera name, resolution, pixel format, and current frame rate. The user can take photos and record videos from the same interface, rescan devices, or switch cameras using the bottom controls.

## Project Layout and Build

```text
camapp_latest/
├── camapp.pro
├── qml/
│   ├── Main.qml
│   └── qml.qrc
└── src/
    ├── main.cpp
    ├── CameraController.cpp/.h
    ├── CameraEnumerator.cpp/.h
    ├── CameraDevice.h
    ├── CameraPreview.cpp/.h
    └── CameraRecorder.cpp/.h
```

Build and run:

```bash
qmake camapp.pro
make
./camapp
```

The main dependencies are Qt 5.15 or a compatible version, a C++11 compiler, GStreamer 1.0 development packages, qmake, make, and pkg-config. Recording requires at least one available H.264 encoder plugin.

