// See sshclient.h. Taken from AspeQt-2k26 by Paul Jones, GPL-2;
// see AUTHORS.txt.

#include "sshclient.h"
#include <fcntl.h>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QRegularExpression> // <-- NEW: Added for filtering

// ============================================================================
// SshBackend Implementation (Background Thread Logic)
// ============================================================================

SshBackend::SshBackend(QObject *parent)
    : QObject(parent), m_session(nullptr), m_channel(nullptr), m_sftp(nullptr), m_isConnected(false), m_pollIntervalMs(10), m_currentMode(ModeTerminal)
{
}

SshBackend::~SshBackend() {
    cleanup();
}

void SshBackend::cleanup() {
    m_isConnected = false;

    if (m_sftp) {
        sftp_free(m_sftp);
        m_sftp = nullptr;
    }
    if (m_channel) {
        if (ssh_channel_is_open(m_channel)) ssh_channel_close(m_channel);
        ssh_channel_free(m_channel);
        m_channel = nullptr;
    }
    if (m_session) {
        ssh_disconnect(m_session);
        ssh_free(m_session);
        m_session = nullptr;
    }
}

void SshBackend::applyOptions(const QString &host, int port, const QString &user) {
    // Set SSH Options
    ssh_options_set(m_session, SSH_OPTIONS_HOST, host.toUtf8().constData());
    int portInt = port;
    ssh_options_set(m_session, SSH_OPTIONS_PORT, &portInt);

    if (!user.isEmpty()) {
        ssh_options_set(m_session, SSH_OPTIONS_USER, user.toUtf8().constData());
    }

    // Keep known hosts inside the app's data dir: libssh's default (~/.ssh) is
    // not writable on Android, and the interactive login needs to remember keys.
    const QString kh = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                       + QLatin1String("/known_hosts");
    QDir().mkpath(QFileInfo(kh).absolutePath());
    ssh_options_set(m_session, SSH_OPTIONS_KNOWNHOSTS, kh.toUtf8().constData());

    // ---------------------------------------------------------
    // CRITICAL FIX: The Ultimate Retro-SSH Compatibility Block
    // ---------------------------------------------------------

    // 1. KEX: Bypass buggy mlkem, add legacy DH Exchange
    const char* safe_kex = "curve25519-sha256,curve25519-sha256@libssh.org,"
                           "ecdh-sha2-nistp256,ecdh-sha2-nistp384,ecdh-sha2-nistp521,"
                           "diffie-hellman-group18-sha512,diffie-hellman-group16-sha512,"
                           "diffie-hellman-group14-sha256,diffie-hellman-group14-sha1,"
                           "diffie-hellman-group-exchange-sha1,"
                           "diffie-hellman-group1-sha1";
    ssh_options_set(m_session, SSH_OPTIONS_KEY_EXCHANGE, safe_kex);

    // 2. HOST KEYS: Add DSA (ssh-dss)
    const char* safe_hostkeys = "ssh-ed25519,ecdsa-sha2-nistp256,ecdsa-sha2-nistp384,"
                                "ecdsa-sha2-nistp521,rsa-sha2-512,rsa-sha2-256,ssh-rsa,"
                                "ssh-dss";
    ssh_options_set(m_session, SSH_OPTIONS_HOSTKEYS, safe_hostkeys);

    // 3. CIPHERS: Add Blowfish, Cast128, and Arcfour (RC4)
    const char* legacy_ciphers = "aes256-gcm@openssh.com,aes128-gcm@openssh.com,"
                                 "aes256-ctr,aes192-ctr,aes128-ctr,"
                                 "aes256-cbc,aes192-cbc,aes128-cbc,3des-cbc,"
                                 "blowfish-cbc,cast128-cbc,arcfour256,arcfour128,arcfour";
    ssh_options_set(m_session, SSH_OPTIONS_CIPHERS_C_S, legacy_ciphers);
    ssh_options_set(m_session, SSH_OPTIONS_CIPHERS_S_C, legacy_ciphers);

    // 4. MACs: Add Truncated SHA1/MD5
    const char* legacy_macs = "hmac-sha2-512,hmac-sha2-256,hmac-sha1,hmac-md5,"
                              "hmac-sha1-96,hmac-md5-96";
    ssh_options_set(m_session, SSH_OPTIONS_HMAC_C_S, legacy_macs);
    ssh_options_set(m_session, SSH_OPTIONS_HMAC_S_C, legacy_macs);
}

