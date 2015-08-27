#include "serialport.h"
#include "sioworker.h"
#include "headers/atarisio.h"
#include "aspeqtsettings.h"

#include <QTime>
#include <QtDebug>

#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <QAndroidJniObject>

extern char * arr;
static bool debug = false;

AbstractSerialPortBackend::AbstractSerialPortBackend(QObject *parent)
    : QObject(parent)
{
}

AbstractSerialPortBackend::~AbstractSerialPortBackend()
{
}

StandardSerialPortBackend::StandardSerialPortBackend(QObject *parent)
    : AbstractSerialPortBackend(parent)
{
    mHandle = 0;
}

StandardSerialPortBackend::~StandardSerialPortBackend()
{
    if (isOpen()) {
        close();
    }
}

QString StandardSerialPortBackend::defaultPortName()
  {

      return QString("Sio2PC-USB");
  }

bool StandardSerialPortBackend::open()
{
    if (debug) qWarning() << "!i" << tr("open");

    if (isOpen()) {
        close();
    }

    QString name = aspeqtSettings->serialPortName();

    mHandle = QAndroidJniObject::callStaticMethod<jint>("net/greblus/MyActivity", "ftdiOpenDevice", "()I");

    if (mHandle == 0 || mHandle > 1000000) {
    if (debug) qCritical() << "!e" << tr("Cannot open serial port '%1': %2")
                       .arg(name, "No device connected!");
        return false;
    }

    if (mHandle == -1)
        qCritical() << "!e" << tr("No FTDI chip detected!");

    mMethod = aspeqtSettings->serialPortHandshakingMethod();
    mCanceled = false;

    if (!setNormalSpeed()) {
        close();
        return false;
    }

    QString m;
        switch (mMethod) {
        case 0:
            m = "RI";
            break;
        case 1:
            m = "DSR";
            break;
        case 2:
            m = "CTS";
            break;
        default:
            m = "DSR";
    }
    /* Notify the user that emulation is started */
    qWarning() << "!i" << tr("Emulation started through standard serial port backend on '%1' with %2 handshaking.")
                  .arg(aspeqtSettings->serialPortName())
                  .arg(m);

    return true;
}

bool StandardSerialPortBackend::isOpen()
{
    if (debug) qWarning() << "!i" << tr("isOpen %1").arg(mHandle);
    return (mHandle > 0);
}

void StandardSerialPortBackend::close()
{
    if (debug) qWarning() << "!i" << tr("close");
    cancel();
    QAndroidJniObject::callStaticMethod<jint>("net/greblus/MyActivity", "ftdiCloseDevice", "()V");
    mHandle = -1;
}

void StandardSerialPortBackend::cancel()
{
    if (debug) qWarning() << "!i" << tr("cancel");
    mCanceled = true;
}

int StandardSerialPortBackend::speedByte()
{
    if (debug) qWarning() << "!i" << tr("speedByte");
    if (aspeqtSettings->serialPortUsePokeyDivisors()) {
        return aspeqtSettings->serialPortPokeyDivisor();
    } else {
        int speed = 0x08;
        switch (aspeqtSettings->serialPortMaximumSpeed()) {
        case 0:
            speed = 0x28;
            break;
        case 1:
            speed = 0x10;
            break;
        case 2:
            speed = 0x08;
            break;
        }
        return speed;
    }
}

bool StandardSerialPortBackend::setNormalSpeed()
{
    if (debug) qWarning() << "!i" << tr("setNormalSpeed");
    mHighSpeed = false;
    return setSpeed(19200);
}

bool StandardSerialPortBackend::setHighSpeed()
{
    if (debug) qWarning() << "!i" << tr("setHighSpeed");

    mHighSpeed = true;
    if (aspeqtSettings->serialPortUsePokeyDivisors()) {
        return setSpeed(divisorToBaud(aspeqtSettings->serialPortPokeyDivisor()));
    } else {
        int speed = 57600;
        switch (aspeqtSettings->serialPortMaximumSpeed()) {
        case 0:
            speed = 19200;
            break;
        case 1:
            speed = 38400;
            break;
        case 2:
            speed = 57600;
            break;
        }
        return setSpeed(speed);
    }
}

bool StandardSerialPortBackend::setSpeed(int speed)
{
    if (debug) qWarning() << "!i" << tr("Serial port speed set to %1.").arg(speed);

    bool ret = QAndroidJniObject::callStaticMethod<jint>("net/greblus/MyActivity", "setSpeed", "(I)Z", speed);
    if (!ret) {
        if (debug) qCritical() << "!e" << tr("Cannot set serial port speed: %1")
                       .arg(lastErrorMessage());
    return false;
    }

    emit statusChanged(tr("%1 bits/sec").arg(speed));
    qWarning() << "!i" << tr("Serial port speed set to %1.").arg(speed);

    mSpeed = speed;
    return true;
}

