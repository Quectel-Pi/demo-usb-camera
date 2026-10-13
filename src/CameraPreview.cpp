#include "CameraPreview.h"
#include <QPainter>
#include <QMutexLocker>
CameraPreview::CameraPreview(QQuickItem *p) : QQuickPaintedItem(p) { setAntialiasing(false); }
void CameraPreview::setFrame(const QImage &i)
{
    {
        QMutexLocker l(&m_mutex);
        m_frame = i;
    }
    update();
}
void CameraPreview::paint(QPainter *p)
{
    QImage f;
    {
        QMutexLocker l(&m_mutex);
        f = m_frame;
    }
    if (f.isNull())
        return;
    QRectF t = boundingRect();
    QSizeF s = f.size();
    s.scale(t.size(), Qt::KeepAspectRatio);
    QRectF d(t.center().x() - s.width() / 2, t.center().y() - s.height() / 2, s.width(), s.height());
    p->setRenderHint(QPainter::SmoothPixmapTransform, true);
    p->drawImage(d, f);
}
