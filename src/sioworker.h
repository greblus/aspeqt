#ifndef SIOWORKER_H
#define SIOWORKER_H

#include <QThread>
#include <atomic>
#include <QRecursiveMutex>
#include <QMutex>
#include <QElapsedTimer>
#include <climits>

#include "serialport.h"

enum SIO_CDEVIC
{
    DISK_BASE_CDEVIC = 0x31,
    PRINTER_BASE_CDEVIC = 0x40,
    SMART_CDEVIC = 0x45,
    ASPEQT_CLIENT_CDEVIC = 0x46,
    RS232_BASE_CDEVIC = 0x50,
    PCLINK_CDEVIC = 0x6F
};

class SioWorker;

class SioDevice : public QObject
{
    Q_OBJECT

protected:
    int m_deviceNo;
    QMutex mLock;
    SioWorker *sio;
public:
    SioDevice(SioWorker *worker);
    virtual ~SioDevice();
    virtual void handleCommand(quint8 command, quint16 aux) = 0;
    // Only meaningful for a device that takes over the line outside the SIO
    // command protocol -- currently just RDevice in its stream (modem) mode.
    virtual void processSerialData(const QByteArray &data) { Q_UNUSED(data); }
    virtual void forceCommandMode(bool sendAlert = false) { Q_UNUSED(sendAlert); }
    virtual QString deviceName();
    inline void lock() {mLock.lock();}
    inline bool tryLock() {return mLock.tryLock();}
    inline void unlock() {mLock.unlock();}
    inline void setDeviceNo(int no) {emit statusChanged(m_deviceNo); m_deviceNo = no; emit statusChanged(no);}
    inline int deviceNo() const {return m_deviceNo;}
signals:
    void statusChanged(int deviceNo);
};

class SioWorker : public QThread
{
    Q_OBJECT

private:
    quint8 sioChecksum(const QByteArray &data, uint size);
    QRecursiveMutex *deviceMutex;
    SioDevice* devices[256];
    AbstractSerialPortBackend *mPort;
    // Same reason as mCanceled: the worker polls this between frames.
    std::atomic<bool> mustTerminate{false};

    // Stream (modem) mode: while the R: device holds the line, the loop stops
    // reading SIO command frames and shuttles raw bytes both ways instead.
    // How often to check COMMAND while streaming. The backend latches the line
    // from the data it is already receiving, so the check is cheap and a short
    // interval mainly bounds how fast we notice an Atari reset.
    static const int STREAM_GUARD_MS = 20;
    // Toggled from the R: device's thread, polled by the worker loop.
    std::atomic<bool> m_isStreaming{false};
    QElapsedTimer m_streamGuardTimer;
    void runStreamMode();
public:
    AbstractSerialPortBackend* port() {return mPort;}
    int maxSpeed;

    SioWorker();
    ~SioWorker();

    bool wait (unsigned long time = ULONG_MAX);
    // Ask the loop to finish without blocking the caller: the UI must stay
    // responsive while the worker completes the SIO command it is in the
    // middle of. The port is closed by wait() once finished() has fired.
    void requestStop();

    void run();

    void installDevice(quint8 no, SioDevice *device);
    void uninstallDevice(quint8 no);
    void swapDevices(quint8 d1, quint8 d2);
    SioDevice* getDevice(quint8 no);

    QString deviceName(int device);
    static void usleep(unsigned long time) {QThread::usleep(time);}
signals:
    void statusChanged(QString status);
public slots:
    void start(Priority p = InheritPriority);

    // Driven by RDevice when the Atari's 850 handler enters and leaves
    // concurrent (stream) mode.
    void onChangeBaudRate(int baudRate);
    void onStreamFinished();
    void onWriteRawData(const QByteArray &data);
};

class CassetteRecord {
public:
    int baudRate;
    int gapDuration;
    int totalDuration;
    QByteArray data;
};

class CassetteWorker : public QThread
{
    Q_OBJECT

private:
    QMutex mustTerminate;
    AbstractSerialPortBackend *mPort;
    QList<CassetteRecord> mRecords;
public:
    AbstractSerialPortBackend* port() {return mPort;}

    CassetteWorker();
    ~CassetteWorker();

    bool loadCasImage(const QString &fileName);

    bool wait (unsigned long time = ULONG_MAX);

    void run();

    int mTotalDuration;
signals:
    void statusChanged(int remainingTime);
public slots:
    void start(Priority p = InheritPriority);
};

#endif // SIOWORKER_H