int StandardSerialPortBackend::speed()
{
    return mSpeed;
}

QByteArray StandardSerialPortBackend::readCommandFrame()
{
    QByteArray data;

    int mask;

    switch (mMethod) {
    case 0:
        mask = 64;
        break;
    case 1:
        mask = 32;
        break;
    case 2:
        mask = 16;
        break;
    default:
        mask = 32;
    }

        int status = -1;
        int retries = 0, totalRetries = 0;

        do {
            data.clear();
            /* First, wait until command line goes off */
            do {
                status = QAndroidJniObject::callStaticMethod<jint>("net/greblus/MyActivity", "getModemStatus", "()I");
                if (status < 0) {
                    if (debug) qCritical() << "!e" << tr("Cannot retrieve serial port status: %1").arg(lastErrorMessage());
                    return data;
                }
                if (status & mask) {
                    QThread::yieldCurrentThread();
                }
            } while ((status & mask) && !mCanceled);

            /* Now wait for it to go on again */
            do {
                status = QAndroidJniObject::callStaticMethod<jint>("net/greblus/MyActivity", "getModemStatus", "()I");
                if (status < 0) {
                    if (debug) qCritical() << "!e" << tr("Cannot retrieve serial port status: %1").arg(lastErrorMessage());
                    return data;
                }
                if (!(status & mask)) {
                    QThread::yieldCurrentThread();
                }
            } while (!(status & mask) && !mCanceled );

            if (mCanceled) {
                return data;
            }

            bool ret = QAndroidJniObject::callStaticMethod<jint>("net/greblus/MyActivity", "purgeRX", "()Z");
            if (!ret) {
                if (debug) qCritical() << "!e" << tr("Cannot clear serial port: %1")
                               .arg(lastErrorMessage());
            }

            data = readDataFrame(4, false);

            if (!data.isEmpty()) {
                do {
                   status = QAndroidJniObject::callStaticMethod<jint>("net/greblus/MyActivity", "getModemStatus", "()I");
                    if (status < 0) {
                        if (debug) qCritical() << "!e" << tr("Cannot retrieve serial port status: %1").arg(lastErrorMessage());
                        return data;
                    }
                } while ((status & mask) && !mCanceled);

                if (debug) {
                    QString tmp="";
                    for (int i=0; i<4; i++) {
                        tmp+=QString::number(data[i]) + ", ";
                    }
                    if (tmp.length() > 2)
                        tmp = tmp.left(tmp.length()-2);

                    QAndroidJniObject msg = QAndroidJniObject::fromString("Qt side buf = "+tmp);
                    QAndroidJniObject::callStaticMethod<void>("net/greblus/MyActivity", "qLog", "(Ljava/lang/String;)V", msg.object<jstring>() );
                }
                break;
            } else {
                retries++;
                totalRetries++;
                if (retries == 2) {
                    retries = 0;
                    if (mHighSpeed) {
                        setNormalSpeed();
                    } else {
                        setHighSpeed();
                    }
                }
            }
    //    } while (totalRetries < 100);
        } while (1);
        return data;
}

QByteArray StandardSerialPortBackend::readDataFrame(uint size, bool verbose)
{
    QByteArray data = readRawFrame(size + 1, verbose);
    if (data.isEmpty()) {
        return NULL;
    }
    quint8 expected = (quint8)data.at(size);
    quint8 got = sioChecksum(data, size);
    if (expected == got) {
        data.resize(size);
        return data;
    } else {
        if (verbose) {
            qWarning() << "!w" << tr("Data frame checksum error, expected: %1, got: %2. (%3)")
                           .arg(expected)
                           .arg(got)
                           .arg(QString(data.toHex()));
        }
        data.clear();
        return data;
    }
}

bool StandardSerialPortBackend::writeDataFrame(const QByteArray &data)
{
    QByteArray copy(data);
    copy.resize(copy.size() + 1);
    copy[copy.size() - 1] = sioChecksum(copy, copy.size() - 1);
    SioWorker::usleep(50);

    return writeRawFrame(copy);
}

bool StandardSerialPortBackend::writeCommandAck()
{
    if (debug) qWarning() << "!i" << tr("writeCommandAck");
    return writeRawFrame(QByteArray(1, 65));
}

bool StandardSerialPortBackend::writeCommandNak()
{
    if (debug) qWarning() << "!i" << tr("writeCommandNak");
    return writeRawFrame(QByteArray(1, 78));
}

bool StandardSerialPortBackend::writeDataAck()
{
    if (debug) qWarning() << "!i" << tr("writeDataAck");
    return writeRawFrame(QByteArray(1, 65));
}