void SshBackend::processConnection(const QString &host, int port, const QString &user, const QString &password, const QString &privateKeyPath, SshMode mode) {
    // Ensure clean state before starting
    cleanup();

    m_currentMode = mode;
    m_session = ssh_new();
    if (!m_session) {
        emit errorOccurred("Internal Error: Failed to create SSH session structure.");
        return;
    }

    applyOptions(host, port, user);

    // Connect to Server
    int rc = ssh_connect(m_session);
    if (rc != SSH_OK) {
        emit errorOccurred(QString("SSH Connection Failed: %1").arg(ssh_get_error(m_session)));
        cleanup();
        return;
    }

    // --- AUTHENTICATION ROUTING ---
    if (!privateKeyPath.isEmpty()) {
        // 1. PUBLIC KEY AUTHENTICATION
        ssh_key privkey = nullptr;
        const char* passphrase = password.isEmpty() ? nullptr : password.toUtf8().constData();

        int import_rc = ssh_pki_import_privkey_file(privateKeyPath.toUtf8().constData(), passphrase, nullptr, nullptr, &privkey);
        if (import_rc != SSH_OK) {
            emit errorOccurred(QString("SSH Key Error: Could not load private key from %1. Check path or passphrase.").arg(privateKeyPath));
            cleanup();
            return;
        }

        rc = ssh_userauth_publickey(m_session, nullptr, privkey);
        ssh_key_free(privkey);

        if (rc != SSH_AUTH_SUCCESS) {
            emit errorOccurred(QString("SSH Key Auth Failed: %1").arg(ssh_get_error(m_session)));
            cleanup();
            return;
        }
    } else {
        // 2. STANDARD PASSWORD AUTHENTICATION
        rc = ssh_userauth_password(m_session, nullptr, password.toUtf8().constData());
        if (rc != SSH_AUTH_SUCCESS) {
            emit errorOccurred(QString("SSH Password Auth Failed: %1").arg(ssh_get_error(m_session)));
            cleanup();
            return;
        }
    }

    // --- MODE ROUTING (Terminal vs SFTP) ---
    if (m_currentMode == ModeTerminal) {
        openTerminal();

    } else if (m_currentMode == ModeSftp) {
        // Initialize SFTP Subsystem
        m_sftp = sftp_new(m_session);
        if (!m_sftp) {
            emit errorOccurred("SFTP Error: Failed to allocate session.");
            cleanup();
            return;
        }

        rc = sftp_init(m_sftp);
        if (rc != SSH_OK) {
            emit errorOccurred(QString("SFTP Init Error: %1").arg(ssh_get_error(m_session)));
            cleanup();
            return;
        }

        m_isConnected = true;
        emit connected();
        // Note: No pollLoop for SFTP. It is driven by processSftpRequest
    }
}

bool SshBackend::openTerminal() {
    m_channel = ssh_channel_new(m_session);
    if (!m_channel) {
        emit errorOccurred("SSH Channel Error: Failed to create channel.");
        cleanup();
        return false;
    }

    if (ssh_channel_open_session(m_channel) != SSH_OK) {
        emit errorOccurred(QString("SSH Session Error: %1").arg(ssh_get_error(m_session)));
        cleanup();
        return false;
    }

    // Request a PTY (Terminal). libssh's default type gives Mystic colour;
    // forcing "ansi" made it serve monochrome, so keep the default.
    if (ssh_channel_request_pty(m_channel) != SSH_OK) {
        emit errorOccurred(QString("SSH PTY Error: %1").arg(ssh_get_error(m_session)));
        cleanup();
        return false;
    }

    if (ssh_channel_request_shell(m_channel) != SSH_OK) {
        emit errorOccurred(QString("SSH Shell Error: %1").arg(ssh_get_error(m_session)));
        cleanup();
        return false;
    }

    m_isConnected = true;
    emit connected();
    QTimer::singleShot(0, this, &SshBackend::pollLoop);
    return true;
}

// ---------------------------------------------------------------------------
// Interactive login (SSH-SHELL)
// ---------------------------------------------------------------------------

