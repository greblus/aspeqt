#include "networkbrowser.h"
#include "inetworkclient.h"
#include "tnfsclient.h"
#include "ftpclient.h"
#ifdef HAVE_LIBSSH
#include "sftpclient.h"
#endif

#include <QUrl>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>
#include <QVariantMap>
#include <algorithm>

// Atari images the Mount action can hand to a drive slot; anything else is a
// plain file the user can only download.
static bool looksLikeDisk(const QString &name)
{
    static const QStringList ext = {".atr", ".atx", ".xfd", ".pro", ".dcm",
                                    ".cas", ".xex", ".com", ".bin"};
    const QString lower = name.toLower();
    for (const QString &e : ext)
        if (lower.endsWith(e))
            return true;
    return false;
}

// Let the user type a bare host. A leading "ftp." (or "ftps.") guesses FTP,
// "sftp." guesses SFTP; anything else defaults to TNFS.
static QString normalizeUrl(const QString &raw)
{
    QString t = raw.trimmed();
    if (t.isEmpty() || t.contains("://")) return t;
    if (t.startsWith("ftps.", Qt::CaseInsensitive)) return "ftps://" + t;
    if (t.startsWith("ftp.",  Qt::CaseInsensitive)) return "ftp://"  + t;
    if (t.startsWith("sftp.", Qt::CaseInsensitive)) return "sftp://" + t;
    return "tnfs://" + t;
}

// ---------------------------------------------------------------------------
// NetworkWorker: owns the INetworkClient and does every blocking operation on
// the browser's worker thread. It talks to NetworkBrowser only through queued
// signals, so the GUI thread never blocks on the network.
// ---------------------------------------------------------------------------
class NetworkWorker : public QObject
{
    Q_OBJECT
public:
    ~NetworkWorker() override { teardown(); }

public slots:
    void doOpen(const QString &url)
    {
        teardown();
        const QString text = normalizeUrl(url);
        const QUrl u(text);
        const QString scheme = u.scheme().toLower();
        const QString host = u.host();
        const int port = u.port(0);
        const QString path = u.path().isEmpty() ? "/" : u.path();

        if (host.isEmpty()) { emit failed(tr("No host in the address.")); return; }

        if (scheme == "ftp" || scheme == "ftps") {
            auto *f = new FtpClient();
            f->setCredentials(u.userName(QUrl::FullyDecoded), u.password(QUrl::FullyDecoded));
            f->setTls(scheme == "ftps");
            m_client = f;
        } else if (scheme == "sftp") {
#ifdef HAVE_LIBSSH
            auto *s = new SftpClient();
            s->setCredentials(u.userName(QUrl::FullyDecoded), u.password(QUrl::FullyDecoded));
            m_client = s;
#else
            emit failed(tr("SFTP is not available in this build."));
            return;
#endif
        } else {
            m_client = new TnfsClient();   // default and "tnfs"
        }

        if (!m_client->connectToHost(host, quint16(port < 0 ? 0 : port))) {
            teardown();
            emit failed(tr("Could not connect to %1.").arg(host));
            return;
        }
        // TNFS needs an explicit MOUNT (session id) before any dir op; the
        // INetworkClient interface has no mount(), so do it on the concrete type.
        if (auto *t = qobject_cast<TnfsClient*>(m_client)) {
            if (!t->mount("/")) {
                teardown();
                emit failed(tr("TNFS mount failed on %1.").arg(host));
                return;
            }
        }
        emit opened(true, host, path);
        listPath(path);
    }

    void doList(const QString &path) { listPath(path); }

    void doDownload(const QString &remotePath, const QString &name,
                    const QString &localDir, int action)
    {
        if (!m_client) { emit failed(tr("Not connected.")); return; }

        QString dirPath = localDir;
        if (dirPath.startsWith("file:")) dirPath = QUrl(dirPath).toLocalFile();
        if (dirPath.isEmpty()) {
            // Mounts (action 1) go to a throwaway cache the engine cleans up on
            // eject; an explicit Download (action 0) goes to the user's folder.
            if (action == 1)
                dirPath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                          + QLatin1String("/netmount");
            else
                dirPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        }
        if (dirPath.isEmpty())
            dirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir dir(dirPath);
        dir.mkpath(".");
        const QString outPath = dir.filePath(name);
        QFile out(outPath);
        if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            emit failed(tr("Cannot write %1.").arg(outPath));
            return;
        }

