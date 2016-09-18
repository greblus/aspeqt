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

extern char *rbuf;
extern char *wbuf;
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

      return QString("Sio2BT");
  }

bool StandardSerialPortBackend::open()
{
    if (debug) qWarning().noquote() << "!i" << tr("open");

    if (isOpen()) {
        close();
    }

    QString name = aspeqtSettings->serialPortName();

    mHandle = QAndroidJniObject::callStaticMethod<jint>("net/greblus/SerialActivity", "openDevice", "()I");

    if (mHandle == 0) {
    if (debug) qCritical() << "!e" << tr("Cannot open serial port '%1': %2")
                       .arg(name, "No device connected!");
        return false;
    }

    if (mHandle == -1)
        qCritical() << "!e" << tr("No device detected!");

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
        case 3:
            m = "SOFT";
            break;
        default:
            m = "SOFT";
    }
    /* Notify the user that emulation is started */
    qWarning().noquote() << "!i" << tr("Emulation started through standard serial port backend on '%1' with %2 handshaking.")
                  .arg(aspeqtSettings->serialPortName())
                  .arg(m);

    return true;
}

bool StandardSerialPortBackend::isOpen()
{
    if (debug) qWarning().noquote() << "!i" << tr("isOpen %1").arg(mHandle);
    return (mHandle > 0);
}

void StandardSerialPortBackend::close()
{
    if (debug) qWarning().noquote() << "!i" << tr("close");
    cancel();
    mHandle = -1;
    QAndroidJniObject::callStaticMethod<jint>("net/greblus/SerialActivity", "closeDevice", "()V");
}

void StandardSerialPortBackend::cancel()
{
    if (debug) qWarning().noquote() << "!i" << tr("cancel");
    mCanceled = true;
}

int StandardSerialPortBackend::speedByte()
{
    if (debug) qWarning().noquote() << "!i" << tr("speedByte");
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
    if (debug) qWarning().noquote() << "!i" << tr("setNormalSpeed");
    mHighSpeed = false;
    return setSpeed(19200);
}

bool StandardSerialPortBackend::setHighSpeed()
{
    if (debug) qWarning().noquote() << "!i" << tr("setHighSpeed");

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
    if (debug) qWarning().noquote() << "!i" << tr("Serial port speed set to %1.").arg(speed);

    return true; //not needed for SIO2BT

    int ret = QAndroidJniObject::callStaticMethod<jint>("net/greblus/SerialActivity", "setSpeed", "(I)I", speed);
    if (ret < 0) {
        if (debug) qCritical() << "!e" << tr("Cannot set serial port speed: %1")
                       .arg(lastErrorMessage());
    return false;
    }
    if (mSpdChanged)
        emit statusChanged(tr("%1 bits/sec").arg(ret));

    mSpeed = ret;
    return true;
}

int StandardSerialPortBackend::speed()
{
    return mSpeed;
}

QByteArray StandardSerialPortBackend::readCommandFrame()
{
    QByteArray data;
    int retries = 0, result = 0;

    do {
        if (mMethod == SOFTWARE_HANDSHAKE)
            result = QAndroidJniObject::callStaticMethod<jint>("net/greblus/SerialActivity", "getSWCommandFrame", "()I");
        else
            result = QAndroidJniObject::callStaticMethod<jint>("net/greblus/SerialActivity", "getHWCommandFrame", "(I)I", mMethod);

        switch (result) {
            case 1:
                data.setRawData(rbuf, 4);
                break;
            default:
                data.clear();
        }

        if (!data.isEmpty()) {
            break;
        } else if (false) { // not needed for SIO2BT
            retries++;
            if (retries == 2) {
                retries = 0;
                if (mHighSpeed) {
                    mSpdChanged = false;
                    setNormalSpeed();

                } else {
                    mSpdChanged = true;
                    setHighSpeed();
                }
            }
        }
    } while (!mCanceled);

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
            qWarning().noquote() << "!w" << tr("Data frame checksum error, expected: %1, got: %2. (%3)")
                           .arg(expected)
                           .arg(got)
                           .arg(QString(data.toHex()));
        }
        data.clear();
        return data;
    }
}

bool StandardSerialPortBackend::writeDataFrame(QByteArray &data)
{
    data.resize(data.size() + 1);
    data[data.size() - 1] = sioChecksum(data, data.size() - 1);

    return writeRawFrame(data);
}

bool StandardSerialPortBackend::writeCommandAck()
{
    if (debug) qWarning().noquote() << "!i" << tr("writeCommandAck");
    return writeRawFrame(QByteArray(1, 65));
}

bool StandardSerialPortBackend::writeCommandNak()
{
    if (debug) qWarning().noquote() << "!i" << tr("writeCommandNak");
    return writeRawFrame(QByteArray(1, 78));
}

bool StandardSerialPortBackend::writeDataAck()
{
    if (debug) qWarning().noquote() << "!i" << tr("writeDataAck");
    return writeRawFrame(QByteArray(1, 65));
}

bool StandardSerialPortBackend::writeDataNak()
{
    if (debug) qWarning().noquote() << "!i" << tr("writeDataNak");
    return writeRawFrame(QByteArray(1, 78));
}

bool StandardSerialPortBackend::writeComplete()
{
    if (debug) qWarning().noquote() << "!i" << tr("writeComplete");
    SioWorker::usleep(10000);
    return writeRawFrame(QByteArray(1, 67));
}

bool StandardSerialPortBackend::writeError()
{
    if (debug) qWarning().noquote() << "!i" << tr("writeError");
    SioWorker::usleep(10000);
    return writeRawFrame(QByteArray(1, 78));
}

quint8 StandardSerialPortBackend::sioChecksum(const QByteArray &data, uint size)
{
    if (debug) qWarning().noquote() << "!i" << tr("sioChecksum");
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
    if (debug) qWarning().noquote() << "!i" << tr("readRawFrame");

    int result;
    uint total, rest;
    QByteArray data;

    total = 0;
    rest = size;
    QTime startTime = QTime::currentTime();
    int timeOut = data.count() * 24000 / mSpeed + 200;
    int elapsed;

    do {
        result =  QAndroidJniObject::callStaticMethod<jint>("net/greblus/SerialActivity", "read", "(II)I", rest, total);
        if (result < 0) result = 0;
        total += result;
        rest -= result;
        elapsed = QTime::currentTime().msecsTo(startTime);
    } while (total < size && elapsed > -timeOut);

    data.setRawData(rbuf, size);

    if (debug) {
        QString tmp;
        for (int i=0; i<data.count(); i++)
        {
            tmp += QString::number(rbuf[i]) + " ";
        }

         qCritical() << "!e" << tr("readRawFrame: %1").arg(tmp);
        }

    if ((uint)total != size) {
        if (verbose) {
            data.resize(total);
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

    std::copy(data.constData(), data.constData()+rest, wbuf);

    QTime startTime = QTime::currentTime();
    int timeOut = data.count() * 24000 / mSpeed + 200;
    int elapsed;

    do {
        result = QAndroidJniObject::callStaticMethod<jint>("net/greblus/SerialActivity", "write", "(II)I", rest, total);

        if (result < 0 ) {
            if (debug) qCritical() << "!e" << tr("Cannot write to serial port: %1")
                           .arg(lastErrorMessage());
            return false;
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
