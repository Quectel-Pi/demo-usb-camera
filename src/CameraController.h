#ifndef CAMERACONTROLLER_H
#define CAMERACONTROLLER_H
#include <QObject>
#include <QImage>
#include <QMutex>
#include <QStringList>
#include <QTimer>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include "CameraDevice.h"
#include "CameraRecorder.h"
class CameraController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(double fps READ fps NOTIFY fpsChanged)
    Q_PROPERTY(bool recording READ recording NOTIFY recordingChanged)
    Q_PROPERTY(int recordSeconds READ recordSeconds NOTIFY recordSecondsChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QString cameraName READ cameraName NOTIFY cameraInfoChanged)
    Q_PROPERTY(QString cameraDevice READ cameraDevice NOTIFY cameraInfoChanged)
    Q_PROPERTY(QStringList cameraList READ cameraList NOTIFY cameraListChanged)
    Q_PROPERTY(int currentCameraIndex READ currentCameraIndex NOTIFY currentCameraIndexChanged)
    Q_PROPERTY(int cameraWidth READ cameraWidth NOTIFY cameraInfoChanged)
    Q_PROPERTY(int cameraHeight READ cameraHeight NOTIFY cameraInfoChanged)
    Q_PROPERTY(int cameraFps READ cameraFps NOTIFY cameraInfoChanged)
    Q_PROPERTY(QString cameraPixelFormat READ cameraPixelFormat NOTIFY cameraInfoChanged)
public:
    explicit CameraController(QObject *parent = nullptr);
    ~CameraController();
    bool running() const { return m_running; }
    double fps() const { return m_fps; }
    bool recording() const { return m_recorder.isRecording(); }
    int recordSeconds() const { return m_recordSeconds; }
    QString errorMessage() const { return m_errorMessage; }
    QString cameraName() const { return m_camera.cardName; }
    QString cameraDevice() const { return m_camera.stablePath.isEmpty() ? m_camera.devicePath : m_camera.stablePath; }
    QStringList cameraList() const;
    int currentCameraIndex() const { return m_currentCameraIndex; }
    int cameraWidth() const { return m_mode.width; }
    int cameraHeight() const { return m_mode.height; }
    int cameraFps() const { return m_mode.fps; }
    QString cameraPixelFormat() const { return m_mode.pixelFormat; }
    Q_INVOKABLE bool start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE bool takePhoto();
    Q_INVOKABLE bool startRecording();
    Q_INVOKABLE void stopRecording();
    Q_INVOKABLE bool rescanCamera();
    Q_INVOKABLE bool switchCamera(int index);
signals:
    void frameReady(const QImage &);
    void runningChanged();
    void fpsChanged();
    void recordingChanged();
    void recordSecondsChanged();
    void errorMessageChanged();
    void cameraInfoChanged();
    void cameraListChanged();
    void currentCameraIndexChanged();
    void error(const QString &);

private:
    static GstFlowReturn onPreviewSample(GstAppSink *, gpointer);
    static GstFlowReturn onJpegSample(GstAppSink *, gpointer);
    GstFlowReturn processPreviewSample(GstAppSink *);
    GstFlowReturn processJpegSample(GstAppSink *);
    bool detectCamera();
    bool createMainPipeline();
    void destroyMainPipeline();
    void setError(const QString &);
    QString makePhotoPath() const;
    QString makeVideoPath() const;
    GstElement *m_pipeline = nullptr, *m_previewSink = nullptr, *m_jpegSink = nullptr;
    bool m_running = false;
    double m_fps = 0;
    QString m_errorMessage;
    CameraDevice m_camera;
    QVector<CameraDevice> m_cameras;
    int m_currentCameraIndex = 0;
    CameraMode m_mode;
    QByteArray m_latestJpeg;
    QImage m_latestFrame;
    QMutex m_frameMutex;
    QTimer m_recordTimer;
    int m_recordSeconds = 0;
    qint64 m_fpsStartMs = 0;
    int m_fpsFrames = 0;
    CameraRecorder m_recorder;
};
#endif
