// R: device -- see rdevice.h. Derived from AspeQt-2k26 by Paul Jones, GPL-2;
// see AUTHORS.txt.

#include "rdevice.h"
#include "rdevice_handler.h"
#include "aspeqtsettings.h"
#include <QDebug>
#include <QCoreApplication>
#include <QFile>
#include <QThread>
#include <QMutexLocker>

#define RESULT_OK           0
#define RESULT_CONNECT      1
#define RESULT_RING         2
#define RESULT_NO_CARRIER   3
#define RESULT_ERROR        4

#define ESCAPE_GUARD_TIME   1000

// Ice-T flushes its 16K buffer to disk and loses serial input while doing so, so
// hold the backlog briefly on re-entry instead of dumping it before it listens.
static const int kResumeHoldMs = 150;

RDevice::RDevice(SioWorker *worker, int portIndex) : SioDevice(worker), m_portIndex(portIndex) // <-- Add to initializer
{

    m_portIndex = portIndex;
    tcpSocket = new QTcpSocket(this);
    tcpSocket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    m_ringPhase = false;

    connect(tcpSocket, &QTcpSocket::connected, this, &RDevice::onSocketConnected);
    connect(tcpSocket, &QTcpSocket::disconnected, this, &RDevice::onSocketDisconnected);
    connect(tcpSocket, &QTcpSocket::readyRead, this, &RDevice::onSocketReadyRead);
    connect(tcpSocket, &QTcpSocket::errorOccurred, this, &RDevice::onSocketError);

    tcpServer = new QTcpServer(this);
    connect(tcpServer, &QTcpServer::newConnection, this, &RDevice::onNewConnection);
    pendingSocket = nullptr;

#ifdef HAVE_LIBSSH
    m_ssh = new SshClient(this);
    connect(m_ssh, &SshClient::connected,    this, &RDevice::onSshConnected);
    connect(m_ssh, &SshClient::disconnected, this, &RDevice::onSshDisconnected);
    connect(m_ssh, &SshClient::rxData,       this, &RDevice::onSshDataReceived);
    connect(m_ssh, &SshClient::error,        this, &RDevice::onSshError);
    connect(m_ssh, &SshClient::hostKeyReady,  this, &RDevice::onSshHostKey);
    connect(m_ssh, &SshClient::promptNeeded,  this, &RDevice::onSshPrompt);
    connect(m_ssh, &SshClient::authFailed,    this, &RDevice::onSshAuthFailed);
#endif

    m_ringTimer = new QTimer(this);
    m_ringTimer->setSingleShot(true);
    connect(m_ringTimer, &QTimer::timeout, this, &RDevice::onRingTimeout);
    m_escapeActionTimer = new QTimer(this);
    m_escapeActionTimer->setSingleShot(true);
    connect(m_escapeActionTimer, &QTimer::timeout, this, &RDevice::onEscapeTriggered);

    m_isEnabled = (aspeqtSettings && aspeqtSettings->rDeviceEnabled());

    if (m_isEnabled) loadPhonebook(aspeqtSettings->phonebookPath());

    // Reads the listener settings itself.
    updateListenerConfig();

    connect(this, &RDevice::dispatchToNetwork, this, [this](const QByteArray &data){
#ifdef HAVE_LIBSSH
        // An interactive login is asking something: the answer is ours, not the
        // server's, until the shell is up.
        if (m_sshAsk != SshAsk::None) {
            for (char c : data) handleSshPromptByte(c);
            return;
        }
#endif
        if (m_isNetworkConnected) {
#ifdef HAVE_LIBSSH
            if (m_isSshMode) m_ssh->write(data); else
#endif
            tcpSocket->write(data);
        }
        else {
            for (char c : data) {
                if (c == 0x0D || (quint8)c == 0x9B) { // Carriage Return
                    if (echoEnabled) {
                        QMutexLocker locker(&m_bufferMutex);
                        m_networkToSioBuffer.append(c); // Echo the CR cleanly
                    }
                    emit executeAtCommand(m_atCmdBuffer);
                    m_atCmdBuffer.clear();
                } else if (c == 8 || c == 126 || c == 127) { // Backspace or Delete
                    if (!m_atCmdBuffer.isEmpty()) {
                        m_atCmdBuffer.chop(1); // Remove from the internal command buffer
                        if (echoEnabled) {
                            QMutexLocker locker(&m_bufferMutex);
                            // Visually erase the character: Backspace, Space, Backspace
                            m_networkToSioBuffer.append(char(8));
                            m_networkToSioBuffer.append(' ');
                            m_networkToSioBuffer.append(char(8));
                        }
                    }
                } else { // Normal typing
                    if (echoEnabled) {
                        QMutexLocker locker(&m_bufferMutex);
                        m_networkToSioBuffer.append(c); // Echo normal characters
                    }
                    m_atCmdBuffer.append(c);
                }
            }
        }
    }, Qt::QueuedConnection);


    connect(this, &RDevice::executeAtCommand, this, &RDevice::processAtCommand, Qt::QueuedConnection);
}

RDevice::~RDevice()
{
    tcpSocket->abort();
    tcpServer->close();
}

void RDevice::resetSession()
{
#ifdef HAVE_LIBSSH
    if (m_ssh->isConnected()) m_ssh->disconnectFromHost();
    m_sshAsk = SshAsk::None;
    m_sshAskBuffer.clear();
    m_oscState = 0;
#endif
    m_isSshMode = false;
    m_isNetworkConnected = false;
    if (tcpSocket->state() != QAbstractSocket::UnconnectedState)
        tcpSocket->abort();

    state = ModemState::CommandMode;
    QMutexLocker locker(&m_bufferMutex);
    m_txBuffer.clear();
    m_networkToSioBuffer.clear();
    m_atCmdBuffer.clear();
}

