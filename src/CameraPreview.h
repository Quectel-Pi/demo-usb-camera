#ifndef CAMERAPREVIEW_H
#define CAMERAPREVIEW_H
#include <QQuickPaintedItem>
#include <QImage>
#include <QMutex>
class CameraPreview : public QQuickPaintedItem
{
Q_OBJECT public : explicit CameraPreview(QQuickItem *p = nullptr);
public slots:
    void setFrame(const QImage &);

protected:
    void paint(QPainter *) override;

private:
    QImage m_frame;
    QMutex m_mutex;
};
#endif
