#ifndef CAMERADEVICE_H
#define CAMERADEVICE_H
#include <QString>
#include <QVector>
struct CameraMode
{
    QString pixelFormat;
    int width = 0, height = 0, fps = 0;
    bool isMjpeg() const { return pixelFormat == "MJPG"; }
    QString displayName() const { return QString("%1x%2 @ %3 FPS (%4)").arg(width).arg(height).arg(fps).arg(pixelFormat); }
};
struct CameraDevice
{
    QString devicePath, stablePath, cardName, driverName, serial;
    QVector<CameraMode> modes;
    bool isValid() const { return !devicePath.isEmpty() && !modes.isEmpty(); }
};
#endif