void RDevice::setEnabled(bool enable)
{
    m_isEnabled = enable;
    if (!enable) {
#ifdef HAVE_LIBSSH
        if (m_ssh->isConnected()) m_ssh->disconnectFromHost();
#endif
        m_isSshMode = false;
        tcpSocket->disconnectFromHost();
        tcpServer->close();
        state = ModemState::CommandMode;
        QMutexLocker locker(&m_bufferMutex);
        m_txBuffer.clear();
    } else {
        updateListenerConfig();
    }
}


void RDevice::handleCommand(quint8 command, quint16 aux)
{
    if (!m_isEnabled) return;

    quint8 aux1 = (aux & 0xFF);
    quint8 aux2 = (aux >> 8) & 0xFF;

    switch (command) {
    case CMD_POLL_TYPE1: handlePollType1(); break;
    case CMD_POLL_TYPE3: handlePollType3(aux1, aux2); break;
    case CMD_RELOCATOR:  handleDownloadRelocator(); break;
    case CMD_DOWNLOAD:   handleDownloadDriver(aux); break;
    case CMD_STATUS:     handleStatus(); break;
    case CMD_WRITE:      handleWrite(aux); break;
    case CMD_READ:       handleRead(aux); break;
    case CMD_CONFIGURE:  handleConfigure(aux1, aux2); break;
    case CMD_CONTROL:    handleControl(aux); break;
    case CMD_AUTOANSWER:
        if (!sio->port()->writeCommandAck()) return;
        sio->port()->writeComplete();
        break;
    case CMD_STREAM:
        handleStream();
        break;
    case CMD_LISTEN:
        handleListen(aux);
        break;
    case CMD_UNLISTEN:
        sio->port()->writeCommandAck();
        tcpServer->close();
        sio->port()->writeComplete();
        break;
    default:
        sio->port()->writeCommandNak();
        break;
    }
}

void RDevice::handleConfigure(quint8 aux1, quint8 aux2) {
    Q_UNUSED(aux2);
    if (!sio->port()->writeCommandAck()) return;

    quint8 speedCode = aux1 & 0x0F;
    switch (speedCode) {
    case 0x08: m_currentBaudRate = 300; break;
    case 0x09: m_currentBaudRate = 600; break;
    case 0x0A: m_currentBaudRate = 1200; break;
    case 0x0B: m_currentBaudRate = 1800; break;
    case 0x0C: m_currentBaudRate = 2400; break;
    case 0x0D: m_currentBaudRate = 4800; break;
    case 0x0E: m_currentBaudRate = 9600; break;
    case 0x0F: m_currentBaudRate = 19200; break;
    default:   m_currentBaudRate = 19200; break;
    }

    sio->port()->writeComplete();
}

void RDevice::sendDataToAtari(const QByteArray &data)
{
    SioWorker::usleep(2000);
    sio->port()->writeComplete();
    SioWorker::usleep(2000);
    QByteArray frame = data;   // writeDataFrame() takes a non-const reference
    sio->port()->writeDataFrame(frame);
}

void RDevice::handlePollType1() {
    if (!sio->port()->writeCommandAck()) return;

    QByteArray bootBlock(12, 0);
    bootBlock[0]=0x50; // DDEVIC
    bootBlock[1]=0x01; // DUNIT
    bootBlock[2]=0x21; // DCOMND = '!' (boot relocator)
    bootBlock[3]=0x40; // DSTATS (Read)
    bootBlock[4]=0x00; // DBUFLO
    bootBlock[5]=0x05; // DBUFHI = $0500
    bootBlock[6]=0x08; // DTIMLO = 8 vblanks
    bootBlock[8]=sizeof(relocator_stub)&0xFF;
    bootBlock[9]=sizeof(relocator_stub)>>8;

    sendDataToAtari(bootBlock);
}

// Type 3/4 poll ($4F/$40): stay silent. Our handler blob is raw 6502 for the
// 850 relocator, not the relocatable-record format a poll reply promises, so
// answering makes the OS parse garbage and hang. The handler loads via the
// Type 1 poll ($3F) -> relocator ($21) -> download ($26) path instead.
void RDevice::handlePollType3(quint8 aux1, quint8 aux2) {
    Q_UNUSED(aux1);
    Q_UNUSED(aux2);
}

void RDevice::handleDownloadRelocator() {
    if (!sio->port()->writeCommandAck()) return;
    QByteArray payload((const char*)relocator_stub, sizeof(relocator_stub));
    sendDataToAtari(payload);
}

// The 850 relocator asks once and expects the whole handler in one data frame.
void RDevice::handleDownloadDriver(quint16 aux) {
    Q_UNUSED(aux);
    if (!sio->port()->writeCommandAck()) return;
    QByteArray payload((const char*)driver_850, sizeof(driver_850));
    sendDataToAtari(payload);
}

void RDevice::handleStatus() {
    if (!sio->port()->writeCommandAck()) return;

    QByteArray status(4, 0);
    status[1] = 0xC0 | 0x30;

    if (m_isNetworkConnected) {
        status[1] |= 0x0C;
    }

    sendDataToAtari(status);
}

