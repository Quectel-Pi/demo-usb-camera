# USB Camera

[English](README.md)

USB Camera 是一个基于 Qt Quick/QML 和 C++ 的 Linux 桌面相机应用，支持实时预览、拍照、录像、多摄像头选择、设备重扫和错误提示。

## 功能

- 实时摄像头预览
- 拍照保存为 JPG
- 录像保存为 MP4
- 多摄像头下拉选择
- 重新扫描摄像头
- 缺少权限、缺少插件、编码器不可用等错误提示
- 自动选择最佳采集模式
- 可配置 H.264 录像编码器

## 技术栈

- 前端：Qt Quick / QML / Qt Quick Controls 2
- 后端：C++11 / Qt / GStreamer 1.0 / Linux V4L2
- 构建：qmake / make / pkg-config

详细架构请见 [ARCHITECTURE.md](ARCHITECTURE.md)。

## 硬件要求

- Linux 设备
- 一个或多个 V4L2 可识别的 USB UVC 摄像头
- 摄像头至少支持 `MJPG` 或 `YUYV`
- 录像需要系统中至少有一个可用 H.264 编码器插件

推荐摄像头支持 MJPEG 和 30 FPS 或更高帧率。

## 依赖

需要安装：

- Qt 5.15 或兼容版本
- Qt GUI / Qt QML / Qt Quick / Qt Quick Controls 2
- GStreamer 1.0 开发包
- GStreamer app/video 开发包
- GStreamer 插件：`v4l2src`、`appsink`、`appsrc`、`jpegdec`、`videoconvert`、`h264parse`、`mp4mux`
- 至少一个 H.264 编码器：`mpph264enc`、`x264enc`、`openh264enc` 或 `avenc_h264`
- C++ 编译器、qmake、make、pkg-config

Ubuntu/Debian 上的包名会随发行版不同而变化，通常需要 Qt 开发包、GStreamer base/good/bad/ugly/libav 插件集合以及对应开发包。

## 构建

```bash
qmake camapp.pro
make
```

## 运行

```bash
./camapp
```

如果摄像头无权限，先确认当前用户是否在 `video` 用户组中：

```bash
groups
```

可以将当前用户加入 `video` 用户组，重新登录后生效：

```bash
sudo usermod -aG video "$USER"
```

## 录像编码器配置

应用默认自动选择第一个可用编码器：

```text
mpph264enc -> x264enc -> openh264enc -> avenc_h264
```

也可以通过环境变量指定：

```bash
CAMAPP_VIDEO_ENCODER=x264enc ./camapp
```

支持的值：

```text
mpph264enc
x264enc
openh264enc
avenc_h264
```

如果指定编码器不可用，应用会提示安装对应 GStreamer 插件或取消 `CAMAPP_VIDEO_ENCODER`。

## 检查环境

检查摄像头设备：

```bash
ls /dev/video*
```

检查 GStreamer 插件：

```bash
gst-inspect-1.0 v4l2src
gst-inspect-1.0 jpegdec
gst-inspect-1.0 x264enc
gst-inspect-1.0 openh264enc
gst-inspect-1.0 avenc_h264
gst-inspect-1.0 mpph264enc
```

只要至少一个 H.264 编码器存在，录像功能即可尝试启动。

## 输出文件

照片默认保存到：

```text
~/Pictures/Camera/
```

视频默认保存到：

```text
~/Videos/Camera/
```

文件名格式：

```text
IMG_yyyyMMdd_HHmmss_zzz.jpg
VID_yyyyMMdd_HHmmss.mp4
```

## 常见问题

### 没有摄像头画面

确认摄像头已连接，并且系统能看到 `/dev/video*`。如果应用提示权限错误，请将当前用户加入 `video` 用户组或调整设备权限。

### 应用提示缺少 GStreamer 插件

根据错误信息安装对应插件。预览通常需要 `v4l2src`、`jpegdec`、`videoconvert`、`appsink`，录像还需要 `appsrc`、H.264 编码器、`h264parse` 和 `mp4mux`。

### 录像启动失败

先检查是否存在可用编码器：

```bash
gst-inspect-1.0 x264enc
gst-inspect-1.0 openh264enc
gst-inspect-1.0 avenc_h264
gst-inspect-1.0 mpph264enc
```

然后可以显式指定一个已安装编码器：

```bash
CAMAPP_VIDEO_ENCODER=x264enc ./camapp
```

### 多个摄像头时如何切换

应用底部提供摄像头下拉框。录像过程中会禁用切换，停止录像后可重新选择摄像头。

## 开源前清理

请不要提交构建产物，例如：

- `camapp`
- `Makefile`
- `.qmake.stash`
- `*.o`
- `moc_*`
- `qrc_*.cpp`

这些已在 `.gitignore` 中忽略。

