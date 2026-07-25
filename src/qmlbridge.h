#ifndef QMLBRIDGE_H
#define QMLBRIDGE_H

// Thin C++<->QML bridge for the QML UI (branch `qml`). Engine runs headless
// as the emulation engine; AppController drives it through its qml*() wrappers
// and mirrors its state (drive slots, loader, status, log) into a QML-friendly
// model, refreshing whenever the engine emits stateChanged().

#include <QAbstractListModel>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QTimer>
#include <QVector>

class Engine;

// One drive slot as seen by the QML delegate.
struct SlotData {
    bool sameShape(const SlotData &o) const { return hwIndex == o.hwIndex; }
    bool operator==(const SlotData &o) const {
        return hwIndex == o.hwIndex && mounted == o.mounted && isFolder == o.isFolder
            && fileName == o.fileName && typeText == o.typeText && modified == o.modified
            && writeProtected == o.writeProtected && autoCommit == o.autoCommit;
    }

    int     hwIndex = 0;        // SIO disk index (device = 0x31 + hwIndex)
    bool    mounted = false;
    bool    isFolder = false;
    QString fileName;
    QString typeText;
    bool    modified = false;
    bool    writeProtected = false;
    bool    autoCommit = false;
};

class DriveModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        HwIndexRole = Qt::UserRole + 1,
        SlotNumberRole,
        MountedRole,
        IsFolderRole,
        FileNameRole,
        TypeTextRole,
        ModifiedRole,
        WriteProtectedRole,
        AutoCommitRole,
        IsBootSlotRole,
    };
    Q_ENUM(Roles)

    explicit DriveModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setSlots(const QVector<SlotData> &list);

private:
    QVector<SlotData> m_slots;
};

class AppController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(DriveModel *drives READ drives CONSTANT)
    Q_PROPERTY(int     loaderKind        READ loaderKind        NOTIFY loaderChanged)
    Q_PROPERTY(QString loaderFileName    READ loaderFileName    NOTIFY loaderChanged)
    Q_PROPERTY(QString loaderTypeText    READ loaderTypeText    NOTIFY loaderChanged)
    Q_PROPERTY(double  loaderFill        READ loaderFill        NOTIFY loaderChanged)
    Q_PROPERTY(bool    loaderPlayEnabled READ loaderPlayEnabled NOTIFY loaderChanged)
    Q_PROPERTY(bool    loaderRetryEnabled READ loaderRetryEnabled NOTIFY loaderChanged)
    Q_PROPERTY(bool    loaderEjectEnabled READ loaderEjectEnabled NOTIFY loaderChanged)
    Q_PROPERTY(bool    loaderLoading    READ loaderLoading    NOTIFY loaderChanged)
    Q_PROPERTY(bool    loaderCasPlaying READ loaderCasPlaying NOTIFY loaderChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)
    Q_PROPERTY(bool    sioRunning READ sioRunning NOTIFY statusChanged)
    Q_PROPERTY(bool    printerOn  READ printerOn  NOTIFY statusChanged)
    Q_PROPERTY(bool    modemEnabled READ modemEnabled NOTIFY statusChanged)
    Q_PROPERTY(bool isAndroid     READ isAndroid  CONSTANT)
    Q_PROPERTY(QString logHtml    READ logHtml    NOTIFY logChanged)
    Q_PROPERTY(QString logTailHtml READ logTailHtml NOTIFY logChanged)
    Q_PROPERTY(bool    canAddSlot READ canAddSlot NOTIFY drivesChanged)
    Q_PROPERTY(QString printerText READ printerText NOTIFY printerTextChanged)
    Q_PROPERTY(QString printerTextAtascii READ printerTextAtascii NOTIFY printerTextChanged)