void SshBackend::processInteractiveConnect(const QString &host, int port) {
    cleanup();
    m_stage = IStage::None;
    m_currentMode = ModeTerminal;

    m_session = ssh_new();
    if (!m_session) {
        emit errorOccurred("Internal Error: Failed to create SSH session structure.");
        return;
    }
    applyOptions(host, port, QString());

    if (ssh_connect(m_session) != SSH_OK) {
        emit errorOccurred(QString("SSH Connection Failed: %1").arg(ssh_get_error(m_session)));
        cleanup();
        return;
    }

    // Fingerprint of whatever answered, plus how it compares to known_hosts.
    QString fp;
    ssh_key key = nullptr;
    if (ssh_get_server_publickey(m_session, &key) == SSH_OK) {
        unsigned char *hash = nullptr;
        size_t hlen = 0;
        if (ssh_get_publickey_hash(key, SSH_PUBLICKEY_HASH_SHA256, &hash, &hlen) == 0) {
            char *text = ssh_get_fingerprint_hash(SSH_PUBLICKEY_HASH_SHA256, hash, hlen);
            if (text) {
                fp = QString::fromUtf8(text);
                ssh_string_free_char(text);
            }
            ssh_clean_pubkey_hash(&hash);
        }
        ssh_key_free(key);
    }

    const int status = ssh_session_is_known_server(m_session);
    m_stage = IStage::HostKey;
    emit hostKeyReady(fp, status);
}

void SshBackend::processHostKeyDecision(bool accept) {
    if (!m_session || m_stage != IStage::HostKey)
        return;
    if (!accept) {
        cleanup();
        emit disconnected();
        return;
    }
    // Trust on first use: remember it, so a later change is noticed.
    ssh_session_update_known_hosts(m_session);
    m_stage = IStage::User;
    emit promptNeeded(QStringLiteral("login: "), true);
}

// Ask the server's own keyboard-interactive questions one at a time. Returns
// with m_stage set to Kbdint while answers are outstanding.
void SshBackend::pumpKbdint() {
    int rc = ssh_userauth_kbdint(m_session, nullptr, nullptr);
    while (rc == SSH_AUTH_INFO) {
        m_kbdCount = ssh_userauth_kbdint_getnprompts(m_session);
        if (m_kbdCount > 0) {
            m_kbdPrompt = 0;
            char echo = 0;
            const char *p = ssh_userauth_kbdint_getprompt(m_session, 0, &echo);
            m_stage = IStage::Kbdint;
            emit promptNeeded(QString::fromUtf8(p ? p : "Password: "), echo != 0);
            return;                       // wait for the answer
        }
        rc = ssh_userauth_kbdint(m_session, nullptr, nullptr);  // informational round
    }

    if (rc == SSH_AUTH_SUCCESS) {
        finishAuth();
        return;
    }
    m_stage = IStage::None;
    emit authFailed(QString::fromUtf8(ssh_get_error(m_session)));
}

void SshBackend::finishAuth() {
    m_stage = IStage::None;
    openTerminal();
}

void SshBackend::processAuthAnswer(const QString &answer) {
    if (!m_session)
        return;

    if (m_stage == IStage::User) {
        m_pendingUser = answer.trimmed();
        ssh_options_set(m_session, SSH_OPTIONS_USER, m_pendingUser.toUtf8().constData());

        // A "none" attempt is what makes the server list its methods; some
        // accept it outright (open guest shells).
        int rc = ssh_userauth_none(m_session, nullptr);
        if (rc == SSH_AUTH_SUCCESS) {
            finishAuth();
            return;
        }
        const int methods = ssh_userauth_list(m_session, nullptr);
        if (methods & SSH_AUTH_METHOD_INTERACTIVE) {
            pumpKbdint();
        } else if (methods & SSH_AUTH_METHOD_PASSWORD) {
            m_stage = IStage::Password;
            emit promptNeeded(QStringLiteral("password: "), false);
        } else {
            m_stage = IStage::None;
            emit authFailed(QStringLiteral("server offers no password login"));
        }
        return;
    }

    if (m_stage == IStage::Password) {
        int rc = ssh_userauth_password(m_session, nullptr, answer.toUtf8().constData());
        if (rc == SSH_AUTH_SUCCESS) {
            finishAuth();
        } else {
            m_stage = IStage::None;
            emit authFailed(QString::fromUtf8(ssh_get_error(m_session)));
        }
        return;
    }

    if (m_stage == IStage::Kbdint) {
        ssh_userauth_kbdint_setanswer(m_session, m_kbdPrompt, answer.toUtf8().constData());
        if (++m_kbdPrompt < m_kbdCount) {
            char echo = 0;
            const char *p = ssh_userauth_kbdint_getprompt(m_session, m_kbdPrompt, &echo);
            emit promptNeeded(QString::fromUtf8(p ? p : ""), echo != 0);
            return;                       // more questions in this round
        }
        pumpKbdint();                // round complete: submit it
    }
}