        // Read fixed chunks until a short/empty read signals EOF. getFileSize is
        // unreliable across servers (LSEEK returns more than a bare position,
        // STAT varies), and reading to EOF needs neither the size nor a seek.
        const quint32 CHUNK = 512;
        const quint8 h = m_client->openFile(remotePath);
        if (h == 0xFF) {
            out.close(); out.remove();
            emit failed(tr("Cannot open %1.").arg(name));
            return;
        }
        quint32 off = 0;
        for (int guard = 0; guard < 1000000; ++guard) {
            const QByteArray chunk = m_client->readFile(h, off, CHUNK);
            if (chunk.isEmpty()) break;               // EOF or error
            out.write(chunk);
            off += quint32(chunk.size());
            if (quint32(chunk.size()) < CHUNK) break; // short read = EOF
        }
        m_client->closeFile(h);
        out.close();

        if (off == 0) {
            out.remove();
            emit failed(tr("Download of %1 failed (empty).").arg(name));
            return;
        }
        emit downloaded(outPath, name, action);
    }

    void doClose() { teardown(); }

signals:
    void opened(bool ok, const QString &host, const QString &path);
    void listed(const QVariantList &entries, const QString &path);
    void failed(const QString &message);
    void downloaded(const QString &localPath, const QString &name, int action);

private:
    void teardown()
    {
        if (m_client) { delete m_client; m_client = nullptr; }
    }

    void listPath(const QString &path)
    {
        if (!m_client) { emit failed(tr("Not connected.")); return; }
        if (!m_client->beginListing(path)) {
            emit failed(tr("Could not open %1.").arg(path));
            return;
        }
        QVariantList entries;
        for (int guard = 0; guard < 10000; ++guard) {
            const QList<INetworkClient::DirectoryEntry> batch = m_client->fetchNextBatch(40);
            for (const auto &e : batch) {
                if (e.name == ".") continue;
                QVariantMap m;
                m["name"]   = e.name;
                m["isDir"]  = e.isDirectory;
                m["isDisk"] = !e.isDirectory && looksLikeDisk(e.name);
                entries.append(m);
            }
            if (m_client->isListingFinished()) break;
            if (batch.isEmpty()) break;
        }
        m_client->endListing();

        // Directories first, then files, each alphabetical.
        std::sort(entries.begin(), entries.end(), [](const QVariant &a, const QVariant &b){
            const QVariantMap ma = a.toMap(), mb = b.toMap();
            if (ma["isDir"].toBool() != mb["isDir"].toBool())
                return ma["isDir"].toBool();
            return ma["name"].toString().compare(mb["name"].toString(), Qt::CaseInsensitive) < 0;
        });
        emit listed(entries, path);
    }

    INetworkClient *m_client = nullptr;
};

// ---------------------------------------------------------------------------
// NetworkBrowser (GUI thread)
// ---------------------------------------------------------------------------
NetworkBrowser::NetworkBrowser(QObject *parent) : QObject(parent)
{
    m_worker = new NetworkWorker;
    m_worker->moveToThread(&m_thread);
    connect(&m_thread, &QThread::finished, m_worker, &QObject::deleteLater);

    connect(this, &NetworkBrowser::reqOpen,     m_worker, &NetworkWorker::doOpen);
    connect(this, &NetworkBrowser::reqList,     m_worker, &NetworkWorker::doList);
    connect(this, &NetworkBrowser::reqDownload, m_worker, &NetworkWorker::doDownload);
    connect(this, &NetworkBrowser::reqClose,    m_worker, &NetworkWorker::doClose);

    connect(m_worker, &NetworkWorker::opened,     this, &NetworkBrowser::onOpened);
    connect(m_worker, &NetworkWorker::listed,     this, &NetworkBrowser::onListed);
    connect(m_worker, &NetworkWorker::failed,     this, &NetworkBrowser::onFailed);
    connect(m_worker, &NetworkWorker::downloaded, this, &NetworkBrowser::onDownloaded);

    loadPrefs();
    m_thread.start();
}

NetworkBrowser::~NetworkBrowser()
{
    m_thread.quit();
    m_thread.wait();
}

void NetworkBrowser::setBusy(bool b)
{
    if (m_busy == b) return;
    m_busy = b;
    emit busyChanged();
}

void NetworkBrowser::open(const QString &url)
{
    const QString text = normalizeUrl(url);
    if (text.isEmpty()) return;
    const QUrl u(text);
    m_scheme = u.scheme().toLower();
    if (m_scheme.isEmpty()) m_scheme = "tnfs";
    m_pendingConnectUrl = text;

    m_entries.clear();
    emit entriesChanged();
    setBusy(true);
    emit reqOpen(text);
}

