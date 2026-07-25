// SSH transport for the R: device: terminal sessions and SFTP file browsing.
//
// Taken from AspeQt-2k26 by Paul Jones <pjones1063@gmail.com>, GPL-2.
// See AUTHORS.txt.

#ifndef SSHCLIENT_H
#define SSHCLIENT_H

#include <QObject>
#include <QThread>
#include <QByteArray>
#include <QTimer>
#include <libssh/libssh.h>
#include <libssh/sftp.h>

enum SshMode {
    ModeTerminal,
    ModeSftp
};

enum SftpAction {
    ActionMkdir,
    ActionRmdir,
    ActionDelete,
    ActionCheckDir
};

// ============================================================================
// Internal Worker Class (Runs in background thread)
// ============================================================================
class SshBackend : public QObject {
    Q_OBJECT

public:
    explicit SshBackend(QObject *parent = nullptr);
    ~SshBackend();

public slots:
    // Actions triggered by the main thread
    void processConnection(const QString &host, int port, const QString &user, const QString &password, const QString &privateKeyPath, SshMode mode);
    // Interactive ("SSH-SHELL") login: the handshake stops so the caller can show
    // the host-key fingerprint and collect credentials from the Atari, the way a
    // real ssh client does. Each stage answers with a signal and waits.
    void processInteractiveConnect(const QString &host, int port);
    void processHostKeyDecision(bool accept);
    void processAuthAnswer(const QString &answer);
    void processWrite(const QByteArray &data);
    void processDisconnect();
    void setPollingInterval(int ms);
    void processSftpRequest(const QString &path, bool isDirectory, const QString &filter); // <-- UPDATED
    void processSftpAction(const QString &path, SftpAction action);
    void processSftpWrite(const QString &path, const QByteArray &data);
    void processSftpRename(const QString &oldPath, const QString &newPath);

signals:
    // Signals sent back to the main thread
    void connected();
    void disconnected();
    void errorOccurred(const QString &msg);
    void dataReceived(const QByteArray &data);
    void sftpTransferFinished();
    void sftpActionFinished(bool success, const QString &errorMsg);
    // Interactive login. status is an SSH_KNOWN_HOSTS_* value; prompts carry the
    // server's own wording (keyboard-interactive can ask more than once, e.g. 2FA).
    void hostKeyReady(const QString &fingerprint, int status);
    void promptNeeded(const QString &prompt, bool echo);
    void authFailed(const QString &msg);

private slots:
    void pollLoop(); // Non-blocking read loop

private:
    // Where an interactive login has got to, so the answer coming back from the
    // Atari can be routed to the right libssh call.
    enum class IStage { None, HostKey, User, Password, Kbdint };

    ssh_session m_session;
    ssh_channel m_channel;
    bool m_isConnected;
    int m_pollIntervalMs;
    sftp_session m_sftp;
    SshMode m_currentMode;

    IStage m_stage = IStage::None;
    QString m_pendingUser;
    int m_kbdPrompt = 0;      // index of the kbdint prompt being answered
    int m_kbdCount = 0;

    // Helper to clean up libssh structs
    void cleanup();
    // Shared setup: legacy algorithm lists, host, port, user, known_hosts path.
    void applyOptions(const QString &host, int port, const QString &user);
    // PTY + shell + polling; used by both the stored-credential and interactive paths.
    bool openTerminal();
    // Drive keyboard-interactive: emit the next prompt, or finish the round.
    void pumpKbdint();
    void finishAuth();        // auth succeeded -> open the terminal
};

// ============================================================================
// Public Interface Class (The "Wrapper" you use in ModemBridge)
// ============================================================================
class SshClient : public QObject {
    Q_OBJECT

public:
    explicit SshClient(QObject *parent = nullptr);
    ~SshClient();

    // -- Public API --
    void connectToHost(const QString &host, int port = 22, const QString &user = "", const QString &password = "", const QString &privateKeyPath = "", SshMode mode = ModeTerminal);
    // Interactive login: emits hostKeyReady(), then promptNeeded() for the user
    // name, password and any keyboard-interactive questions.
    void connectInteractive(const QString &host, int port = 22);
    void acceptHostKey(bool accept);
    void sendAnswer(const QString &answer);
    void requestSftp(const QString &path, bool isDirectory, const QString &filter = ""); // <-- UPDATED
    void requestSftpAction(const QString &path, SftpAction action);
    void requestSftpWrite(const QString &path, const QByteArray &data);
    void requestSftpRename(const QString &oldPath, const QString &newPath);
    void disconnectFromHost();
    void write(const QByteArray &data);
    bool isConnected() const;

signals:
    // Signals for your GUI / ModemBridge
    void connected();
    void disconnected();
    void error(const QString &message);
    void rxData(const QByteArray &data);
    void sftpFinished();
    void sftpActionFinished(bool success, const QString &errorMsg);
    void hostKeyReady(const QString &fingerprint, int status);
    void promptNeeded(const QString &prompt, bool echo);
    void authFailed(const QString &message);

private:
    // Internal Thread Management
    QThread m_thread;
    SshBackend *m_backend;
    bool m_connectedStatus;

signals:
    void _sigConnect(const QString &host, int port, const QString &user, const QString &password, const QString &privateKeyPath, SshMode mode);
    void _sigSftpRequest(const QString &path, bool isDirectory, const QString &filter); // <-- UPDATED
    void _sigSftpAction(const QString &path, SftpAction action);
    void _sigSftpWrite(const QString &path, const QByteArray &data);
    void _sigSftpRename(const QString &oldPath, const QString &newPath);
    void _sigWrite(const QByteArray &data);
    void _sigDisconnect();
    void _sigInteractiveConnect(const QString &host, int port);
    void _sigHostKeyDecision(bool accept);
    void _sigAuthAnswer(const QString &answer);
};

#endif // SSHCLIENT_H