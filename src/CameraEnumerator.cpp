#include "CameraEnumerator.h"
#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <QtMath>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

bool CameraEnumerator::enumerateModes(int fd, CameraDevice &out)
{
    v4l2_fmtdesc fmt{};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    for (fmt.index = 0; ioctl(fd, VIDIOC_ENUM_FMT, &fmt) == 0; ++fmt.index)
    {
        char cc[5] = {char(fmt.pixelformat & 0xff), char((fmt.pixelformat >> 8) & 0xff), char((fmt.pixelformat >> 16) & 0xff), char((fmt.pixelformat >> 24) & 0xff), 0};
        QString pf = QString::fromLatin1(cc);
        if (pf != "MJPG" && pf != "YUYV")
            continue;
        v4l2_frmsizeenum sz{};
        sz.pixel_format = fmt.pixelformat;
        for (sz.index = 0; ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &sz) == 0; ++sz.index)
        {
            if (sz.type != V4L2_FRMSIZE_TYPE_DISCRETE)
                continue;
            v4l2_frmivalenum iv{};
            iv.pixel_format = fmt.pixelformat;
            iv.width = sz.discrete.width;
            iv.height = sz.discrete.height;
            bool any = false;
            for (iv.index = 0; ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &iv) == 0; ++iv.index)
            {
                if (iv.type != V4L2_FRMIVAL_TYPE_DISCRETE || !iv.discrete.numerator)
                    continue;
                CameraMode m;
                m.pixelFormat = pf;
                m.width = int(sz.discrete.width);
                m.height = int(sz.discrete.height);
                m.fps = qRound(double(iv.discrete.denominator) / double(iv.discrete.numerator));
                if (m.fps > 0)
                    out.modes.append(m);
                any = true;
            }
            if (!any)
            {
                CameraMode m;
                m.pixelFormat = pf;
                m.width = int(sz.discrete.width);
                m.height = int(sz.discrete.height);
                m.fps = 30;
                out.modes.append(m);
            }
        }
    }
    return !out.modes.isEmpty();
}

bool CameraEnumerator::probe(const QString &path, const QString &stablePath, CameraDevice &out)
{
    QByteArray p = path.toLocal8Bit();
    int fd = open(p.constData(), O_RDWR | O_NONBLOCK);
    if (fd < 0)
        return false;
    v4l2_capability cap{};
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0)
    {
        close(fd);
        return false;
    }
    quint32 caps = (cap.capabilities & V4L2_CAP_DEVICE_CAPS) ? cap.device_caps : cap.capabilities;
    if (!(caps & V4L2_CAP_VIDEO_CAPTURE) || !(caps & V4L2_CAP_STREAMING))
    {
        close(fd);
        return false;
    }
    out.devicePath = path;
    out.stablePath = stablePath;
    out.cardName = QString::fromLocal8Bit(reinterpret_cast<const char *>(cap.card));
    out.driverName = QString::fromLocal8Bit(reinterpret_cast<const char *>(cap.driver));
    bool ok = enumerateModes(fd, out);
    close(fd);
    return ok;
}

QVector<CameraDevice> CameraEnumerator::enumerate()
{
    QVector<CameraDevice> result;
    QSet<QString> seen;
    QDir byId("/dev/v4l/by-id");
    for (const QString &e : byId.entryList(QStringList() << "*video-index0", QDir::Files | QDir::System | QDir::NoDotAndDotDot, QDir::Name))
    {
        QString stable = byId.absoluteFilePath(e), real = QFileInfo(stable).canonicalFilePath();
        if (real.isEmpty() || seen.contains(real))
            continue;
        CameraDevice d;
        if (probe(real, stable, d))
        {
            result.append(d);
            seen.insert(real);
        }
    }
    QDir dev("/dev");
    for (const QString &e : dev.entryList(QStringList() << "video*", QDir::System | QDir::Files, QDir::Name))
    {
        QString real = dev.absoluteFilePath(e);
        if (seen.contains(real))
            continue;
        CameraDevice d;
        if (probe(real, QString(), d) && d.driverName == "uvcvideo")
        {
            result.append(d);
            seen.insert(real);
        }
    }
    return result;
}

CameraMode CameraEnumerator::selectBestMode(const CameraDevice &d)
{
    CameraMode best;
    qint64 bestScore = -1;
    for (const CameraMode &m : d.modes)
    {
        qint64 score = 0;
        if (m.pixelFormat == "MJPG")
            score += 1000000000000LL;
        if (m.fps >= 30)
            score += 500000000000LL;
        score += qint64(qMin(m.fps, 60)) * 1000000000LL;
        score += qint64(m.width) * m.height;
        if (score > bestScore)
        {
            bestScore = score;
            best = m;
        }
    }
    return best;
}
