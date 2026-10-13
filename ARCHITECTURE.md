# USB Camera 架构文档

[English version](ARCHITECTURE.en.md)

## 目录

- [1. 项目概述](#1-项目概述)
- [2. 硬件架构](#2-硬件架构)
- [3. 软件架构与数据流](#3-软件架构与数据流)
- [4. 软件组件](#4-软件组件)
- [5. 详细设计](#5-详细设计)
- [6. 项目效果](#6-项目效果)
- [7. 工程目录](#7-工程目录)
- [8. 构建与运行](#8-构建与运行)

## 1. 项目概述

USB Camera 是一个运行在 Linux 桌面环境中的相机应用，支持：

- 实时显示摄像头画面
- 拍照
- 录像
- 多摄像头切换
- 摄像头设备重扫
- 权限、设备、插件和编码器错误提示

项目采用分层架构：

- QML：负责界面显示和用户交互
- C++：负责应用控制和状态管理
- V4L2：负责摄像头发现与能力探测
- GStreamer：负责视频采集、图像转换、编码和文件封装

## 2. 硬件架构

应用运行在 Linux 设备上，通过 USB 连接一个或多个 UVC 摄像头。操作系统会将摄像头呈现为 V4L2 视频设备，应用通过以下路径访问设备：

- `/dev/video*`
- `/dev/v4l/by-id/*`

摄像头至少需要支持 `MJPG` 或 `YUYV` 采集格式。录像功能还需要系统安装可用的 H.264 编码器，编码器可以是软件编码器，也可以是平台相关的硬件编码器。

```text
USB UVC 摄像头
      │ USB
      ▼
Linux V4L2 设备（/dev/video*）
      │
      ▼
USB Camera 应用
```

## 3. 软件架构与数据流

```text
QML 用户界面
  Main.qml / 预览 / 拍照 / 录像 / 设备选择
                       │ Q_PROPERTY / Q_INVOKABLE / signals
                       ▼
CameraController
  生命周期 / 状态 / 拍照 / 录像 / 切换 / 错误处理
          │                         │
          ▼                         ▼
CameraEnumerator                 CameraRecorder
V4L2 枚举与模式选择              appsrc / H.264 / MP4
          │                         │
          ▼                         ▼
Linux V4L2 设备                 GStreamer 管线

v4l2src → 转换 → appsink → QImage → 预览
appsrc → 编码 → mp4mux → MP4
```

软件启动后，`main.cpp` 会：

1. 创建 Qt 应用和 QML 引擎。
2. 将 `CameraController` 以 `camera` 属性注入 QML。
3. 将 `CameraPreview` 注册为 QML 类型。
4. 枚举摄像头并选择最佳采集模式。
5. 启动 GStreamer 采集管线。

## 4. 软件组件

| 组件 | 职责 |
| --- | --- |
| Qt Quick / QML | 窗口、控件、实时状态展示和用户交互 |
| Qt C++ | 应用控制器、信号槽通信、文件路径和线程安全的数据交换 |
| Linux V4L2 | 摄像头发现、设备能力读取、像素格式/分辨率/帧率枚举 |
| GStreamer 1.0 | 视频采集、解码、格式转换、预览分流、H.264 编码和 MP4 封装 |
| qmake / pkg-config | 工程配置、依赖发现和构建 |

## 5. 详细设计

### 5.1 用户界面层

界面由 `qml/Main.qml` 实现，使用 Qt Quick Controls 2 构建，主要区域包括：

- 中央实时预览区域：等比例显示摄像头画面。
- 顶部状态标签：显示当前分辨率、FPS 和 LIVE/REC 状态。
- 底部操作栏：提供拍照、录像、摄像头选择和 Rescan 操作。
- Toast 提示：显示照片保存、录像启动、设备异常和插件缺失等结果。

QML 不直接操作 V4L2 或 GStreamer，而是通过以下方法调用后端能力：

- `camera.takePhoto()`
- `camera.startRecording()`
- `camera.stopRecording()`
- `camera.switchCamera(index)`

QML 通过以下属性更新界面：

- `camera.running`
- `camera.recording`
- `camera.errorMessage`
- `camera.cameraList`
- `camera.cameraWidth`
- `camera.cameraHeight`
- `camera.cameraFps`

### 5.2 应用控制层

`CameraController` 是应用的核心协调模块，负责连接界面、摄像头枚举器和录像器。

主要职责：

- 管理摄像头启动、停止、重扫和切换。
- 将摄像头列表、当前模式、实时 FPS 和录像时长暴露给 QML。
- 创建和销毁预览采集管线。
- 接收采集帧并转换为 `QImage`。
- 通过 `frameReady` 信号将图像交给预览组件。
- 调用 `CameraRecorder` 完成录像。
- 生成照片和视频保存路径。
- 将权限、设备、管线和编码器错误转换为可读提示。

### 5.3 摄像头枚举与采集模式选择

`CameraEnumerator` 使用 V4L2 接口探测设备。枚举时：

1. 优先扫描稳定路径 `/dev/v4l/by-id/*video-index0`。
2. 再扫描 `/dev/video*`。
3. 通过设备真实路径去重，避免同一设备重复出现。

当前支持以下采集格式：

- `MJPG`
- `YUYV`

模式选择采用评分策略：

1. 优先选择 MJPEG。
2. 优先选择 30 FPS 及以上的模式。
3. 综合考虑帧率和分辨率。
4. 从摄像头能力列表中选择评分最高的模式。

### 5.4 实时预览

MJPEG 摄像头的采集管线通过 `tee` 分成预览和拍照两路：

```text
v4l2src
  → image/jpeg
  → tee
      ├→ jpegdec → videoconvert → appsink(previewSink)
      │                              → QImage → CameraPreview
      └→ appsink(jpegSink) → 保存原始 JPEG
```

YUYV 摄像头则直接经过 `videoconvert` 转换为 BGRx，然后输出到预览 appsink。

`CameraPreview` 继承 `QQuickPaintedItem`，使用互斥锁保护最新帧，并在 `paint()` 中按比例缩放和居中绘制。

### 5.5 拍照功能

用户点击拍照按钮后：

1. MJPEG 模式优先保存管线中的最新原始 JPEG，避免重复编码。
2. YUYV 模式保存最新的 `QImage` 为 JPG。
3. 照片保存到系统图片目录下的 `Camera` 子目录。

照片文件名格式：

```text
IMG_yyyyMMdd_HHmmss_zzz.jpg
```

### 5.6 录像功能

录像时，预览帧同时被推送到 `CameraRecorder` 的 `appsrc`。录像管线将图像转换为编码器需要的格式，编码为 H.264 后通过 `h264parse` 和 `mp4mux` 封装为 MP4：

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

编码器选择顺序：

1. 如果设置了 `CAMAPP_VIDEO_ENCODER`，使用指定编码器。
2. 未指定时依次尝试：
   - `mpph264enc`
   - `x264enc`
   - `openh264enc`
   - `avenc_h264`
3. 如果没有可用编码器，返回明确错误，不进入录像状态。

视频默认保存到系统视频目录下的 `Camera` 子目录，文件名格式为：

```text
VID_yyyyMMdd_HHmmss.mp4
```

### 5.7 多摄像头切换

摄像头枚举完成后，控制器将设备名称列表暴露给 QML 的 `ComboBox`。

用户选择其他设备时：

1. 停止当前 GStreamer 管线。
2. 更新当前摄像头索引。
3. 重新检测采集模式。
4. 启动新的采集管线。

录像期间禁用设备切换，避免录制文件和采集管线状态不一致。

### 5.8 错误处理

后端统一通过 `errorMessage` 属性和 `error` 信号向界面报告错误，QML 监听属性变化并显示 Toast。

主要错误场景包括：

- 未发现摄像头设备。
- 摄像头无访问权限。
- 摄像头不支持 MJPG 或 YUYV。
- GStreamer 缺少 `v4l2src`、`appsink`、`videoconvert` 等插件。
- 采集管线启动失败或设备被其他程序占用。
- 没有可用的 H.264 编码器。

## 6. 项目效果

应用启动后进入实时预览状态，界面显示摄像头名称、分辨率、像素格式和当前帧率。用户可以在同一界面完成拍照和录像，并通过底部控件重新扫描设备或切换摄像头。

> 截图位置可在此处补充。

## 7. 工程目录

```text
camapp_latest/
├── camapp.pro
├── qml/
│   ├── Main.qml
│   └── qml.qrc
└── src/
    ├── main.cpp
    ├── CameraController.cpp
    ├── CameraController.h
    ├── CameraEnumerator.cpp
    ├── CameraEnumerator.h
    ├── CameraDevice.h
    ├── CameraPreview.cpp
    ├── CameraPreview.h
    ├── CameraRecorder.cpp
    └── CameraRecorder.h
```

## 8. 构建与运行

```bash
qmake camapp.pro
make
./camapp
```

主要依赖：

- Qt 5.15 或兼容版本
- 支持 C++11 的编译器
- GStreamer 1.0 开发包
- qmake
- make
- pkg-config
- 至少一个可用的 H.264 编码器插件