void RDevice::handleRead(quint16 len) {
    if (!sio->port()->writeCommandAck()) return;

    int readLen = (len > 0) ? len : 128;
    QByteArray chunk;

    {
        QMutexLocker locker(&m_bufferMutex);
        chunk = m_txBuffer.left(readLen);
        m_txBuffer.remove(0, chunk.size());
    }

    while (chunk.size() < readLen) chunk.append((char)0x00);

    sendDataToAtari(chunk);
}

void RDevice::handleStream() {
    if (!sio->port()->writeCommandAck()) return;

    QByteArray response;
    response.resize(9);
    response[0] = 0x28; response[1] = 0xA0; response[2] = 0x00;
    response[3] = 0xA0; response[4] = 0x28; response[5] = 0xA0;
    response[6] = 0x00; response[7] = 0xA0; response[8] = 0x78;

    switch (m_currentBaudRate) {
    case 300:   response[0] = response[4] = 0xA0; response[2] = response[6] = 0x0B; break;
    case 600:   response[0] = response[4] = 0xCC; response[2] = response[6] = 0x05; break;
    case 1200:  response[0] = response[4] = 0xE3; response[2] = response[6] = 0x02; break;
    case 1800:  response[0] = response[4] = 0xEA; response[2] = response[6] = 0x01; break;
    case 2400:  response[0] = response[4] = 0x6E; response[2] = response[6] = 0x01; break;
    case 4800:  response[0] = response[4] = 0xB3; response[2] = response[6] = 0x00; break;
    case 9600:  response[0] = response[4] = 0x56; response[2] = response[6] = 0x00; break;
    case 19200: break;
    }

    sendDataToAtari(response);

    {
        // Carry over network data buffered in command mode; dropping it loses
        // the block a terminal left in flight when it stepped out to write disk.
        QMutexLocker locker(&m_bufferMutex);
        m_networkToSioBuffer.prepend(m_txBuffer);
        m_txBuffer.clear();
        // Drop any half-typed AT command: left over, it can later join fresh
        // bytes into a complete one and redial by itself.
        m_atCmdBuffer.clear();
    }

    state = ModemState::StreamMode;
    m_streamEntered.start();
    m_escapeTimer.start();
    m_plusCount = 0;

    sio->onChangeBaudRate(m_currentBaudRate);

    // Let the line settle, then forward (not drop) whatever already waits --
    // dropping it costs an ACK per concurrent-mode re-entry and stalls XMODEM.
    SioWorker::usleep(1100);
    if (sio->port()) {
        const QByteArray pending = sio->port()->readRawFrame(256, false);
        if (!pending.isEmpty())
            processSerialData(pending);
    }
}

void RDevice::handleWrite(quint16 aux) {
    if (!sio->port()->writeCommandAck()) return;

    quint8 logicalLen = aux & 0xFF;

    // A zero-length write (Atari leaving concurrent mode) carries no data frame;
    // reading one anyway would swallow the next command frame.
    if (logicalLen == 0) {
        sio->port()->writeComplete();
        return;
    }

    QByteArray data = sio->port()->readDataFrame(64);

    if (data.isEmpty()) {
        sio->port()->writeError();
        return;
    }

    sio->port()->writeDataAck();

    // Connected: these bytes are modem traffic, not an AT command.
    if (m_isNetworkConnected) {
        emit dispatchToNetwork(data.left(qMin<int>(logicalLen, data.size())));
        sio->port()->writeComplete();
        return;
    }

    for (int i = 0; i < logicalLen && i < data.size(); i++) {
        char c = data.at(i);
        if (c == 0x0D || (quint8)c == 0x9B) {
            emit executeAtCommand(m_atCmdBuffer);
            m_atCmdBuffer.clear();
        }
        else if (c == 8 || c == 126 || c == 127) {
            if (!m_atCmdBuffer.isEmpty()) {
                m_atCmdBuffer.chop(1); // Silently remove from the actual command buffer
            }
        }
        else {
            m_atCmdBuffer.append(c);
        }
    }
    sio->port()->writeComplete();
}

void RDevice::handleControl(quint16 aux) {
    if (!sio->port()->writeCommandAck()) return;

    quint8 ctrl = aux & 0xFF;

    // Aux1 bit 7 = DTR; clearing it (e.g. BBS Express hanging up) drops the line.
    if ((ctrl & 0x80) == 0) {
        QMetaObject::invokeMethod(this, "hangup", Qt::QueuedConnection);
    }

    sio->port()->writeComplete();
}

void RDevice::handleListen(quint16 aux) {
    if (tcpServer->isListening()) {
        sio->port()->writeCommandAck();
        sio->port()->writeComplete();
    } else if (tcpServer->listen(QHostAddress::Any, aux)) {
        sio->port()->writeCommandAck();
        sio->port()->writeComplete();
    } else {
        sio->port()->writeCommandNak();
    }
}


