#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QMetaType>
#include <QDebug>
#include "CameraController.h"
#include "CameraPreview.h"
int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    qRegisterMetaType<QImage>("QImage");
    qmlRegisterType<CameraPreview>("Camera", 1, 0, "CameraPreview");
    CameraController camera;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("camera", &camera);
    engine.load(QUrl(QStringLiteral("qrc:/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;
    CameraPreview *preview = engine.rootObjects().first()->findChild<CameraPreview *>("preview");
    if (!preview)
        return -1;
    QObject::connect(&camera, &CameraController::frameReady, preview, &CameraPreview::setFrame, Qt::QueuedConnection);
    QObject::connect(&camera, &CameraController::error, [](const QString &s)
                     { qWarning() << "CAMERA ERROR:" << s; });
    if (!camera.start())
        qWarning() << camera.errorMessage();
    return app.exec();
}
