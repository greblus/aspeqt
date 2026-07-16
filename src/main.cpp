#include <QApplication>
#include <QTextCodec>
#include <QLibraryInfo>
#include <QStyleHints>
#include "mainwindow.h"

#ifdef ASPEQT_QML
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "qmlbridge.h"
#endif

#ifdef Q_OS_WIN
#include <windows.h>
#include <Mmsystem.h>
#endif

#ifdef Q_OS_ANDROID
#include <QJniObject>
#include <jni.h>

jbyte *jrbuf = NULL;
char *rbuf;
jbyte *jwbuf = NULL;
char *wbuf;

extern "C" {
    JNIEXPORT void JNICALL
    Java_net_greblus_SerialActivity_sendBufAddr(JNIEnv *env/*env*/,
    jobject /*obj*/, jobject rbf, jobject wbf)
        {
            jrbuf = (jbyte *)env->GetDirectBufferAddress(rbf);
            rbuf = reinterpret_cast<char *>(jrbuf);
            jwbuf = (jbyte *)env->GetDirectBufferAddress(wbf);
            wbuf = reinterpret_cast<char *>(jwbuf);
        }
}
#endif

int main(int argc, char *argv[])
{
    int ret;
#ifdef Q_OS_WIN
    timeBeginPeriod(1);
#endif
#ifdef Q_OS_ANDROID
    // Qt 6.11 on Android crashes (qFatal in makeCurrent/QRhi::create) when a
    // modal dialog forces the widget backing store onto the RHI/OpenGL path and
    // the Android surface is momentarily gone. Force the raster backing store.
    qputenv("QT_WIDGETS_RHI", "0");
#endif
    QApplication a(argc, argv);
    // The UI (grey icons/text) was designed for a light background; force a light
    // colour scheme so it doesn't render as a black void under the system dark mode.
    a.styleHints()->setColorScheme(Qt::ColorScheme::Light);
#ifdef ASPEQT_QML
    // QML UI spike (branch `qml`): reproduce the main-window layout in Qt Quick.
    qputenv("QT_QUICK_CONTROLS_STYLE", "Material");
    QQmlApplicationEngine engine;
    AppController controller;
    engine.rootContext()->setContextProperty("app", &controller);
    engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;
    ret = a.exec();
#else
    MainWindow w;
    w.show();
    ret = a.exec();
#endif
#ifdef Q_OS_WIN
    timeEndPeriod(1);
#endif
    return ret;
}