void RDevice::processSerialData(const QByteArray &data) {
    if (state != ModemState::StreamMode) return;

    static bool escPending = false;
    QByteArray finalDataToNetwork;

    for (int i = 0; i < data.size(); ++i) {
        char c = data[i];

        // Convert Atari Backspace to Standard Backspace here
        if (c == 126 || c == 127) c = 8;

        if (escPending) {
            escPending = false;

            if (c == 'U' || c == 'u') {
                QString inject = m_currentConnection.login + "\r";
                emit dispatchToNetwork(inject.toUtf8());
                continue;
            }
            if (c == 'P' || c == 'p') {
                QString inject = m_currentConnection.password + "\r";
                emit dispatchToNetwork(inject.toUtf8());
                continue;
            }
            if (c == 'H' || c == 'h') {
                QMetaObject::invokeMethod(this, "hangup", Qt::QueuedConnection);
                continue;
            }

            finalDataToNetwork.append(0x1B);
            finalDataToNetwork.append(c);
        } else if (c == 0x1B) {
            escPending = true;

        } else {
            // +++ escape: pass the plusses through, arm the guard timer on the
            // third, cancel on any other byte (TIES-style).
            if (m_isNetworkConnected) {
                if (c == '+') {
                    m_escapeBuffer.append(c);
                    finalDataToNetwork.append(c);
                    if (m_escapeBuffer.length() > 3) {
                        m_escapeBuffer.clear();
                    } else if (m_escapeBuffer.length() == 3) {
                        QMetaObject::invokeMethod(m_escapeActionTimer, "start", Qt::QueuedConnection, Q_ARG(int, 1000));
                    }
                } else {
                    m_escapeBuffer.clear();
                    QMetaObject::invokeMethod(m_escapeActionTimer, "stop", Qt::QueuedConnection);
                    finalDataToNetwork.append(c);
                }
            } else {
                finalDataToNetwork.append(c);
            }
        }
      }

    // Only dispatch to the network lambda if there is actually data left!
    if (!finalDataToNetwork.isEmpty()) {
        emit dispatchToNetwork(finalDataToNetwork);
    }
}


void RDevice::onEscapeTriggered() {
    if (m_escapeBuffer == "+++") {
        m_isNetworkConnected = false;
        qDebug() << "!i [RDevice] +++ Escape Sequence triggered. Dropping to AT Mode.";
        sendResultCode(RESULT_OK);
    }
    m_escapeBuffer.clear();
}



void RDevice::onSocketReadyRead() {
    QByteArray data = tcpSocket->readAll();

    QMutexLocker locker(&m_bufferMutex);
    parseTelnet(data);

    if (state == ModemState::StreamMode) {
        if (!m_txBuffer.isEmpty()) {
            m_networkToSioBuffer.append(m_txBuffer);
            m_txBuffer.clear();
        }
    }
}

void RDevice::parseTelnet(const QByteArray &data) {
    for (char c : data) {
        unsigned char byte = (unsigned char)c;
        switch (m_telnetState) {
        case TelnetState::Normal:
            if (byte == 0xFF) m_telnetState = TelnetState::IacReceived;
            else m_txBuffer.append(c);
            break;
        case TelnetState::IacReceived:
            switch (byte) {
            case 0xFF: m_txBuffer.append((char)0xFF); m_telnetState = TelnetState::Normal; break;
            case 0xFB: m_telnetState = TelnetState::Will; break;
            case 0xFC: m_telnetState = TelnetState::Wont; break;
            case 0xFD: m_telnetState = TelnetState::Do;   break;
            case 0xFE: m_telnetState = TelnetState::Dont; break;
            case 0xFA: m_telnetState = TelnetState::SubNegotiation; break;
            default: m_telnetState = TelnetState::Normal; break;
            }
            break;
        case TelnetState::Will:
        case TelnetState::Wont:
        case TelnetState::Do:
        case TelnetState::Dont:
            // <-- The rejection logic belongs out here in the main state switch!
            if (m_telnetState == TelnetState::Will || m_telnetState == TelnetState::Do) {
                QByteArray reject;
                reject.append((char)0xFF);
                reject.append(m_telnetState == TelnetState::Will ? (char)0xFE : (char)0xFC); // DONT or WONT
                reject.append((char)byte);
                if (tcpSocket->state() == QAbstractSocket::ConnectedState) {
                    tcpSocket->write(reject);
                }
            }
            m_telnetState = TelnetState::Normal;
            break;
        case TelnetState::SubNegotiation:
            if (byte == 0xFF) m_telnetState = TelnetState::SubIac;
            break;
        case TelnetState::SubIac:
            if (byte == 0xF0) m_telnetState = TelnetState::Normal;
            else if (byte != 0xFF) m_telnetState = TelnetState::SubNegotiation;
            break;
        }
    }
}

