#include <QApplication>
#include <QTextCodec>
#include <QLibraryInfo>
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
    MainWindow w;
    w.show();
    ret = a.exec();
#ifdef Q_OS_WIN
    timeEndPeriod(1);
#endif
    return ret;
}