bool StandardSerialPortBackend::writeDataNak()
{
    if (debug) qWarning() << "!i" << tr("writeDataNak");
    return writeRawFrame(QByteArray(1, 78));
}

bool StandardSerialPortBackend::writeComplete()
{
    if (debug) qWarning() << "!i" << tr("writeComplete");
    SioWorker::usleep(300);
    return writeRawFrame(QByteArray(1, 67));
}

bool StandardSerialPortBackend::writeError()
{
    if (debug) qWarning() << "!i" << tr("writeError");
    SioWorker::usleep(300);
    return writeRawFrame(QByteArray(1, 78));
}

quint8 StandardSerialPortBackend::sioChecksum(const QByteArray &data, uint size)
{
    if (debug) qWarning() << "!i" << tr("sioChecksum");
    uint i;
    uint sum = 0;

    for (i=0; i < size; i++) {
        sum += (quint8)data[i];
        if (sum > 255) {
            sum -= 255;
        }
    }

    return sum;
}

QByteArray StandardSerialPortBackend::readRawFrame(uint size, bool verbose)
{
    if (debug) qWarning() << "!i" << tr("readRawFrame");

    int result;
    uint total, rest;

    QByteArray data;
    data.resize(size);

    total = 0;
    rest = size;
    QTime startTime = QTime::currentTime();
    int timeOut = data.count() * 12000 / mSpeed + 10;
    int elapsed;

    do {
        result =  QAndroidJniObject::callStaticMethod<jint>("net/greblus/MyActivity", "ftdiRead", "(II)I", rest, total);

        if (result < 0) {
            result = 0;
        }

        total += result;
        rest -= result;
        elapsed = QTime::currentTime().msecsTo(startTime);
    } while (total < size && elapsed > -timeOut);

    data.clear();
    for (int i=0; i<size; i++)
    {
        data.insert(i, (quint8)(arr[i] & 0xff));
    }

    if (debug) {
        QString tmp;
        for (int i=0; i<data.count(); i++)
        {
            tmp += QString::number(arr[i] & 0xff) + " ";
        }

         qCritical() << "!e" << tr("readRawFrame: %1").arg(tmp);
        }

    if ((uint)total != size) {
        if (verbose) {
            data.resize(total);
            if (debug) {
             QAndroidJniObject msg = QAndroidJniObject::fromString("Serial port read timeout.");
             QAndroidJniObject::callStaticMethod<void>("net/greblus/MyActivity", "qLog", "(Ljava/lang/String;)V", msg.object<jstring>() );
            }
        }
        data.clear();
        return data;
    }

    return data;
}

bool StandardSerialPortBackend::writeRawFrame(const QByteArray &data)
{
    int result;
    uint total, rest;

    total = 0;

    rest = data.count();

    for (int i = 0; i<rest; i++)
        arr[i] = (quint8)(data.at(i)&0xff);

    if (debug) {
     QAndroidJniObject msg = QAndroidJniObject::fromString("data.count():" + QString::number(rest));
     QAndroidJniObject::callStaticMethod<void>("net/greblus/MyActivity", "qLog", "(Ljava/lang/String;)V", msg.object<jstring>() );
    }

    QTime startTime = QTime::currentTime();
    int timeOut = data.count() * 120000 / mSpeed + 10;
    int elapsed;

    do {
        result = QAndroidJniObject::callStaticMethod<jint>("net/greblus/MyActivity", "ftdiWrite", "(II)I", rest, total);

        if (debug) {
            QAndroidJniObject msg = QAndroidJniObject::fromString("Qt5 side: ftdiWrite() result:" + QString::number(result));
            QAndroidJniObject::callStaticMethod<void>("net/greblus/MyActivity", "qLog", "(Ljava/lang/String;)V", msg.object<jstring>() );
        }

        if (result < 0 ) {
            if (debug) qCritical() << "!e" << tr("Cannot write to serial port: %1")
                           .arg(lastErrorMessage());
            return false;
        }

        if (debug) {
            QString tmp;

            for (int i=0; i<data.count(); i++)
            {
                tmp += QString::number(arr[i] & 0xff) + " ";
            }

            qCritical() << "!e" << tr("writeRawFrame: %1").arg(tmp);
        }

        if (result < 0) {
            result = 0;
        }
        total += result;
        rest -= result;
        elapsed = QTime::currentTime().msecsTo(startTime);
    } while (total < (uint)data.count() && elapsed > -timeOut);

    if (total != (uint)data.count()) {
        if (debug) qCritical() << "!e" << tr("Serial port write timeout. (%1 of %2 written)").arg(total).arg(data.count());
        return false;
    }

    return true;
}

QString StandardSerialPortBackend::lastErrorMessage()
{
    return QString::fromUtf8(strerror(errno)) + ".";
}
