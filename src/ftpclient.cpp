// Network file-system client (FTP). Taken from AspeQt-2k26 by
// Paul Jones <pjones1063@gmail.com>, GPL-2. See AUTHORS.txt.

#include "ftpclient.h"
#include <QRegularExpression>
#include <QSslConfiguration>
#include <QDebug>
#include <QThread>

FtpClient::FtpClient(QObject *parent) : INetworkClient(parent)
{
    m_controlSocket = new QSslSocket(this);
    // Retro/hobbyist FTPS servers commonly use self-signed certificates; a file
    // browser has no trust store to check them against, so don't verify.
    m_controlSocket->setPeerVerifyMode(QSslSocket::VerifyNone);
    // Qt throws the TLS session away once the handshake is done. Keep it: FTPS
    // servers routinely demand that the data connection *resume* the control
    // connection's session, and without it they answer LIST with 425.
    QSslConfiguration ctrlCfg = m_controlSocket->sslConfiguration();
    ctrlCfg.setSslOption(QSsl::SslOptionDisableSessionPersistence, false);
    m_controlSocket->setSslConfiguration(ctrlCfg);
    m_dataSocket = nullptr;
    m_listingFinished = true;
}

FtpClient::~FtpClient()
{
    if (m_controlSocket->state() == QAbstractSocket::ConnectedState) {
        QString res;
        sendCommand("QUIT", res);
        m_controlSocket->close();
    }
    cleanupDataSocket();
}

void FtpClient::cleanupDataSocket()
{
    if (m_dataSocket) {
        if (m_dataSocket->state() == QAbstractSocket::ConnectedState) {
            m_dataSocket->close();
        }
        m_dataSocket->deleteLater();
        m_dataSocket = nullptr;
    }
}

void FtpClient::setCredentials(const QString &user, const QString &pass) {
    m_user = user;
    m_pass = pass;
}

int FtpClient::sendCommand(const QString &cmd, QString &response)
{
    if (!cmd.isEmpty()) {
        m_controlSocket->write((cmd + "\r\n").toUtf8());
        m_controlSocket->waitForBytesWritten(3000);
    }

    response.clear();

    while (true) {
        if (!m_controlSocket->waitForReadyRead(5000)) break;
        response += m_controlSocket->readAll();

        QStringList lines = response.split("\r\n", Qt::SkipEmptyParts);
        if (!lines.isEmpty()) {
            QString lastLine = lines.last();
            if (lastLine.length() >= 4 && lastLine.at(3) == ' ') {
                break;
            }
        }
    }

    if (response.length() >= 3) {
        return response.left(3).toInt();
    }
    return 0;
}

bool FtpClient::getDataEndpoint(QHostAddress &addr, quint16 &port)
{
    QString res;

    // EPSV: the server gives only a port and the data connection reuses the
    // control connection's address. This is the only thing that works when the
    // control channel is IPv6, or when PASV would hand back a NATed/unreachable
    // IPv4 literal (e.g. an IPv6-only/NAT64 phone reaching ftp.funet.fi).
    if (sendCommand("EPSV", res) == 229) {
        QRegularExpression rx("\\(\\|\\|\\|(\\d+)\\|\\)");
        QRegularExpressionMatch m = rx.match(res);
        if (m.hasMatch()) {
            port = m.captured(1).toUShort();
            addr = m_controlSocket->peerAddress();
            return true;
        }
    }

    // Classic PASV fallback (IPv4 only) for servers without EPSV.
    if (sendCommand("PASV", res) == 227) {
        QRegularExpression rx("\\((\\d+),(\\d+),(\\d+),(\\d+),(\\d+),(\\d+)\\)");
        QRegularExpressionMatch m = rx.match(res);
        if (m.hasMatch()) {
            addr = QHostAddress(QString("%1.%2.%3.%4").arg(
                       m.captured(1), m.captured(2), m.captured(3), m.captured(4)));
            port = (m.captured(5).toUInt() << 8) + m.captured(6).toUInt();
            return true;
        }
    }
    return false;
}

bool FtpClient::connectToHost(const QString &host, quint16 port)
{
    if (port == 0) port = 21;

    m_controlSocket->connectToHost(host, port);
    if (!m_controlSocket->waitForConnected(5000)) {
        qCritical() << "!e" << "FTP: Could not connect to control port.";
        return false;
    }

    QString res;
    int code = sendCommand("", res);
    if (code != 220 ) return false;

    // Explicit FTPS: secure the control channel before sending credentials.
    if (m_tls) {
        code = sendCommand("AUTH TLS", res);
        if (code != 234) {
            qCritical() << "!e" << "FTPS: AUTH TLS refused:" << res.trimmed();
            return false;
        }
        m_controlSocket->startClientEncryption();
        if (!m_controlSocket->waitForEncrypted(5000)) {
            qCritical() << "!e" << "FTPS: control-channel TLS handshake failed.";
            return false;
        }
    }

    QString actualUser = m_user.isEmpty() ? "anonymous" : m_user;
    QString actualPass = m_pass.isEmpty() ? "aspeqt@retro.net" : m_pass;

    code = sendCommand("USER " + actualUser, res);
    if (code == 331) {
        code = sendCommand("PASS " + actualPass, res);
    }

    if (code != 230 && code != 202) {
        qCritical() << "!e" << "FTP: Login failed. Server replied:" << res.trimmed();
        return false;
    }

    if (m_tls) {
        // Protect the data channel too (PBSZ 0 is mandatory before PROT).
        sendCommand("PBSZ 0", res);
        sendCommand("PROT P", res);
    }

    sendCommand("TYPE I", res);
    return true;
}