void RDevice::processAtCommand(const QString &rawCmd) {

    QString cmd = rawCmd.trimmed().toUpper();
    if (cmd.startsWith("AT")) cmd.remove(0, 2);

    // Dial first: everything after DT is the target, so it must not be scanned
    // for the E0/E1/V0/V1 flags below -- a BBS name like "V01D C1PH3R" would
    // otherwise be swallowed by the verbose-response branch and never dialled.
    if (cmd.startsWith("DT")) {
        int dtIndex = rawCmd.toUpper().indexOf("DT");
        at_handle_dial(rawCmd.mid(dtIndex + 2).trimmed());
        return;
    }

    if (cmd.contains("E0")) {
        echoEnabled = false; cmd.replace("E0", "");
    }
    else if (cmd.contains("E1")) {
        echoEnabled = true; cmd.replace("E1", "");
    }
    if (cmd.contains("V0")) {
        verboseResponses = false; cmd.replace("V0", "");
    }
    else if (cmd.contains("V1")) {
        verboseResponses = true; cmd.replace("V1", "");
    }

    else if (cmd == "A" || cmd.startsWith("A ")) {
        if (m_ringPhase && pendingSocket) {
            m_ringTimer->stop();
            tcpSocket->disconnect(this);
            tcpSocket->deleteLater();
            tcpSocket = pendingSocket;
            pendingSocket = nullptr;
            m_ringPhase = false;
            m_isNetworkConnected = true;

            connect(tcpSocket, &QTcpSocket::connected, this, &RDevice::onSocketConnected);
            connect(tcpSocket, &QTcpSocket::disconnected, this, &RDevice::onSocketDisconnected);
            connect(tcpSocket, &QTcpSocket::readyRead, this, &RDevice::onSocketReadyRead);
            connect(tcpSocket, &QTcpSocket::errorOccurred, this, &RDevice::onSocketError);

            sendResultCode(RESULT_CONNECT);
        } else {
            sendResultCode(RESULT_ERROR);
        }
    }

    // --- S0 REGISTER (Auto-Answer) ---
    else if (cmd.startsWith("S0=")) {
        bool ok;
        int val = cmd.mid(3).trimmed().toInt(&ok);
        if (ok && val >= 0 && val <= 255) {
            m_s0Register = val;
            sendResultCode(RESULT_OK);
        } else {
            sendResultCode(RESULT_ERROR);
        }
    }
    else if (cmd == "S0?") {
        QString valStr = QString("%1\r\n").arg(m_s0Register, 3, 10, QChar('0'));
        sendAtResponse(valStr);
        sendResultCode(RESULT_OK);
    }



    else if (cmd == "H") {
        hangup();
    }
    else if (cmd == "Z") {
        {
            QMutexLocker locker(&m_bufferMutex);
            m_txBuffer.clear();
            m_networkToSioBuffer.clear();
            m_atCmdBuffer.clear();
        }
#ifdef HAVE_LIBSSH
        if (m_ssh->isConnected()) m_ssh->disconnectFromHost();
#endif
        m_isSshMode = false;
        tcpSocket->abort();

        if (pendingSocket) {
            pendingSocket->disconnect(this);
            pendingSocket->disconnectFromHost();
            pendingSocket->deleteLater();
            pendingSocket = nullptr;
            m_ringPhase = false;
            m_ringTimer->stop();
        }

        sendResultCode(RESULT_OK);
    }

    // --- RETURN TO ONLINE (ATO) ---
    else if (cmd == "O" || cmd.startsWith("O0") || cmd.startsWith("O ")) {

        bool hasActiveConnection = (tcpSocket->state() == QAbstractSocket::ConnectedState);

        if (hasActiveConnection) {
            m_isNetworkConnected = true; // Restore routing to the network

            m_plusCount = 0;
            m_escapeTimer.restart();
            if (m_escapeActionTimer->isActive()) m_escapeActionTimer->stop();

            sendResultCode(RESULT_CONNECT);
        } else {
            m_isNetworkConnected = false;
            sendResultCode(RESULT_NO_CARRIER); // Authentic Hayes failure response
        }
    }


    else if(cmd.isEmpty()) {
        sendResultCode(RESULT_OK);
    }
    else {
            qDebug() << "!w Unrecognized AT command:" << cmd;
    }
}


void RDevice::sendResultCode(int code) {
    QByteArray resp;
    if (verboseResponses) {
        if (code == RESULT_OK) resp = "\r\nOK\r\n";
        else if (code == RESULT_CONNECT) resp = "\r\nCONNECT\r\n";
        else if (code == RESULT_RING) resp = "\r\nRING\r\n";
        else if (code == RESULT_NO_CARRIER) resp = "\r\nNO CARRIER\r\n";
        else if (code == RESULT_ERROR) resp = "\r\nERROR\r\n";
    } else {
        resp = QString("%1\r").arg(code).toLatin1();
    }

    QMutexLocker locker(&m_bufferMutex);
    if (state == ModemState::StreamMode) {
        m_networkToSioBuffer.append(resp);
    } else {
        m_txBuffer.append(resp);
    }
}

// ATD<target>: dial a phonebook name, or a literal host[:port]. An "ssh:" or
// "sshauth:" prefix -- or a phonebook entry whose protocol says SSH -- picks SSH.
void RDevice::at_handle_dial(const QString &target) {

    const BbsEntry entry = m_phonebook.findByName(target);

    if (!entry.name.isEmpty()) {
        m_currentConnection = entry;
    } else {
        QString host = target;
        int port = 23;
        m_currentConnection = BbsEntry();

        const QString upper = host.toUpper();
        if (upper.startsWith("SSHSHELL:") || upper.startsWith("SSH-SHELL:")) {
            m_currentConnection.protocol = "SSH-SHELL";
            host = host.mid(host.indexOf(':') + 1);
            port = 22;
        } else if (upper.startsWith("SSHAUTH:") || upper.startsWith("SSH-AUTH:")) {
            m_currentConnection.protocol = "SSH-AUTH";
            host = host.mid(host.indexOf(':') + 1);
            port = 22;
        } else if (upper.startsWith("SSH:")) {
            m_currentConnection.protocol = "SSH";
            host = host.mid(4);
            port = 22;
        }
        if (host.contains(":")) {
            const QStringList parts = host.split(":");
            host = parts[0];
            port = parts[1].toInt();
        }
        m_currentConnection.ip = host;
        m_currentConnection.port = port;
    }

    sendAtResponse("DIALING " + m_currentConnection.ip + "...\r\n");
    startCall();
}

