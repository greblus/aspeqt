// Network file-system client (FTP). Taken from AspeQt-2k26 by
// Paul Jones <pjones1063@gmail.com>, GPL-2. See AUTHORS.txt.

/*
 * ftpclient.h
 * Lightweight FTP Client for AspeQt-2k26
 */
#ifndef FTPCLIENT_H
#define FTPCLIENT_H

#include "inetworkclient.h"
#include <QSslSocket>
#include <QHostAddress>
#include <QStringList>

class FtpClient : public INetworkClient
{
    Q_OBJECT
public:
    explicit FtpClient(QObject *parent = nullptr);
    ~FtpClient() override;

    bool connectToHost(const QString &host, quint16 port = 0) override;

    void setCredentials(const QString &user, const QString &pass);
    // Explicit FTPS (AUTH TLS on the control channel, PROT P on the data one).
    void setTls(bool on) { m_tls = on; }

    bool beginListing(const QString &path) override;
    QList<DirectoryEntry> fetchNextBatch(int count) override;
    void endListing() override;
    bool isListingFinished() const override { return m_listingFinished; }

    quint32 getFileSize(const QString &path) override;
    quint32 getFileSize(quint8 handle) override;
    quint8 openFile(const QString &path) override;

    Q_INVOKABLE QByteArray readFile(quint8 handle, quint32 offset, quint32 size) override;
    Q_INVOKABLE void closeFile(quint8 handle) override;

private:
    QSslSocket *m_controlSocket;
    QSslSocket *m_dataSocket;

    bool m_listingFinished;
    bool m_tls = false;
    QString m_currentPath;

    QString m_user;
    QString m_pass;

    QStringList m_directoryCache;

    int sendCommand(const QString &cmd, QString &response);
    // Resolve a passive-mode data endpoint: EPSV first (works over IPv4 and
    // IPv6, data host = control peer), falling back to classic PASV.
    bool getDataEndpoint(QHostAddress &addr, quint16 &port);
    // Open a passive data connection, upgrading it to TLS when in FTPS mode.
    bool openDataConnection();
    void cleanupDataSocket();
};

#endif // FTPCLIENT_H