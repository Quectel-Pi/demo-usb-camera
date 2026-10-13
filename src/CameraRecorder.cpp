#include "CameraRecorder.h"
#include <QMutexLocker>
#include <QProcessEnvironment>
#include <cstring>
CameraRecorder::CameraRecorder() {}
CameraRecorder::~CameraRecorder() { stop(); }
bool CameraRecorder::hasElement(const char *name)
{
    GstElementFactory *factory = gst_element_factory_find(name);
    if (!factory)
        return false;
    gst_object_unref(factory);
    return true;
}
CameraRecorder::EncoderConfig CameraRecorder::selectEncoder(QString *error)
{
    QString requested = QProcessEnvironment::systemEnvironment().value("CAMAPP_VIDEO_ENCODER").trimmed();
    QStringList candidates;
    if (!requested.isEmpty())
        candidates << requested;
    else
        candidates << "mpph264enc" << "x264enc" << "openh264enc" << "avenc_h264";

    for (const QString &name : candidates)
    {
        if (name == "mpph264enc" && hasElement("mpph264enc"))
            return {name, "mpph264enc bps=4000000 gop=%1"};
        if (name == "x264enc" && hasElement("x264enc"))
            return {name, "x264enc bitrate=4000 key-int-max=%1 speed-preset=ultrafast tune=zerolatency"};
        if (name == "openh264enc" && hasElement("openh264enc"))
            return {name, "openh264enc bitrate=4000000 gop-size=%1"};
        if (name == "avenc_h264" && hasElement("avenc_h264"))
            return {name, "avenc_h264 bitrate=4000000 gop-size=%1"};
    }

    if (error)
    {
        if (!requested.isEmpty())
            *error = "Configured encoder is unavailable: " + requested + ". Install the matching GStreamer plugin or unset CAMAPP_VIDEO_ENCODER.";
        else
            *error = "No H.264 encoder plugin found. Install mpph264enc, x264enc, openh264enc, or avenc_h264.";
    }
    return {};
}
bool CameraRecorder::start(const QString &path, int w, int h, int fps, QString *error)
{
    stop();
    EncoderConfig encoder = selectEncoder(error);
    if (encoder.pipeline.isEmpty())
        return false;
    QString safe = path;
    safe.replace("\\", "\\\\").replace("\"", "\\\"");
    QString desc = QString("appsrc name=recordSource is-live=true format=time block=false do-timestamp=false "
                           "caps=video/x-raw,format=BGRx,width=%1,height=%2,framerate=%3/1 "
                           "! queue max-size-buffers=8 leaky=downstream ! videoconvert ! video/x-raw,format=NV12 "
                           "! %4 ! h264parse ! mp4mux ! filesink location=\"%5\"")
                       .arg(w)
                       .arg(h)
                       .arg(fps)
                       .arg(encoder.pipeline.arg(qMax(1, fps)))
                       .arg(safe);
    GError *err = nullptr;
    GstElement *pipe = gst_parse_launch(desc.toUtf8().constData(), &err);
    if (!pipe)
    {
        if (error)
            *error = err ? QString("Failed to create recording pipeline with %1: %2").arg(encoder.name, QString::fromUtf8(err->message))
                         : QString("Failed to create recording pipeline with %1").arg(encoder.name);
        if (err)
            g_error_free(err);
        return false;
    }
    GstElement *src = gst_bin_get_by_name(GST_BIN(pipe), "recordSource");
    if (!src)
    {
        gst_object_unref(pipe);
        if (error)
            *error = "recordSource not found";
        return false;
    }
    if (gst_element_set_state(pipe, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE)
    {
        gst_object_unref(src);
        gst_element_set_state(pipe, GST_STATE_NULL);
        gst_object_unref(pipe);
        if (error)
            *error = "Failed to start recording pipeline with " + encoder.name + ". Check encoder availability and caps support.";
        return false;
    }
    QMutexLocker lock(&m_mutex);
    m_pipeline = pipe;
    m_source = src;
    m_width = w;
    m_height = h;
    m_fps = qMax(1, fps);
    m_frameCount = 0;
    m_recording = true;
    return true;
}
void CameraRecorder::pushFrame(const QImage &input)
{
    QMutexLocker lock(&m_mutex);
    if (!m_recording || !m_source)
        return;
    QImage img = input.convertToFormat(QImage::Format_RGB32);
    if (img.width() != m_width || img.height() != m_height)
        return;
    const gsize size = gsize(img.bytesPerLine()) * img.height();
    GstBuffer *b = gst_buffer_new_allocate(nullptr, size, nullptr);
    GstMapInfo map{};
    if (!gst_buffer_map(b, &map, GST_MAP_WRITE))
    {
        gst_buffer_unref(b);
        return;
    }
    std::memcpy(map.data, img.constBits(), size);
    gst_buffer_unmap(b, &map);
    GstClockTime dur = gst_util_uint64_scale_int(1, GST_SECOND, m_fps);
    GST_BUFFER_PTS(b) = m_frameCount * dur;
    GST_BUFFER_DTS(b) = GST_CLOCK_TIME_NONE;
    GST_BUFFER_DURATION(b) = dur;
    ++m_frameCount;
    GstFlowReturn r = gst_app_src_push_buffer(GST_APP_SRC(m_source), b);
    if (r != GST_FLOW_OK)
    {
    }
}
void CameraRecorder::stop()
{
    GstElement *pipe = nullptr, *src = nullptr;
    {
        QMutexLocker lock(&m_mutex);
        if (!m_pipeline)
        {
            m_recording = false;
            return;
        }
        m_recording = false;
        pipe = GST_ELEMENT(gst_object_ref(m_pipeline));
        src = m_source ? GST_ELEMENT(gst_object_ref(m_source)) : nullptr;
    }
    if (src)
    {
        gst_app_src_end_of_stream(GST_APP_SRC(src));
        gst_object_unref(src);
    }
    GstBus *bus = gst_element_get_bus(pipe);
    GstMessage *msg = gst_bus_timed_pop_filtered(bus, 5 * GST_SECOND, GstMessageType(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    if (msg)
        gst_message_unref(msg);
    gst_object_unref(bus);
    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(pipe);
    QMutexLocker lock(&m_mutex);
    if (m_source)
    {
        gst_object_unref(m_source);
        m_source = nullptr;
    }
    if (m_pipeline)
    {
        gst_element_set_state(m_pipeline, GST_STATE_NULL);
        gst_object_unref(m_pipeline);
        m_pipeline = nullptr;
    }
    m_frameCount = 0;
}
