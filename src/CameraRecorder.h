#ifndef CAMERARECORDER_H
#define CAMERARECORDER_H
#include <QString>
#include <QImage>
#include <QMutex>
#include <QStringList>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
class CameraRecorder
{
public:
    CameraRecorder();
    ~CameraRecorder();
    bool start(const QString &path, int width, int height, int fps, QString *error);
    void pushFrame(const QImage &image);
    void stop();
    bool isRecording() const { return m_recording; }

private:
    struct EncoderConfig
    {
        QString name;
        QString pipeline;
    };
    static EncoderConfig selectEncoder(QString *error);
    static bool hasElement(const char *name);
    GstElement *m_pipeline = nullptr, *m_source = nullptr;
    bool m_recording = false;
    int m_width = 0, m_height = 0, m_fps = 30;
    quint64 m_frameCount = 0;
    QMutex m_mutex;
};
#endif