void RDevice::startCall() {
    const QString proto = m_currentConnection.protocol.toUpper();
    const bool wantSsh = proto.startsWith("SSH") || m_currentConnection.port == 22;

#ifdef HAVE_LIBSSH
    if (wantSsh) {
        m_isSshMode = true;
        m_sshAsk = SshAsk::None;
        m_oscState = 0;
        if (tcpSocket->state() != QAbstractSocket::UnconnectedState) tcpSocket->abort();
        if (m_ssh->isConnected()) m_ssh->disconnectFromHost();  // drop a prior call

        // SSH-SHELL: nothing is stored, so the host key and the credentials are
        // asked for on the Atari, like a real ssh client.
        if (proto == "SSH-SHELL") {
            m_ssh->connectInteractive(m_currentConnection.ip, m_currentConnection.port);
            return;
        }
        // Authenticate with the entry's login/password when it has them; an
        // empty login means anonymous (the BBS does its own login). SSH-AUTH
        // with no login falls back to "guest".
        QString user = m_currentConnection.login;
        if (proto == "SSH-AUTH" && user.isEmpty()) user = QStringLiteral("guest");
        m_ssh->connectToHost(m_currentConnection.ip, m_currentConnection.port,
                             user, m_currentConnection.password);
        return;
    }
    m_isSshMode = false;
    if (m_ssh->isConnected()) m_ssh->disconnectFromHost();
#else
    if (wantSsh) {
        sendAtResponse("\r\nERROR: SSH not available in this build\r\n");
        sendResultCode(RESULT_NO_CARRIER);
        return;
    }
#endif
    // Reset a socket left mid-connect by a previous attempt, or connectToHost()
    // errors with "already looking up or connecting".
    if (tcpSocket->state() != QAbstractSocket::UnconnectedState) tcpSocket->abort();
    tcpSocket->connectToHost(m_currentConnection.ip, m_currentConnection.port);
}

void RDevice::sendAtResponse(const QString &text) {
    QMutexLocker locker(&m_bufferMutex);
    if (state == ModemState::StreamMode) {
        m_networkToSioBuffer.append(text.toLatin1());
    } else {
        m_txBuffer.append(text.toLatin1());
    }
}

void RDevice::loadPhonebook(const QString &path) {
    m_phonebook.load(path);
}

void RDevice::forceCommandMode(bool sendAlert) {
    if (state == ModemState::StreamMode) {
        if (sendAlert) {
            QByteArray alert;
            alert.append(0x9B);
            alert.append("*** VIRTUAL MODEM POWERED OFF ***");
            alert.append(0x9B);
            alert.append("*** SIO IS OUT OF SYNC ***");
            alert.append(0x9B);

            if (sio && sio->port()) {
                sio->port()->writeRawFrame(alert);
                SioWorker::usleep(5000);
            }
        }

        state = ModemState::CommandMode;
        {
            // Keep undelivered network data; the Atari usually only stepped out
            // for some SIO and wants the rest. Hangup/ATZ clears it anyway.
            QMutexLocker locker(&m_bufferMutex);
            m_txBuffer.prepend(m_networkToSioBuffer);
            m_networkToSioBuffer.clear();
        }

        sio->onStreamFinished();
    }
}


void RDevice::onSocketConnected() {
    m_isNetworkConnected = true;
    sendResultCode(RESULT_CONNECT);
}
void RDevice::onSocketDisconnected() {
    qDebug() << "!i" << "[RDevice] TCP disconnected.";
    m_isNetworkConnected = false;
    {
        QMutexLocker locker(&m_bufferMutex);
        m_atCmdBuffer.clear();
    }
    sendResultCode(RESULT_NO_CARRIER);
}

void RDevice::onSocketError(QAbstractSocket::SocketError socketError) {
    Q_UNUSED(socketError);

    QString errorMsg = tcpSocket->errorString();
    qDebug() << "!w" << "[RDevice] socket error:" << errorMsg;

    // If we aren't connected yet (e.g. dialing failed), show the verbose error
    if (!m_isNetworkConnected) {
        sendAtResponse("\r\nERROR: " + errorMsg + "\r\n");
        sendResultCode(RESULT_NO_CARRIER);
    } else {
        // If an error happens while connected
        sendResultCode(RESULT_ERROR);
    }
}
#ifdef HAVE_LIBSSH
void RDevice::onSshConnected() {
    m_isNetworkConnected = true;
    m_sshDataSinceConnect = false;
    sendResultCode(RESULT_CONNECT);

    // SSH has no telnet TTYPE negotiation, so BBSes like Mystic detect the
    // terminal in-band and stay silent until a keypress before drawing their
    // login. Send that Enter -- but only if the server has stayed silent: one
    // that drew its login already sent data, and an Enter would submit an empty
    // username there.
    QTimer::singleShot(700, this, [this]() {
        if (m_isSshMode && m_isNetworkConnected && !m_sshDataSinceConnect)
            m_ssh->write(QByteArray("\r"));
    });
}

// The server's key, shown the way a real ssh client does. libssh has already
// compared it with our known_hosts; only an unknown or changed key needs asking.
void RDevice::onSshHostKey(const QString &fingerprint, int status) {
    if (!m_isSshMode) return;

    if (status == SSH_KNOWN_HOSTS_OK) {
        m_ssh->acceptHostKey(true);            // seen before and unchanged
        return;
    }
    if (status == SSH_KNOWN_HOSTS_CHANGED || status == SSH_KNOWN_HOSTS_OTHER) {
        sendAtResponse("\r\nWARNING: the host key has CHANGED.\r\n"
                       "Someone may be impersonating the server.\r\n"
                       + fingerprint + "\r\nAccept anyway (y/n)? ");
    } else {
        sendAtResponse("\r\nUnknown host key:\r\n" + fingerprint
                       + "\r\nAccept and remember (y/n)? ");
    }
    m_sshAsk = SshAsk::HostKey;
    m_sshAskEcho = true;
    m_sshAskBuffer.clear();
}