void SshBackend::processSftpRequest(const QString &path, bool isDirectory, const QString &filter) {
    if (!m_isConnected || !m_sftp) return;

    // --- STRIP SLASHES FOR HOME DIR MAPPING ---
    QString safePath = path;
    if (safePath.startsWith('/')) safePath.remove(0, 1);
    if (safePath.endsWith('/')) safePath.chop(1); // Strip trailing slash for strict servers
    if (safePath.isEmpty()) safePath = "."; // '.' forces libssh to open the default Home folder

    if (isDirectory) {
        // --- CORRECTED: Use safePath ---
        sftp_dir dir = sftp_opendir(m_sftp, safePath.toUtf8().constData());
        if (!dir) {
            emit errorOccurred(QString("SFTP Error: Could not open directory. (%1)").arg(ssh_get_error(m_session)));
            return;
        }

        sftp_attributes attributes;
        QByteArray listing;

        // Setup the Wildcard Regex Engine
        QRegularExpression rx;
        if (!filter.isEmpty()) {
            QString safeFilter = filter;
            if (safeFilter == "*.*") safeFilter = "*";
            rx = QRegularExpression(QRegularExpression::wildcardToRegularExpression(safeFilter), QRegularExpression::CaseInsensitiveOption);
        }

        while ((attributes = sftp_readdir(m_sftp, dir)) != nullptr) {
            QString name = QString::fromUtf8(attributes->name);
            if (name != "." && name != "..") {

                // Apply the Filter
                if (!filter.isEmpty() && !rx.match(name).hasMatch()) {
                    sftp_attributes_free(attributes);
                    continue;
                }

                if (attributes->type == SSH_FILEXFER_TYPE_DIRECTORY) {
                    name.append("/");
                }
                listing.append(name.toLatin1());
                listing.append('\n');
            }
            sftp_attributes_free(attributes);
        }
        sftp_closedir(dir);

        emit dataReceived(listing);
        emit sftpTransferFinished();

    } else {
        // --- CORRECTED: Use safePath for downloads too ---
        sftp_file file = sftp_open(m_sftp, safePath.toUtf8().constData(), O_RDONLY, 0);
        if (!file) {
            emit errorOccurred(QString("SFTP Error: Could not open file. (%1)").arg(ssh_get_error(m_session)));
            return;
        }

        char buffer[4096];
        int nbytes;

        while ((nbytes = sftp_read(file, buffer, sizeof(buffer))) > 0) {
            emit dataReceived(QByteArray(buffer, nbytes));
        }

        sftp_close(file);
        emit sftpTransferFinished();
    }
}

void SshBackend::processSftpAction(const QString &path, SftpAction action) {
    if (!m_isConnected || !m_sftp) {
        emit sftpActionFinished(false, "SFTP Not Connected");
        return;
    }

    int rc = SSH_ERROR;

    // --- NEW: Strip leading slash ---
    QString safePath = path;
    if (safePath.startsWith('/')) safePath.remove(0, 1);
    if (safePath.isEmpty()) safePath = ".";

    QByteArray pathBytes = safePath.toUtf8(); // Safe byte array assignment

    if (action == ActionMkdir) {
        rc = sftp_mkdir(m_sftp, pathBytes.constData(), 0755);
    } else if (action == ActionRmdir) {
        rc = sftp_rmdir(m_sftp, pathBytes.constData());
    } else if (action == ActionDelete) {
        rc = sftp_unlink(m_sftp, pathBytes.constData());
    } else if (action == ActionCheckDir) {
        // --- NEW: Try to open the directory to prove it exists ---
        sftp_dir dir = sftp_opendir(m_sftp, pathBytes.constData());
        if (dir) {
            sftp_closedir(dir);
            rc = SSH_OK;
        } else {
            rc = SSH_ERROR;
        }
    }

    if (rc == SSH_OK) {
        emit sftpActionFinished(true, "");
    } else {
        emit sftpActionFinished(false, QString(ssh_get_error(m_session)));
    }
}



