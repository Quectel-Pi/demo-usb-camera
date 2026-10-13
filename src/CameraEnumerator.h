#ifndef CAMERAENUMERATOR_H
#define CAMERAENUMERATOR_H
#include "CameraDevice.h"
class CameraEnumerator
{
public:
    static QVector<CameraDevice> enumerate();
    static CameraMode selectBestMode(const CameraDevice &device);

private:
    static bool probe(const QString &path, const QString &stablePath, CameraDevice &out);
    static bool enumerateModes(int fd, CameraDevice &out);
};
#endif
