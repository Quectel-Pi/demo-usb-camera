# Camera Studio

Camera Studio is a Linux desktop USB camera application built with Qt Quick/QML and C++. It supports live preview, JPG photo capture, MP4 video recording, multiple-camera selection, device rescanning, and configurable H.264 encoders.

## Features

- Live preview from V4L2/UVC cameras
- Photo capture as JPG and video recording as MP4
- Multiple-camera selection and device rescanning
- Automatic selection of the best capture mode
- Clear diagnostics for permissions, missing GStreamer plugins, and unavailable encoders

## Technology Stack

- Qt Quick, QML, and Qt Quick Controls 2
- C++11, Qt, GStreamer 1.0, and Linux V4L2
- qmake, make, and pkg-config

See [ARCHITECTURE.md](ARCHITECTURE.md) for the detailed design and data flow.

## Requirements

- Linux
- One or more V4L2-compatible USB UVC cameras
- A camera supporting `MJPG` or `YUYV` (MJPEG at 30 FPS or higher is recommended)
- Qt 5.15 or a compatible version
- GStreamer development packages and plugins: `v4l2src`, `appsink`, `appsrc`, `jpegdec`, `videoconvert`, `h264parse`, and `mp4mux`
- At least one H.264 encoder: `mpph264enc`, `x264enc`, `openh264enc`, or `avenc_h264`
- A C++11 compiler, qmake, make, and pkg-config

## Build and Run

```bash
qmake camapp.pro
make
./camapp
```

If camera access fails, add the user to the `video` group and log in again:

```bash
sudo usermod -aG video "$USER"
```

## Video Encoder Configuration

The default encoder priority is:

```text
mpph264enc -> x264enc -> openh264enc -> avenc_h264
```

Select an encoder explicitly with:

```bash
CAMAPP_VIDEO_ENCODER=x264enc ./camapp
```

## Diagnostics

```bash
ls /dev/video*
gst-inspect-1.0 v4l2src
gst-inspect-1.0 jpegdec
gst-inspect-1.0 x264enc
gst-inspect-1.0 openh264enc
gst-inspect-1.0 avenc_h264
gst-inspect-1.0 mpph264enc
```

## Output Files

- Photos: `~/Pictures/Camera/IMG_yyyyMMdd_HHmmss_zzz.jpg`
- Videos: `~/Videos/Camera/VID_yyyyMMdd_HHmmss.mp4`

## Source Tree Cleanup

Do not commit build products such as `camapp`, `Makefile`, `.qmake.stash`, object files, `moc_*`, or generated `qrc_*.cpp` files. They are ignored by `.gitignore`.

