#include <QApplication>
#include <QTextCodec>
#include <QLibraryInfo>
#include <QStyleHints>
#include "mainwindow.h"

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
    QApplication a(argc, argv);
    // The UI (grey icons/text) was designed for a light background; force a light
    // colour scheme so it doesn't render as a black void under the system dark mode.
    a.styleHints()->setColorScheme(Qt::ColorScheme::Light);
    MainWindow w;
    w.show();
    ret = a.exec();
#ifdef Q_OS_WIN
    timeEndPeriod(1);
#endif
    return ret;
}
