// Network file browser for the QML UI: connects to a TNFS/SFTP/FTP server and
// lists directories through the INetworkClient backend (ported from AspeQt-2k26).
// All blocking network I/O runs on a worker thread so the UI never freezes on a
// slow or unreachable server; this object only marshals results back to QML.
#ifndef NETWORKBROWSER_H
#define NETWORKBROWSER_H

#include <QObject>
#include <QThread>
#include <QStringList>
#include <QVariantList>

class NetworkWorker;

class NetworkBrowser : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList entries READ entries NOTIFY entriesChanged)
    Q_PROPERTY(QString path        READ path       NOTIFY pathChanged)
    Q_PROPERTY(QString host        READ host       NOTIFY pathChanged)
    Q_PROPERTY(QString currentUrl  READ currentUrl NOTIFY pathChanged)
    Q_PROPERTY(bool    connected   READ connected  NOTIFY connectedChanged)
    Q_PROPERTY(bool    busy        READ busy       NOTIFY busyChanged)
    Q_PROPERTY(QStringList history   READ history   NOTIFY historyChanged)
    Q_PROPERTY(QStringList favorites READ favorites NOTIFY favoritesChanged)

public:
    explicit NetworkBrowser(QObject *parent = nullptr);
    ~NetworkBrowser() override;

    QVariantList entries() const { return m_entries; }
    QString path() const { return m_path; }
    QString host() const { return m_host; }
    QString currentUrl() const;
    bool connected() const { return m_connected; }
    bool busy() const { return m_busy; }
    QStringList history() const { return m_history; }
    QStringList favorites() const { return m_favorites; }

    // url: tnfs://host[:port]/path, sftp://user:pass@host/path, ftp://host/path.
    Q_INVOKABLE void open(const QString &url);
    Q_INVOKABLE void enter(const QString &name);   // into a subdirectory
    Q_INVOKABLE void up();                          // to the parent directory
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void close();
    Q_INVOKABLE void mount(const QString &name);    // download, then emit mounted()
    Q_INVOKABLE void save(const QString &name);     // download, then emit saved()

    // Reconnect to the last location if idle (used when the window reopens).
    Q_INVOKABLE void reopenLast();
    Q_INVOKABLE bool isFavorite(const QString &url) const;
    Q_INVOKABLE void toggleFavorite(const QString &url);

signals:
    void entriesChanged();
    void pathChanged();
    void connectedChanged();
    void busyChanged();
    void historyChanged();
    void favoritesChanged();
    void error(const QString &message);
    void mounted(const QString &localPath);         // QML mounts it into a drive
    void saved(const QString &localPath);           // QML shows a "saved" notice

    // Internal: dispatched (queued) to the worker thread.
    void reqOpen(const QString &url);
    void reqList(const QString &path);
    void reqDownload(const QString &remotePath, const QString &name,
                     const QString &localDir, int action);   // 1 = mount, 0 = save
    void reqClose();
    // Reconnect and list `path`: used when a listing fails because the session
    // died while the app sat in the background.
    void reqReconnect(const QString &url, const QString &path);

private slots:
    void onOpened(bool ok, const QString &host, const QString &path);
    void onListed(const QVariantList &entries, const QString &path);
    void onFailed(const QString &message);
    void onListFailed(const QString &path, const QString &message);
    void onDownloaded(const QString &localPath, const QString &name, int action);

private:
    void setBusy(bool b);
    void loadPrefs();
    void savePrefs();
    void pushHistory(const QString &url);
    void rememberLocation();

    QThread m_thread;
    NetworkWorker *m_worker = nullptr;
    QString m_scheme = "tnfs";
    QString m_host;
    QString m_path = "/";
    QVariantList m_entries;
    bool m_connected = false;
    bool m_busy = false;

    // Persisted across runs.
    QStringList m_history;
    QStringList m_favorites;
    QString m_lastUrl;
    QString m_pendingConnectUrl;   // added to history once the connect succeeds
    // One silent reconnect per failed listing, so a dropped session recovers
    // instead of leaving the browser stuck on a location it can no longer read.
    bool m_reconnectTried = false;
};

#endif // NETWORKBROWSER_H