bool FtpClient::openDataConnection()
{
    QHostAddress addr;
    quint16 port;
    if (!getDataEndpoint(addr, port)) return false;

    cleanupDataSocket();
    m_dataSocket = new QSslSocket(this);
    m_dataSocket->setPeerVerifyMode(QSslSocket::VerifyNone);
    m_dataSocket->connectToHost(addr, port);
    if (!m_dataSocket->waitForConnected(5000)) return false;

    if (m_tls) {
        // Resume the control channel's TLS session; a fresh handshake here is
        // what servers enforcing session reuse reject. Copying the whole
        // configuration is not enough -- the ticket has to be carried over.
        QSslConfiguration cfg = m_dataSocket->sslConfiguration();
        cfg.setSslOption(QSsl::SslOptionDisableSessionPersistence, false);
        cfg.setSessionTicket(m_controlSocket->sslConfiguration().sessionTicket());
        cfg.setProtocol(m_controlSocket->sslConfiguration().sessionProtocol());
        m_dataSocket->setSslConfiguration(cfg);
        m_dataSocket->startClientEncryption();
        if (!m_dataSocket->waitForEncrypted(5000)) {
            qCritical() << "!e" << "FTPS: data-channel TLS handshake failed.";
            cleanupDataSocket();
            return false;
        }
    }
    return true;
}

bool FtpClient::beginListing(const QString &path)
{
    m_directoryCache.clear();
    m_listingFinished = false;

    QString target = path.isEmpty() ? "/" : path;
    QString res;

    if (sendCommand("CWD " + target, res) >= 400) return false;

    if (!openDataConnection()) return false;

    int code = sendCommand("LIST", res);
    if (code >= 400) return false;

    QByteArray rawList;
    while (m_dataSocket->waitForReadyRead(3000)) {
        rawList.append(m_dataSocket->readAll());
    }

    cleanupDataSocket();
    sendCommand("", res);

    QString listStr = QString::fromUtf8(rawList);
    m_directoryCache = listStr.split("\n", Qt::SkipEmptyParts);

    return true;
}

QList<INetworkClient::DirectoryEntry> FtpClient::fetchNextBatch(int count)
{
    QList<DirectoryEntry> entries;

    int processed = 0;
    while (!m_directoryCache.isEmpty() && processed < count) {
        QString line = m_directoryCache.takeFirst().trimmed();
        if (line.isEmpty()) continue;

        QStringList tokens = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (tokens.size() < 4) continue;

        DirectoryEntry entry;
        bool isDir = false;
        QString name;

        const QChar t0 = tokens[0].at(0);
        const bool unixLong =
            tokens[0].size() >= 10 && tokens.size() > 8 &&
            (t0 == 'd' || t0 == '-' || t0 == 'l' || t0 == 'c' ||
             t0 == 'b' || t0 == 'p' || t0 == 's');

        if (unixLong) {
            // "perms links owner group size mon day time/year name [-> target]"
            isDir = (t0 == 'd' || t0 == 'l');        // treat symlinks as navigable
            name = QStringList(tokens.mid(8)).join(" ");
            int arrow = name.indexOf(" -> ");
            if (arrow >= 0) name = name.left(arrow); // drop the symlink target
        } else if (tokens.contains("<DIR>")) {
            isDir = true;                            // MS-DOS style directory
            name = QStringList(tokens.mid(3)).join(" ");
        } else {
            name = QStringList(tokens.mid(3)).join(" ");
        }

        if (!name.isEmpty() && name != "." && name != "..") {
            entry.name = name;
            entry.isDirectory = isDir;
            entries.append(entry);
            processed++;
        }
    }

    if (m_directoryCache.isEmpty()) {
        m_listingFinished = true;
    }

    return entries;
}

void FtpClient::endListing() {
    m_directoryCache.clear();
    m_listingFinished = true;
}

quint32 FtpClient::getFileSize(const QString &path)
{
    QString res;
    int code = sendCommand("SIZE " + path, res);
    if (code == 213) {
        return res.mid(4).trimmed().toUInt();
    }
    return 0;
}

quint32 FtpClient::getFileSize(quint8 /*handle*/) {
    return 0;
}

quint8 FtpClient::openFile(const QString &path)
{
    if (!openDataConnection()) return 0xFF;

    QString res;
    int code = sendCommand("RETR " + path, res);
    if (code >= 400) {
        cleanupDataSocket();
        return 0xFF;
    }

    return 0x01;
}

QByteArray FtpClient::readFile(quint8 handle, quint32 offset, quint32 size)
{
    Q_UNUSED(handle);
    Q_UNUSED(offset);

    if (!m_dataSocket) return QByteArray();

    while (m_dataSocket->bytesAvailable() < size && m_dataSocket->state() == QAbstractSocket::ConnectedState) {
        m_dataSocket->waitForReadyRead(100);
    }

    return m_dataSocket->read(size);
}

void FtpClient::closeFile(quint8 handle)
{
    Q_UNUSED(handle);
    cleanupDataSocket();

    QString res;
    sendCommand("", res);
}