void RDevice::onSshPrompt(const QString &prompt, bool echo) {
    if (!m_isSshMode) return;
    sendAtResponse("\r\n" + prompt);
    m_sshAsk = SshAsk::Line;
    m_sshAskEcho = echo;
    m_sshAskBuffer.clear();
}

void RDevice::onSshAuthFailed(const QString &msg) {
    if (!m_isSshMode) return;
    m_sshAsk = SshAsk::None;
    sendAtResponse("\r\nLogin failed: " + msg + "\r\n");
    m_ssh->disconnectFromHost();
    sendResultCode(RESULT_NO_CARRIER);
}

// One byte of an answer to our own prompt. Echo is ours to control, which is
// what lets a password be typed without showing.
void RDevice::handleSshPromptByte(char c) {
    if (m_sshAsk == SshAsk::HostKey) {
        if (c == 'y' || c == 'Y' || c == 'n' || c == 'N') {
            const bool yes = (c == 'y' || c == 'Y');
            sendAtResponse(QString(QChar(c)) + "\r\n");
            m_sshAsk = SshAsk::None;
            m_ssh->acceptHostKey(yes);
            if (!yes)
                sendAtResponse("Rejected.\r\n");
        }
        return;                                 // ignore anything else
    }

    if (c == 0x0D || (quint8)c == 0x9B) {       // Return: submit the line
        const QString answer = m_sshAskBuffer;
        m_sshAskBuffer.clear();
        m_sshAsk = SshAsk::None;
        sendAtResponse("\r\n");
        m_ssh->sendAnswer(answer);
        return;
    }
    if (c == 8 || c == 126 || c == 127) {       // Backspace / Delete
        if (!m_sshAskBuffer.isEmpty()) {
            m_sshAskBuffer.chop(1);
            if (m_sshAskEcho) sendAtResponse("\b \b");
        }
        return;
    }
    m_sshAskBuffer.append(QChar::fromLatin1(c));
    if (m_sshAskEcho) sendAtResponse(QString(QChar::fromLatin1(c)));
}

void RDevice::onSshDisconnected() {
    qDebug() << "!i" << "[RDevice] SSH disconnected.";
    m_isNetworkConnected = false;
    m_sshAsk = SshAsk::None;
    if (!m_isSshMode) return;
    {
        QMutexLocker locker(&m_bufferMutex);
        m_atCmdBuffer.clear();
    }
    sendResultCode(RESULT_NO_CARRIER);
}

QByteArray RDevice::stripOsc(const QByteArray &in) {
    QByteArray out;
    out.reserve(in.size());
    for (char c : in) {
        switch (m_oscState) {
        case 0:                                     // normal text
            if (c == 0x1B) m_oscState = 1;          // hold the ESC back
            else           out.append(c);
            break;
        case 1:                                     // just after an ESC
            if (c == ']') {                         // OSC: swallow the whole thing
                m_oscState = 2;
            } else {                                // CSI and friends: let it out
                out.append(char(0x1B));
                if (c == 0x1B) break;               // ESC ESC: still pending
                out.append(c);
                m_oscState = 0;
            }
            break;
        case 2:                                     // inside the OSC body
            if (c == 0x07)      m_oscState = 0;     // BEL terminates
            else if (c == 0x1B) m_oscState = 3;     // maybe ST
            break;
        case 3:                                     // ESC inside the body
            if (c == '\\')      m_oscState = 0;     // ST terminates
            else if (c != 0x1B) m_oscState = 2;     // false alarm, keep eating
            break;
        }
    }
    return out;
}

// SSH carries no telnet negotiation, so this bypasses parseTelnet().
void RDevice::onSshDataReceived(const QByteArray &data) {
    m_sshDataSinceConnect = true;                   // the server spoke, stripped or not
    const QByteArray clean = stripOsc(data);
    if (clean.isEmpty()) return;
    QMutexLocker locker(&m_bufferMutex);
    if (state == ModemState::StreamMode)
        m_networkToSioBuffer.append(clean);
    else
        m_txBuffer.append(clean);
}

void RDevice::onSshError(const QString &msg) {
    if (!m_isSshMode) return;
    qDebug() << "!w" << "[RDevice] SSH error:" << msg;
    // An error mid-login leaves no one to answer our prompt; go back to
    // accepting AT commands rather than eating the Atari's keystrokes.
    m_sshAsk = SshAsk::None;
    if (!m_isNetworkConnected) {
        sendAtResponse("\r\nERROR: SSH - " + msg + "\r\n");
        sendResultCode(RESULT_NO_CARRIER);
    } else {
        sendResultCode(RESULT_ERROR);
    }
}
#endif

void RDevice::onNewConnection() {
    QTcpSocket *client = tcpServer->nextPendingConnection();

    // Busy check: Are we already connected, dialing, or already ringing?
    if (m_isNetworkConnected || tcpSocket->state() != QAbstractSocket::UnconnectedState || pendingSocket) {
        qDebug() << "!i [RDevice] Rejected inbound call (Busy).";
        client->disconnectFromHost();
        client->deleteLater();
        return;
    }

    // Park the caller and trigger the Ring Phase
    pendingSocket = client;
    if (m_s0Register > 0) {
        int ringDelay = m_s0Register * 2000;
        QTimer::singleShot(ringDelay, this, &RDevice::onAutoAnswerTriggered);
    } else {
        m_ringTimer->start(30000);
    }
    m_ringPhase = true;
    connect(pendingSocket, &QTcpSocket::disconnected, this, &RDevice::onPendingSocketDisconnected);

    sendResultCode(RESULT_RING);
}