QString NetworkBrowser::currentUrl() const
{
    if (m_host.isEmpty()) return QString();
    return m_scheme + "://" + m_host + m_path;
}

void NetworkBrowser::enter(const QString &name)
{
    if (!m_connected) return;
    if (!m_path.endsWith('/')) m_path += '/';
    m_path += name;
    emit pathChanged();
    setBusy(true);
    emit reqList(m_path);
}

void NetworkBrowser::up()
{
    if (!m_connected || m_path == "/" || m_path.isEmpty()) return;
    QString p = m_path;
    if (p.endsWith('/')) p.chop(1);
    const int slash = p.lastIndexOf('/');
    m_path = (slash <= 0) ? "/" : p.left(slash);
    emit pathChanged();
    setBusy(true);
    emit reqList(m_path);
}

void NetworkBrowser::refresh()
{
    if (!m_connected) return;
    setBusy(true);
    emit reqList(m_path);
}

void NetworkBrowser::close()
{
    emit reqClose();
    m_entries.clear();
    m_connected = false;
    m_host.clear();
    m_path = "/";
    setBusy(false);
    emit entriesChanged();
    emit connectedChanged();
    emit pathChanged();
}

void NetworkBrowser::mount(const QString &name)
{
    if (!m_connected) return;
    QString remote = m_path;
    if (!remote.endsWith('/')) remote += '/';
    remote += name;
    setBusy(true);
    emit reqDownload(remote, name, QString(), 1);
}

void NetworkBrowser::save(const QString &name)
{
    if (!m_connected) return;
    QString remote = m_path;
    if (!remote.endsWith('/')) remote += '/';
    remote += name;
    setBusy(true);
    emit reqDownload(remote, name, QString(), 0);
}

void NetworkBrowser::onOpened(bool ok, const QString &host, const QString &path)
{
    m_connected = ok;
    m_host = host;
    m_path = path;
    if (ok) {
        pushHistory(m_pendingConnectUrl);
        rememberLocation();
    }
    emit connectedChanged();
    emit pathChanged();
    // busy stays true until the follow-up listed() arrives.
}

void NetworkBrowser::onListed(const QVariantList &entries, const QString &path)
{
    m_entries = entries;
    m_path = path;
    rememberLocation();
    setBusy(false);
    emit entriesChanged();
    emit pathChanged();
}

void NetworkBrowser::onFailed(const QString &message)
{
    setBusy(false);
    emit error(message);
}

void NetworkBrowser::onDownloaded(const QString &localPath, const QString &name, int action)
{
    Q_UNUSED(name)
    setBusy(false);
    if (action == 1) emit mounted(localPath);
    else             emit saved(localPath);
}

void NetworkBrowser::reopenLast()
{
    if (m_connected || m_busy) return;
    if (!m_lastUrl.isEmpty())
        open(m_lastUrl);
}

bool NetworkBrowser::isFavorite(const QString &url) const
{
    return m_favorites.contains(url.trimmed());
}

void NetworkBrowser::toggleFavorite(const QString &url)
{
    const QString u = url.trimmed();
    if (u.isEmpty()) return;
    if (m_favorites.contains(u)) m_favorites.removeAll(u);
    else                         m_favorites.prepend(u);
    savePrefs();
    emit favoritesChanged();
}

void NetworkBrowser::pushHistory(const QString &url)
{
    const QString u = url.trimmed();
    if (u.isEmpty()) return;
    m_history.removeAll(u);
    m_history.prepend(u);
    while (m_history.size() > 20)
        m_history.removeLast();
    savePrefs();
    emit historyChanged();
}

void NetworkBrowser::rememberLocation()
{
    const QString u = currentUrl();
    if (u.isEmpty() || u == m_lastUrl) return;
    m_lastUrl = u;
    savePrefs();
}

void NetworkBrowser::loadPrefs()
{
    QSettings s;
    s.beginGroup("NetworkBrowser");
    m_history   = s.value("history").toStringList();
    m_favorites = s.value("favorites").toStringList();
    m_lastUrl   = s.value("lastUrl").toString();
    s.endGroup();
}

void NetworkBrowser::savePrefs()
{
    QSettings s;
    s.beginGroup("NetworkBrowser");
    s.setValue("history", m_history);
    s.setValue("favorites", m_favorites);
    s.setValue("lastUrl", m_lastUrl);
    s.endGroup();
}

#include "networkbrowser.moc"
