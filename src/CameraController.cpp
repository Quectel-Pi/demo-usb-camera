#include "CameraController.h"
#include "CameraEnumerator.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QStringList>
#include <QDebug>

namespace
{
bool hasGstElement(const char *name)
{
    GstElementFactory *factory = gst_element_factory_find(name);
    if (!factory)
        return false;
    gst_object_unref(factory);
    return true;
}

QString missingElementMessage(const QStringList &elements)
{
    QStringList missing;
    for (const QString &element : elements)
    {
        if (!hasGstElement(element.toLatin1().constData()))
            missing << element;
    }
    if (missing.isEmpty())
        return QString();
    return "Missing GStreamer plugin(s): " + missing.join(", ");
}
}
CameraController::CameraController(QObject *p) : QObject(p)
{
    gst_init(nullptr, nullptr);
    m_recordTimer.setInterval(1000);
    connect(&m_recordTimer, &QTimer::timeout, this, [this]
            {if(recording()){++m_recordSeconds;emit recordSecondsChanged();} });
}
CameraController::~CameraController() { stop(); }
void CameraController::setError(const QString &s)
{
    m_errorMessage = s;
    emit errorMessageChanged();
    emit error(s);
}
QString CameraController::makePhotoPath() const
{
    QString d = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    if (d.isEmpty())
        d = QDir::homePath() + "/Pictures";
    d += "/Camera";
    QDir().mkpath(d);
    return d + "/IMG_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz") + ".jpg";
}
QString CameraController::makeVideoPath() const
{
    QString d = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    if (d.isEmpty())
        d = QDir::homePath() + "/Videos";
    d += "/Camera";
    QDir().mkpath(d);
    return d + "/VID_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".mp4";
}
QStringList CameraController::cameraList() const
{
    QStringList names;
    for (int i = 0; i < m_cameras.size(); ++i)
    {
        const CameraDevice &device = m_cameras.at(i);
        QString path = device.devicePath.isEmpty() ? device.stablePath : device.devicePath;
        names << QString("Camera %1  %2").arg(i + 1).arg(path);
    }
    return names;
}
bool CameraController::detectCamera()
{
    m_cameras = CameraEnumerator::enumerate();
    emit cameraListChanged();
    if (m_cameras.isEmpty())
    {
        QDir dev("/dev");
        QStringList videoNodes = dev.entryList(QStringList() << "video*", QDir::System | QDir::Files, QDir::Name);
        if (videoNodes.isEmpty())
        {
            setError("No camera device found. Connect a V4L2/UVC camera.");
            return false;
        }
        for (const QString &node : videoNodes)
        {
            QString path = dev.absoluteFilePath(node);
            QFileInfo info(path);
            if (info.exists() && !info.isReadable())
            {
                setError("Camera permission denied: " + path + ". Add the user to the video group or adjust device permissions.");
                return false;
            }
        }
        setError("No supported USB camera found. The camera must support MJPG or YUYV via V4L2/UVC.");
        return false;
    }
    if (m_currentCameraIndex < 0 || m_currentCameraIndex >= m_cameras.size())
    {
        m_currentCameraIndex = 0;
        emit currentCameraIndexChanged();
    }
    m_camera = m_cameras.at(m_currentCameraIndex);
    m_mode = CameraEnumerator::selectBestMode(m_camera);
    if (m_mode.width <= 0 || m_mode.height <= 0 || m_mode.fps <= 0)
    {
        setError("No supported camera mode found");
        return false;
    }
    qInfo() << "Camera:" << m_camera.cardName << cameraDevice() << m_mode.displayName();
    emit cameraInfoChanged();
    return true;
}
bool CameraController::createMainPipeline()
{
    QString dev = cameraDevice(), desc;
    QStringList requiredElements;
    requiredElements << "v4l2src" << "videoconvert" << "appsink";
    if (m_mode.pixelFormat == "MJPG")
        requiredElements << "jpegdec";
    QString missing = missingElementMessage(requiredElements);
    if (!missing.isEmpty())
    {
        setError(missing);
        return false;
    }
    if (m_mode.pixelFormat == "MJPG")
        desc = QString("v4l2src device=\"%1\" ! image/jpeg,width=%2,height=%3,framerate=%4/1 ! tee name=t "
                       "t. ! queue leaky=downstream max-size-buffers=2 ! jpegdec ! videoconvert ! video/x-raw,format=BGRx,width=%2,height=%3,framerate=%4/1 ! appsink name=previewSink "
                       "t. ! queue leaky=downstream max-size-buffers=1 ! appsink name=jpegSink")
                   .arg(dev)
                   .arg(m_mode.width)
                   .arg(m_mode.height)
                   .arg(m_mode.fps);
    else
        desc = QString("v4l2src device=\"%1\" ! video/x-raw,format=YUY2,width=%2,height=%3,framerate=%4/1 ! videoconvert ! video/x-raw,format=BGRx,width=%2,height=%3,framerate=%4/1 ! appsink name=previewSink").arg(dev).arg(m_mode.width).arg(m_mode.height).arg(m_mode.fps);
    GError *err = nullptr;
    m_pipeline = gst_parse_launch(desc.toUtf8().constData(), &err);
    if (!m_pipeline)
    {
        setError(err ? QString::fromUtf8(err->message) : "Failed to create camera pipeline");
        if (err)
            g_error_free(err);
        return false;
    }
    m_previewSink = gst_bin_get_by_name(GST_BIN(m_pipeline), "previewSink");
    m_jpegSink = gst_bin_get_by_name(GST_BIN(m_pipeline), "jpegSink");
    if (!m_previewSink)
    {
        setError("previewSink not found");
        destroyMainPipeline();
        return false;
    }
    gst_app_sink_set_emit_signals(GST_APP_SINK(m_previewSink), FALSE);
    gst_app_sink_set_drop(GST_APP_SINK(m_previewSink), TRUE);
    gst_app_sink_set_max_buffers(GST_APP_SINK(m_previewSink), 2);
    GstAppSinkCallbacks pc{};
    pc.new_sample = &CameraController::onPreviewSample;
    gst_app_sink_set_callbacks(GST_APP_SINK(m_previewSink), &pc, this, nullptr);
    if (m_jpegSink)
    {
        gst_app_sink_set_emit_signals(GST_APP_SINK(m_jpegSink), FALSE);
        gst_app_sink_set_drop(GST_APP_SINK(m_jpegSink), TRUE);
        gst_app_sink_set_max_buffers(GST_APP_SINK(m_jpegSink), 1);
        GstAppSinkCallbacks jc{};
        jc.new_sample = &CameraController::onJpegSample;
        gst_app_sink_set_callbacks(GST_APP_SINK(m_jpegSink), &jc, this, nullptr);
    }
    if (gst_element_set_state(m_pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE)
    {
        setError("Failed to start camera pipeline. Check camera permissions, supported mode, and whether another process is using the camera.");
        destroyMainPipeline();
        return false;
    }
    return true;
}
bool CameraController::start()
{
    if (m_running)
        return true;
    if (!detectCamera() || !createMainPipeline())
        return false;
    m_running = true;
    m_fpsStartMs = QDateTime::currentMSecsSinceEpoch();
    m_fpsFrames = 0;
    emit runningChanged();
    return true;
}
void CameraController::destroyMainPipeline()
{
    if (m_previewSink)
    {
        gst_object_unref(m_previewSink);
        m_previewSink = nullptr;
    }
    if (m_jpegSink)
    {
        gst_object_unref(m_jpegSink);
        m_jpegSink = nullptr;
    }
    if (m_pipeline)
    {
        gst_element_set_state(m_pipeline, GST_STATE_NULL);
        gst_object_unref(m_pipeline);
        m_pipeline = nullptr;
    }
}
void CameraController::stop()
{
    stopRecording();
    destroyMainPipeline();
    if (m_running)
    {
        m_running = false;
        emit runningChanged();
    }
    m_fps = 0;
    emit fpsChanged();
}
bool CameraController::rescanCamera()
{
    stop();
    return start();
}
bool CameraController::switchCamera(int index)
{
    if (index < 0)
        return false;
    if (index == m_currentCameraIndex && m_running)
        return true;
    m_currentCameraIndex = index;
    emit currentCameraIndexChanged();
    stop();
    return start();
}
GstFlowReturn CameraController::onPreviewSample(GstAppSink *s, gpointer u) { return static_cast<CameraController *>(u)->processPreviewSample(s); }
GstFlowReturn CameraController::onJpegSample(GstAppSink *s, gpointer u) { return static_cast<CameraController *>(u)->processJpegSample(s); }
GstFlowReturn CameraController::processPreviewSample(GstAppSink *s)
{
    GstSample *sample = gst_app_sink_pull_sample(s);
    if (!sample)
        return GST_FLOW_ERROR;
    GstBuffer *b = gst_sample_get_buffer(sample);
    GstCaps *c = gst_sample_get_caps(sample);
    GstStructure *st = c ? gst_caps_get_structure(c, 0) : nullptr;
    int w = 0, h = 0;
    if (!st || !gst_structure_get_int(st, "width", &w) || !gst_structure_get_int(st, "height", &h))
    {
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }
    GstMapInfo map{};
    if (!gst_buffer_map(b, &map, GST_MAP_READ))
    {
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }
    int stride = h > 0 ? int(map.size / gsize(h)) : w * 4;
    QImage wrapped(map.data, w, h, stride, QImage::Format_RGB32);
    QImage image = wrapped.copy();
    gst_buffer_unmap(b, &map);
    gst_sample_unref(sample);
    {
        QMutexLocker l(&m_frameMutex);
        m_latestFrame = image;
    }
    emit frameReady(image);
    if (recording())
        m_recorder.pushFrame(image);
    ++m_fpsFrames;
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    qint64 dt = now - m_fpsStartMs;
    if (dt >= 1000)
    {
        m_fps = double(m_fpsFrames) * 1000.0 / double(dt);
        m_fpsFrames = 0;
        m_fpsStartMs = now;
        emit fpsChanged();
    }
    return GST_FLOW_OK;
}
GstFlowReturn CameraController::processJpegSample(GstAppSink *s)
{
    GstSample *sample = gst_app_sink_pull_sample(s);
    if (!sample)
        return GST_FLOW_ERROR;
    GstBuffer *b = gst_sample_get_buffer(sample);
    GstMapInfo map{};
    if (gst_buffer_map(b, &map, GST_MAP_READ))
    {
        QMutexLocker l(&m_frameMutex);
        m_latestJpeg = QByteArray(reinterpret_cast<const char *>(map.data), int(map.size));
        gst_buffer_unmap(b, &map);
    }
    gst_sample_unref(sample);
    return GST_FLOW_OK;
}
bool CameraController::takePhoto()
{
    QString path = makePhotoPath();
    QByteArray jpeg;
    QImage frame;
    {
        QMutexLocker l(&m_frameMutex);
        jpeg = m_latestJpeg;
        frame = m_latestFrame;
    }
    if (m_mode.isMjpeg() && !jpeg.isEmpty())
    {
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly) || f.write(jpeg) != jpeg.size())
        {
            setError("Failed to save photo: " + path);
            return false;
        }
        return true;
    }
    if (frame.isNull() || !frame.save(path, "JPG", 95))
    {
        setError("Failed to save photo: " + path);
        return false;
    }
    return true;
}
bool CameraController::startRecording()
{
    if (recording())
        return true;
    if (!m_running)
    {
        setError("Camera is not running");
        return false;
    }
    QString err;
    if (!m_recorder.start(makeVideoPath(), m_mode.width, m_mode.height, m_mode.fps, &err))
    {
        setError(err);
        return false;
    }
    m_recordSeconds = 0;
    emit recordSecondsChanged();
    m_recordTimer.start();
    emit recordingChanged();
    return true;
}
void CameraController::stopRecording()
{
    if (!recording())
        return;
    m_recordTimer.stop();
    m_recorder.stop();
    emit recordingChanged();
}