void RDevice::onRingTimeout() {
    qDebug() << "!w [RDevice] Ring timeout: No ATA received within 30s. Dropping caller.";
    if (pendingSocket) pendingSocket->disconnectFromHost(); // Cleanly trigger the disconnect signal
}

void RDevice::onPendingSocketDisconnected() {
    if (pendingSocket) {
        pendingSocket->disconnect(this);
        pendingSocket->deleteLater();
        pendingSocket = nullptr;
        m_ringTimer->stop();
        m_ringPhase = false;
        sendResultCode(RESULT_NO_CARRIER);
        qDebug() << "!i [RDevice] Caller disconnected before answer.";
    }
}

QByteArray RDevice::dequeueNetworkData() {
    if (m_streamEntered.isValid() && m_streamEntered.elapsed() < kResumeHoldMs)
        return QByteArray();

    QMutexLocker locker(&m_bufferMutex);
    QByteArray data = m_networkToSioBuffer;
    m_networkToSioBuffer.clear();
    return data;
}

void RDevice::dial(const BbsEntry &entry) {
    m_currentConnection = entry;

    // 1. Clean up any existing connection
    if (tcpSocket->state() == QAbstractSocket::ConnectedState) {
        tcpSocket->disconnectFromHost();
    }

    // 2. Execute the dial
    m_isNetworkConnected = false;
    qDebug() << "!i" << tr("[RDevice] Dialing %1:%2...").arg(entry.ip).arg(entry.port);
    startCall();
}


void RDevice::injectDial(const QString &target)
{
    const QString cmd = QStringLiteral("ATDT") + target;

    // Show it on the Atari, then run it through the normal AT path. Queued, so
    // it lands on this object's thread like a command typed by the user.
    sendAtResponse(QStringLiteral("\r\n") + cmd + QStringLiteral("\r\n"));
    emit executeAtCommand(cmd);
}

void RDevice::hangup() {
    {
        QMutexLocker locker(&m_bufferMutex);
        m_txBuffer.clear();
        m_networkToSioBuffer.clear();
        m_atCmdBuffer.clear();
    }
#ifdef HAVE_LIBSSH
    if (m_ssh->isConnected()) m_ssh->disconnectFromHost();
#endif
    m_isSshMode = false;
    if (tcpSocket->state() == QAbstractSocket::ConnectedState) tcpSocket->disconnectFromHost();

    if (pendingSocket) {
        pendingSocket->disconnect(this); // Prevent signal loops
        pendingSocket->disconnectFromHost();
        pendingSocket->deleteLater();
        pendingSocket = nullptr;
        m_ringTimer->stop();
        m_ringPhase = false;
    }
    sendResultCode(RESULT_OK);
}

void RDevice::injectMacro(char macroType) {
    if (!m_isNetworkConnected) return; // Ignore if offline

    QString textToSend;
    if (macroType == 'U' || macroType == 'u') textToSend = m_currentConnection.login;
    else if (macroType == 'P' || macroType == 'p') textToSend = m_currentConnection.password;

    if (!textToSend.isEmpty()) {
        QByteArray bytes = textToSend.toUtf8() + "\r";
        tcpSocket->write(bytes);
    }
}


void RDevice::updateListenerConfig() {
    if (!aspeqtSettings) return;

    // Only listen if both the R: Device AND the BBS Listener are enabled
    bool shouldListen = aspeqtSettings->bbsListenerEnabled(m_portIndex) && m_isEnabled;
    int port = aspeqtSettings->modemListenPort(m_portIndex);

    if (shouldListen) {
        if (tcpServer->isListening()) {
            // If it's already listening but the user changed the port, restart it
            if (tcpServer->serverPort() != port) {
                tcpServer->close();
                if (tcpServer->listen(QHostAddress::Any, port)) {
                    qDebug() << "!i [RDevice] BBS listener restarted on new port:" << port;
                }
            }
        } else {
            // Start listening
            if (tcpServer->listen(QHostAddress::Any, port)) {
                qDebug() << "!i [RDevice] BBS listener started on port:" << port;
            } else {
                qDebug() << "!e [RDevice] Failed to start BBS listener on port:" << port;
            }
        }
    } else {
        // If the setting is toggled off, kill the server
        if (tcpServer->isListening()) {
            tcpServer->close();
            qDebug() << "!i [RDevice] BBS listener stopped.";
        }
    }
}


void RDevice::onAutoAnswerTriggered() {
    if (m_ringPhase && pendingSocket) {
        qDebug() << "!i [RDevice] Auto-answering call (S0=" << m_s0Register << ")";

        tcpSocket->disconnect(this);
        tcpSocket->deleteLater();

        tcpSocket = pendingSocket;
        pendingSocket = nullptr;
        m_ringPhase = false;
        m_isNetworkConnected = true;

        connect(tcpSocket, &QTcpSocket::connected, this, &RDevice::onSocketConnected);
        connect(tcpSocket, &QTcpSocket::disconnected, this, &RDevice::onSocketDisconnected);
        connect(tcpSocket, &QTcpSocket::readyRead, this, &RDevice::onSocketReadyRead);
        connect(tcpSocket, &QTcpSocket::errorOccurred, this, &RDevice::onSocketError);

        sendResultCode(RESULT_CONNECT);
    }
}
