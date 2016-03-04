#include <QApplication>
#include <QTextCodec>
#include <QLibraryInfo>
#include "mainwindow.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <Mmsystem.h>
#endif

#ifdef Q_OS_ANDROID
#include <QAndroidJniObject>
#include <jni.h>

jbyte *jbuf = NULL;
char *bbuf;
extern "C" {
    JNIEXPORT void JNICALL
    Java_net_greblus_SerialActivity_sendBufAddr(JNIEnv *env/*env*/,
    jobject /*obj*/, jobject buf)
        {
            jbuf = (jbyte *)env->GetDirectBufferAddress(buf);
            bbuf = reinterpret_cast<char *>(jbuf);
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
    MainWindow w;
#if QT_VERSION >= 0x050600
    a.setAttribute(Qt::AA_EnableHighDpiScaling, true);
#endif
    w.show();
    ret = a.exec();
#ifdef Q_OS_WIN
    timeEndPeriod(1);
#endif
    return ret;
}