public:
    explicit AppController(Engine *engine, QObject *parent = nullptr);

    DriveModel *drives() { return &m_model; }

    int     loaderKind() const         { return m_loaderKind; }
    QString loaderFileName() const     { return m_loaderFileName; }
    QString loaderTypeText() const     { return m_loaderTypeText; }
    double  loaderFill() const         { return m_loaderFill; }
    bool    loaderPlayEnabled() const  { return m_loaderPlayEnabled; }
    bool    loaderRetryEnabled() const { return m_loaderRetryEnabled; }
    bool    loaderEjectEnabled() const { return m_loaderEjectEnabled; }
    bool    loaderLoading() const      { return m_loaderLoading; }
    bool    loaderCasPlaying() const   { return m_loaderCasPlaying; }

    QString statusText() const { return m_statusText; }
    bool    sioRunning() const { return m_sioRunning; }
    bool    printerOn() const  { return m_printerOn; }
    bool    modemEnabled() const;
    bool    isAndroid() const {
#ifdef Q_OS_ANDROID
        return true;
#else
        return false;
#endif
    }
    QString logHtml() const;
    QString logTailHtml() const;
    bool    canAddSlot() const { return m_canAddSlot; }
    QString printerText() const { return m_printerText; }
    QString printerTextAtascii() const { return m_printerTextAtascii; }
    Q_INVOKABLE void printerClear();
    Q_INVOKABLE bool printerSavePath(const QString &url, bool asPdf);

    // Actions from QML -> engine wrappers.
    Q_INVOKABLE void eject(int hwIndex);
    Q_INVOKABLE void removeSlot(int hwIndex);
    Q_INVOKABLE int  save(int hwIndex);
    Q_INVOKABLE int  toggleAutoCommit(int hwIndex);
    Q_INVOKABLE void toggleWriteProtect(int hwIndex);
    Q_INVOKABLE int addSlot();    // hardware index of the added slot (-1 none)
    Q_INVOKABLE void swapSlots(int fromHw, int toHw);

    Q_INVOKABLE void loaderPlay();
    Q_INVOKABLE void loaderRetry();
    Q_INVOKABLE void loaderEject();

    Q_INVOKABLE void toggleSio();
    Q_INVOKABLE void togglePrinter();
    Q_INVOKABLE void clearLog();

    // menu items
    Q_INVOKABLE void createDisk(int sectorCount, int sectorSize);
    Q_INVOKABLE void ejectAll();
    Q_INVOKABLE QVariantList modifiedDisks();
    Q_INVOKABLE void quit();
    Q_INVOKABLE QStringList recentFiles();
    Q_INVOKABLE void mountRecent(int index);

    // options window
    Q_INVOKABLE QVariantMap  loadOptions();
    Q_INVOKABLE void         applyOptions(const QVariantMap &o);
    Q_INVOKABLE QVariantList languages();

    // BBS phonebook (R: device)
    Q_INVOKABLE QVariantList phonebookEntries();
    Q_INVOKABLE bool         phonebookSave(const QVariantList &list);
    Q_INVOKABLE QString      phonebookPath();
    Q_INVOKABLE QString      phonebookUseBundled();
    Q_INVOKABLE QString      phonebookBundledPath();
    Q_INVOKABLE void         phonebookDial(const QVariantMap &entry);

    // disk viewer/editor
    Q_INVOKABLE bool         diskOpen(int hwIndex);
    Q_INVOKABLE void         diskClose();
    Q_INVOKABLE QVariantList diskEntries();
    Q_INVOKABLE QString      diskPath();
    Q_INVOKABLE bool         diskCanParent();
    Q_INVOKABLE bool         diskReadOnly();
    Q_INVOKABLE int          diskFsType();
    Q_INVOKABLE bool         diskSetFsType(int index);
    Q_INVOKABLE void         toast(const QString &text);
    // File picking: QML asks the user, then calls back with the chosen URL.
    Q_INVOKABLE QString      startDir(const QString &kind);
    Q_INVOKABLE void         mountDiskPath(int hwIndex, const QString &url);
    Q_INVOKABLE void         mountNetworkTemp(const QString &path);
    Q_INVOKABLE bool         isNetworkTempSlot(int hwIndex);
    Q_INVOKABLE QString      netTempName(int hwIndex);
    Q_INVOKABLE void         clearNetworkTemp(int hwIndex);
    Q_INVOKABLE QVariantList networkTempDisks();
    Q_INVOKABLE bool         isLoaderNetworkTemp();
    Q_INVOKABLE QString      loaderNetTempName();
    Q_INVOKABLE void         loaderEjectDiscard();
    Q_INVOKABLE bool         saveLoaderTempAs(const QString &url);
    Q_INVOKABLE void         mountFolderPath(int hwIndex, const QString &url);
    Q_INVOKABLE void         loaderLoadPath(const QString &url);
    Q_INVOKABLE bool         saveAsPath(int hwIndex, const QString &url);
    Q_INVOKABLE void         installDos(int hwIndex);
    Q_INVOKABLE void         openSessionPath(const QString &url);
    Q_INVOKABLE void         saveSessionPath(const QString &url);
    // Android SAF pickers; the result arrives via documentPicked().
    Q_INVOKABLE void         pickDocument(int reqId, const QString &mimeType);
    Q_INVOKABLE void         createDocument(int reqId, const QString &mimeType, const QString &suggestedName);
    Q_INVOKABLE void         pickFolder(int reqId);
    Q_INVOKABLE void         diskEnter(int row);
    Q_INVOKABLE void         diskParent();
    Q_INVOKABLE void         diskSetTextConversion(bool on);
    Q_INVOKABLE bool         diskExtractPath(const QVariantList &rows, const QString &url);
    Q_INVOKABLE bool         diskDelete(const QVariantList &rows);
    Q_INVOKABLE bool         diskAddFilesPath(const QString &url);

signals:
    void loaderChanged();
    void statusChanged();
    void logChanged();
    void documentPicked(int reqId, const QString &uri);
    void drivesChanged();
    void printerTextChanged();

private slots:
    void refresh();                              // pull engine state -> model
    void refreshLoader();                        // light: loader progress only
    void refreshPrinter();                       // printer text updated
    void onLogMessage(int type, const QString &msg);

private:
    Engine *m_engine;
    DriveModel  m_model;

    int     m_loaderKind = 0;
    QString m_loaderFileName;
    QString m_loaderTypeText;
    double  m_loaderFill = 0.0;
    bool    m_loaderPlayEnabled = false;
    bool    m_loaderRetryEnabled = false;
    bool    m_loaderEjectEnabled = false;
    bool    m_loaderLoading = false;
    bool    m_loaderCasPlaying = false;

    QString m_statusText;
    bool    m_sioRunning = false;
    bool    m_printerOn = false;
    bool    m_canAddSlot = true;
    // The log pane renders this as RichText, so it is re-parsed and re-laid out
    // on every change: keep it bounded and coalesce bursts (fast SIO logs
    // hundreds of lines a second, which otherwise stalls the whole UI).
    static const int kMaxLogLines = 2000;   // full history, for the log window
    static const int kTailLines   = 150;    // what the main-window pane renders
    QStringList     m_logLines;
    QString         m_lastLogLine;
    int             m_lastLogRepeat = 1;
    mutable QString m_logCache;
    mutable QString m_logTailCache;
    mutable bool    m_logDirty = true;
    QTimer          m_logNotify;
    void rebuildLogCaches() const;
    QString m_printerText;
    QString m_printerTextAtascii;
};

#endif // QMLBRIDGE_H
