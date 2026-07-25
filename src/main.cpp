#include <QGuiApplication>
#include <QLibraryInfo>
#include <QStyleHints>
#include "engine.h"

extern Engine *g_engine;

#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "qmlbridge.h"
#include "networkbrowser.h"
#include "paperprovider.h"

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

    // Result of a SAF pick. Arrives on Android's UI thread, so hand it to the
    // engine through a queued call. The URI is Android's own string: it must
    // not be re-encoded on the way (see Engine::documentPicked).
    JNIEXPORT void JNICALL
    Java_net_greblus_SerialActivity_documentPicked(JNIEnv *env,
    jclass /*cls*/, jint reqId, jstring juri)
        {
            const char *chars = juri ? env->GetStringUTFChars(juri, nullptr) : nullptr;
            const QString uri = chars ? QString::fromUtf8(chars) : QString();
            if (chars)
                env->ReleaseStringUTFChars(juri, chars);
            if (!g_engine)
                return;
            QMetaObject::invokeMethod(g_engine, "onDocumentPicked", Qt::QueuedConnection,
                                      Q_ARG(int, (int)reqId), Q_ARG(QString, uri));
        }

    // Cable plugged in while the app was already running. Arrives on Android's
    // UI thread, so hand it to the engine through a queued call.
    JNIEXPORT void JNICALL
    Java_net_greblus_SerialActivity_usbAttached(JNIEnv * /*env*/, jclass /*cls*/)
        {
            if (!g_engine)
                return;
            QMetaObject::invokeMethod(g_engine, "onUsbAttached", Qt::QueuedConnection);
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
    QGuiApplication a(argc, argv);
    // The UI (grey icons/text) was designed for a light background; force a light
    // colour scheme so it doesn't render as a black void under the system dark mode.
    a.styleHints()->setColorScheme(Qt::ColorScheme::Light);
    // QML UI (branch `qml`): Engine runs headless as the emulation engine
    // (never shown); the Qt Quick front-end drives it via AppController.
    qputenv("QT_QUICK_CONTROLS_STYLE", "Material");
    Engine engineWindow;              // the engine: SIO, mounting, log
    QQmlApplicationEngine engine;
    AppController controller(&engineWindow);
    NetworkBrowser netBrowser;
    // The printer's page is served to QML through an image provider; the engine
    // owns the QImage, the controller keeps the provider fed.
    PaperProvider *paperProvider = new PaperProvider;   // engine takes ownership
    engine.addImageProvider(QStringLiteral("paper"), paperProvider);
    controller.setPaperProvider(paperProvider);
    engine.rootContext()->setContextProperty("app", &controller);
    engine.rootContext()->setContextProperty("netbrowser", &netBrowser);
    engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;
    ret = a.exec();
#ifdef Q_OS_WIN
    timeEndPeriod(1);
#endif
    return ret;
}
