#ifndef SERIALPORTUNIX_H
#define SERIALPORTUNIX_H

#include "serialport.h"
#include <atomic>

class StandardSerialPortBackend : public AbstractSerialPortBackend
{
    Q_OBJECT

public:
    StandardSerialPortBackend(QObject *parent = 0);
    ~StandardSerialPortBackend();

    static QString defaultPortName();

    bool open();
    bool isOpen();
    void close();
    void cancel();
    int speedByte();
    QByteArray readCommandFrame();
    QByteArray readDataFrame(uint size, bool verbose = true);
    bool writeDataFrame(QByteArray &data);
    bool writeCommandAck();
    bool writeCommandNak();
    bool writeDataAck();
    bool writeDataNak();
    bool writeComplete();
    bool writeError();
    bool setSpeed(int speed);
    bool writeRawFrame(const QByteArray &data);
    QByteArray readRawFrame(uint size, bool verbose = true) override;

    void setStreamMode(bool stream) override;
    bool isStreamMode() const override { return m_isStreamMode; }
    bool isCommandLineAsserted() override;

private:
    // Set from the GUI thread to break the command-frame wait loop.
    std::atomic<bool> mCanceled{false};
    int mHandle;
    int mSpeed;
    int mMethod;
    bool m_isStreamMode = false;

    bool setNormalSpeed();
    bool setHighSpeed();
    int speed();
    bool mHighSpeed;
    quint8 sioChecksum(const QByteArray &data, uint size);
    int commandLineMask() const;
    QString lastErrorMessage();
};

class AtariSioBackend : public AbstractSerialPortBackend
{
    Q_OBJECT

public:
    AtariSioBackend(QObject *parent = 0);
    ~AtariSioBackend();

    static QString defaultPortName();

    bool open();
    bool isOpen();
    void close();
    void cancel();
    int speedByte();
    QByteArray readCommandFrame();
    QByteArray readDataFrame(uint size, bool verbose = true);
    bool writeDataFrame(const QByteArray &data);
    bool writeCommandAck();
    bool writeCommandNak();
    bool writeDataAck();
    bool writeDataNak();
    bool writeComplete();
    bool writeError();
    bool setSpeed(int speed);
    bool writeRawFrame(const QByteArray &data);

private:
    int mHandle, mCancelHandles[2];
    int mSpeed;
    int mMethod;
    QString lastErrorMessage();
};

#endif // SERIALPORTUNIX_H
