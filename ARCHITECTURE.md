
[English version](ARCHITECTURE.en.md)

项目需求

Camera Studio 是一个运行在 Linux 桌面环境的相机应用，用于对外展示相机实时画面，并提供拍照、录像、多摄像头切换和设备重扫等功能。

项目重点是建立一套清晰、可扩展的软件架构：由 QML 负责界面交互，由 C++ 负责应用控制和状态管理，由 V4L2 负责摄像头发现与能力探测，由 GStreamer 负责视频采集、图像转换、编码和文件封装。

项目架构

硬件架构

应用运行在 Linux 设备上，通过 USB 连接一个或多个 UVC 摄像头。操作系统将摄像头呈现为 V4L2 视频设备，应用通过 /dev/video* 或 /dev/v4l/by-id/* 访问。

硬件部分只要求摄像头支持 MJPEG 或 YUYV 采集格式。录像功能还需要系统安装可用的 H.264 编码器，编码器可以是软件编码器，也可以是平台相关的硬件编码器。

USB UVC 摄像头
        │ USB
        ▼
Linux V4L2 设备（/dev/video*）
        │
        ▼
Camera Studio 应用

软件架构/流程

┌──────────────────────────────────────────────┐
│                QML 用户界面                   │
│ Main.qml / 预览 / 拍照 / 录像 / 设备选择       │
└──────────────────────┬───────────────────────┘
                       │ Q_PROPERTY / Q_INVOKABLE / signals
                       ▼
┌──────────────────────────────────────────────┐
│             CameraController                  │
│ 生命周期、状态、拍照、录像、切换、错误处理     │
└──────────────┬──────────────┬────────────────┘
               │              │
               ▼              ▼
┌────────────────────┐  ┌────────────────────┐
│ CameraEnumerator    │  │ CameraRecorder     │
│ V4L2 枚举与模式选择 │  │ appsrc/H.264/MP4   │
└─────────┬──────────┘  └─────────┬──────────┘
          │                        │
          ▼                        ▼
   Linux V4L2 设备             GStreamer 管线
               ┌──────────────────────────────┐
               │ v4l2src → 转换 → appsink      │
               │             │                │
               │             └→ QImage → 预览  │
               │ appsrc → 编码 → mp4mux → MP4  │
               └──────────────────────────────┘

软件启动后，main.cpp 创建 Qt 应用和 QML 引擎，将 CameraController 以 camera 属性注入 QML，并将 CameraPreview 注册为 QML 类型。随后控制器枚举摄像头、选择最佳采集模式并启动 GStreamer 采集管线。

软件驱动列表

1. Qt Quick / QML：负责窗口、控件、实时状态展示和用户交互。
2. Qt C++：负责应用控制器、信号槽通信、文件路径和线程安全的数据交换。
3. Linux V4L2：负责发现摄像头、读取设备能力，并枚举像素格式、分辨率和帧率。
4. GStreamer 1.0：负责视频采集、解码、像素格式转换、预览分流、H.264 编码和 MP4 封装。
5. qmake / pkg-config：负责工程配置、依赖发现和构建。

项目方案介绍

1. 用户界面层

界面由 qml/Main.qml 实现，采用 Qt Quick Controls 2 构建，主要区域包括：

- 中央实时预览区域，等比例显示摄像头画面。
- 顶部状态标签，显示当前分辨率、FPS 和 LIVE/REC 状态。
- 底部操作栏，提供拍照、录像、摄像头选择和 Rescan 操作。
- Toast 提示，用于展示照片保存、录像启动、设备异常和插件缺失等结果。
QML 不直接操作 V4L2 或 GStreamer，而是通过 camera.takePhoto()、camera.startRecording() 等方法调用后端能力，并通过 camera.running、camera.recording、camera.errorMessage 等属性更新显示。

2. 应用控制层

CameraController 是应用的核心协调模块，负责连接界面、摄像头枚举器和录像器。

主要职责：

- 管理摄像头启动、停止、重扫和切换。
- 将摄像头列表、当前模式、实时 FPS 和录像时长暴露给 QML。
- 创建和销毁预览采集管线。
- 接收采集帧并转为 QImage，通过 frameReady 信号交给预览组件。
- 调用 CameraRecorder 完成录像。
- 生成照片和视频保存路径。
- 将权限、设备、管线和编码器错误转换为可读提示。
3. 摄像头枚举与采集模式选择

CameraEnumerator 使用 V4L2 接口探测设备。枚举时优先使用稳定路径 /dev/v4l/by-id/*video-index0，再扫描 /dev/video*，并通过设备真实路径去重。

当前支持 MJPG 和 YUYV 两种采集格式。模式选择采用评分策略：优先 MJPEG，其次优先 30 FPS 及以上的模式，再综合考虑帧率和分辨率，从摄像头能力列表中选择最佳模式。

4. 实时预览

MJPEG 摄像头的采集管线通过 tee 分成预览和拍照两路：

v4l2src
  → image/jpeg
  → tee
      ├→ jpegdec → videoconvert → appsink(previewSink) → QImage → CameraPreview
      └→ appsink(jpegSink) → 保存原始 JPEG

YUYV 摄像头则直接经过 videoconvert 转换为 BGRx 后输出到预览 appsink。CameraPreview 继承 QQuickPaintedItem，用互斥锁保护最新帧，在 paint() 中按比例缩放并居中绘制。

5. 拍照功能

用户点击拍照按钮后，后端优先保存 MJPEG 管线中的最新原始 JPEG 数据，避免重复编码；对于 YUYV 模式，则保存最新的 QImage 为 JPG。照片保存到系统图片目录下的 Camera 子目录，文件名格式为：

IMG_yyyyMMdd_HHmmss_zzz.jpg

6. 录像功能

录像时，预览帧同时被推送到 CameraRecorder 的 appsrc。录像管线将图像转换为编码器需要的格式，编码为 H.264 后通过 h264parse 和 mp4mux 封装为 MP4：

QImage
  → appsrc
  → queue
  → videoconvert
  → NV12
  → H.264 encoder
  → h264parse
  → mp4mux
  → filesink

编码器选择顺序如下：

1. 如果设置了 CAMAPP_VIDEO_ENCODER，使用指定编码器。
2. 未指定时依次尝试 mpph264enc、x264enc、openh264enc、avenc_h264。
3. 如果没有可用编码器，返回明确错误，不进入录像状态。
视频默认保存到系统视频目录下的 Camera 子目录，文件名格式为：

VID_yyyyMMdd_HHmmss.mp4

7. 多摄像头切换

摄像头枚举完成后，控制器将设备名称列表暴露给 QML 的 ComboBox。用户选择其他设备时，控制器先停止当前管线，更新当前索引，再重新检测模式并启动新管线。录像期间禁用设备切换，避免录制文件与采集状态不一致。

8. 错误处理

后端统一通过 errorMessage 属性和 error 信号向界面报告错误，QML 监听属性变化并显示 Toast。主要错误场景包括：

- 未发现摄像头设备。
- 摄像头无访问权限。
- 摄像头不支持 MJPG 或 YUYV。
- GStreamer 缺少 v4l2src、appsink、videoconvert 等插件。
- 采集管线启动失败或设备被其他程序占用。
- 没有可用的 H.264 编码器。
项目效果

应用启动后进入实时预览状态，界面显示摄像头名称、分辨率、像素格式和当前帧率。用户可以在同一界面完成拍照和录像，并通过底部控件重新扫描设备或切换摄像头。

[图片]

图：Camera Studio 主界面截图。中央区域显示实时画面，顶部展示采集状态，底部提供拍照、录像、摄像头选择和重扫功能。

工程目录与构建

核心工程结构如下：

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

编译和运行：

qmake camapp.pro
make
./camapp

主要依赖为 Qt 5.15 或兼容版本、C++11 编译器、GStreamer 1.0 开发包、qmake、make 和 pkg-config。录像至少需要安装一个可用的 H.264 编码器插件。