void SshBackend::processSftpRename(const QString &oldPath, const QString &newPath) {
    if (!m_isConnected || !m_sftp) {
        emit sftpActionFinished(false, "SFTP Not Connected");
        return;
    }

    // --- NEW: Strip leading slash for Home Directory mapping ---
    QString safeOld = oldPath;
    if (safeOld.startsWith('/')) safeOld.remove(0, 1);
    if (safeOld.isEmpty()) safeOld = ".";

    QString safeNew = newPath;
    if (safeNew.startsWith('/')) safeNew.remove(0, 1);
    if (safeNew.isEmpty()) safeNew = ".";

    int rc = sftp_rename(m_sftp, safeOld.toUtf8().constData(), safeNew.toUtf8().constData());

    if (rc == SSH_OK) {
        emit sftpActionFinished(true, "");
    } else {
        emit sftpActionFinished(false, QString(ssh_get_error(m_session)));
    }
}


void SshBackend::processSftpWrite(const QString &path, const QByteArray &data) {
    if (!m_isConnected || !m_sftp) {
        emit errorOccurred("SFTP Write Error: Not Connected");
        return;
    }

    // --- NEW: Strip leading slash ---
    QString safePath = path;
    if (safePath.startsWith('/')) safePath.remove(0, 1);
    if (safePath.isEmpty()) safePath = ".";

    sftp_file file = sftp_open(m_sftp, safePath.toUtf8().constData(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (!file) {
        emit errorOccurred(QString("SFTP Write Error: Could not create file. Reason: %1").arg(ssh_get_error(m_session)));
        return;
    }

    int totalWritten = 0;
    int size = data.size();
    const char* ptr = data.constData();

    while (totalWritten < size) {
        int written = sftp_write(file, ptr + totalWritten, size - totalWritten);
        if (written < 0) break;
        totalWritten += written;
    }

    sftp_close(file);

    if (totalWritten < size) {
        emit errorOccurred(QString("SFTP Write Failed: %1").arg(ssh_get_error(m_session)));
    } else {
        emit sftpTransferFinished();
    }
}


void SshBackend::processWrite(const QByteArray &data) {
    if (!m_isConnected || !m_channel) return;

    int written = ssh_channel_write(m_channel, data.constData(), data.size());
    if (written < 0) {
        emit errorOccurred(QString("SSH Write Error: %1").arg(ssh_get_error(m_session)));
        cleanup();
        emit disconnected();
    }
}

void SshBackend::processDisconnect() {
    cleanup();
    emit disconnected();
}

void SshBackend::setPollingInterval(int ms) {
    m_pollIntervalMs = ms;
}

void SshBackend::pollLoop() {
    if (!m_isConnected || !m_channel) return;

    // Check if the channel is still open
    if (ssh_channel_is_eof(m_channel) || !ssh_channel_is_open(m_channel)) {
        cleanup();
        emit disconnected();
        return;
    }

    // Non-blocking read
    char buffer[2048];
    int nbytes = ssh_channel_read_nonblocking(m_channel, buffer, sizeof(buffer), 0);

    if (nbytes > 0) {
        emit dataReceived(QByteArray(buffer, nbytes));
        // Keep reading immediately if there's data
        QTimer::singleShot(0, this, &SshBackend::pollLoop);
    } else if (nbytes < 0) {
        emit errorOccurred(QString("SSH Read Error: %1").arg(ssh_get_error(m_session)));
        cleanup();
        emit disconnected();
    } else {
        // No data available right now, poll again later
        QTimer::singleShot(m_pollIntervalMs, this, &SshBackend::pollLoop);
    }
}

// ============================================================================
// SshClient Implementation (Main Thread Wrapper)
// ============================================================================

SshClient::SshClient(QObject *parent) : QObject(parent), m_connectedStatus(false) {

    ssh_init();

    qRegisterMetaType<SshMode>("SshMode");
    qRegisterMetaType<SftpAction>("SftpAction");

    m_backend = new SshBackend();
    m_backend->moveToThread(&m_thread);

    // ---------------------------------------------------------
    // Signal Wiring (Main Thread -> Worker Thread)
    // ---------------------------------------------------------
    connect(this, &SshClient::_sigConnect, m_backend, &SshBackend::processConnection);
    connect(this, &SshClient::_sigDisconnect, m_backend, &SshBackend::processDisconnect);
    connect(this, &SshClient::_sigWrite, m_backend, &SshBackend::processWrite);
    connect(this, &SshClient::_sigSftpRequest, m_backend, &SshBackend::processSftpRequest);
    connect(this, &SshClient::_sigSftpAction, m_backend, &SshBackend::processSftpAction);
    connect(this, &SshClient::_sigSftpWrite, m_backend, &SshBackend::processSftpWrite);
    connect(this, &SshClient::_sigSftpRename, m_backend, &SshBackend::processSftpRename);
    connect(this, &SshClient::_sigInteractiveConnect, m_backend, &SshBackend::processInteractiveConnect);
    connect(this, &SshClient::_sigHostKeyDecision, m_backend, &SshBackend::processHostKeyDecision);
    connect(this, &SshClient::_sigAuthAnswer, m_backend, &SshBackend::processAuthAnswer);

    // ---------------------------------------------------------
    // Signal Wiring (Worker Thread -> Main Thread)
    // ---------------------------------------------------------
    connect(m_backend, &SshBackend::connected, this, [this]() {
        m_connectedStatus = true;
        emit connected();
    });

    connect(m_backend, &SshBackend::disconnected, this, [this]() {
        m_connectedStatus = false;
        emit disconnected();
    });

    connect(m_backend, &SshBackend::errorOccurred, this, &SshClient::error);
    connect(m_backend, &SshBackend::dataReceived, this, &SshClient::rxData);
    connect(m_backend, &SshBackend::sftpTransferFinished, this, &SshClient::sftpFinished);
    connect(m_backend, &SshBackend::sftpActionFinished, this, &SshClient::sftpActionFinished);
    connect(m_backend, &SshBackend::hostKeyReady, this, &SshClient::hostKeyReady);
    connect(m_backend, &SshBackend::promptNeeded, this, &SshClient::promptNeeded);
    connect(m_backend, &SshBackend::authFailed, this, &SshClient::authFailed);

    // ---------------------------------------------------------
    // Thread Lifecycle
    // ---------------------------------------------------------
    connect(&m_thread, &QThread::finished, m_backend, &QObject::deleteLater);

    // Start the event loop for the worker
    m_thread.start();
}

SshClient::~SshClient() {
    disconnectFromHost();
    m_thread.quit();
    m_thread.wait();
    ssh_finalize();
}

void SshClient::connectToHost(const QString &host, int port, const QString &user, const QString &password, const QString &privateKeyPath, SshMode mode) {
    emit _sigConnect(host, port, user, password, privateKeyPath, mode);
}

void SshClient::connectInteractive(const QString &host, int port) {
    emit _sigInteractiveConnect(host, port);
}

void SshClient::acceptHostKey(bool accept) {
    emit _sigHostKeyDecision(accept);
}

void SshClient::sendAnswer(const QString &answer) {
    emit _sigAuthAnswer(answer);
}

void SshClient::disconnectFromHost() {
    emit _sigDisconnect();
}

void SshClient::write(const QByteArray &data) {
    emit _sigWrite(data);
}

// --- UPDATED ---
void SshClient::requestSftp(const QString &path, bool isDirectory, const QString &filter) {
    emit _sigSftpRequest(path, isDirectory, filter);
}

void SshClient::requestSftpAction(const QString &path, SftpAction action) {
    emit _sigSftpAction(path, action);
}

void SshClient::requestSftpWrite(const QString &path, const QByteArray &data) {
    emit _sigSftpWrite(path, data);
}

void SshClient::requestSftpRename(const QString &oldPath, const QString &newPath) {
    emit _sigSftpRename(oldPath, newPath);
}

bool SshClient::isConnected() const {
    return m_connectedStatus;
}
