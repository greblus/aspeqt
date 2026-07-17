#ifndef QMLBRIDGE_H
#define QMLBRIDGE_H

// Thin C++<->QML bridge for the QML UI (branch `qml`). MainWindow runs headless
// as the emulation engine; AppController drives it through its qml*() wrappers
// and mirrors its state (drive slots, loader, status, log) into a QML-friendly
// model, refreshing whenever the engine emits qmlChanged().

#include <QAbstractListModel>
#include <QObject>
#include <QString>
#include <QVector>

class MainWindow;

// One drive slot as seen by the QML delegate.
struct SlotData {
    int     hwIndex = 0;        // SIO disk index (device = 0x31 + hwIndex)
    bool    mounted = false;
    bool    isFolder = false;
    QString fileName;
    QString typeText;
    bool    modified = false;
    bool    writeProtected = false;
    bool    autoCommit = false;
    bool    editOpen = false;
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
        EditOpenRole,
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
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)
    Q_PROPERTY(bool    sioRunning READ sioRunning NOTIFY statusChanged)
    Q_PROPERTY(bool    printerOn  READ printerOn  NOTIFY statusChanged)
    Q_PROPERTY(QString logHtml    READ logHtml    NOTIFY logChanged)
    Q_PROPERTY(bool    canAddSlot READ canAddSlot NOTIFY drivesChanged)

public:
    explicit AppController(MainWindow *engine, QObject *parent = nullptr);

    DriveModel *drives() { return &m_model; }

    int     loaderKind() const         { return m_loaderKind; }
    QString loaderFileName() const     { return m_loaderFileName; }
    QString loaderTypeText() const     { return m_loaderTypeText; }
    double  loaderFill() const         { return m_loaderFill; }
    bool    loaderPlayEnabled() const  { return m_loaderPlayEnabled; }
    bool    loaderRetryEnabled() const { return m_loaderRetryEnabled; }
    bool    loaderEjectEnabled() const { return m_loaderEjectEnabled; }

    QString statusText() const { return m_statusText; }
    bool    sioRunning() const { return m_sioRunning; }
    bool    printerOn() const  { return m_printerOn; }
    QString logHtml() const    { return m_logHtml; }
    bool    canAddSlot() const { return m_canAddSlot; }

    // Actions from QML -> engine wrappers.
    Q_INVOKABLE void mountDisk(int hwIndex);
    Q_INVOKABLE void mountFolder(int hwIndex);
    Q_INVOKABLE void eject(int hwIndex);
    Q_INVOKABLE void removeSlot(int hwIndex);
    Q_INVOKABLE void save(int hwIndex);
    Q_INVOKABLE void toggleAutoCommit(int hwIndex);
    Q_INVOKABLE void openEditor(int hwIndex);
    Q_INVOKABLE void toggleWriteProtect(int hwIndex);
    Q_INVOKABLE void bootOptions();
    Q_INVOKABLE void addSlot();
    Q_INVOKABLE void swapSlots(int fromHw, int toHw);

    Q_INVOKABLE void loaderLoad();
    Q_INVOKABLE void loaderPlay();
    Q_INVOKABLE void loaderRetry();
    Q_INVOKABLE void loaderEject();

    Q_INVOKABLE void toggleSio();
    Q_INVOKABLE void togglePrinter();
    Q_INVOKABLE void clearLog();

    // menu items
    Q_INVOKABLE void newImage();
    Q_INVOKABLE void mountDiskAny();
    Q_INVOKABLE void mountFolderAny();
    Q_INVOKABLE void ejectAll();
    Q_INVOKABLE void showPrinterOutput();
    Q_INVOKABLE void openSession();
    Q_INVOKABLE void saveSession();
    Q_INVOKABLE void options();
    Q_INVOKABLE void logWindow();
    Q_INVOKABLE void quit();
    Q_INVOKABLE QStringList recentFiles();
    Q_INVOKABLE void mountRecent(int index);

    // options window
    Q_INVOKABLE QVariantMap  loadOptions();
    Q_INVOKABLE void         applyOptions(const QVariantMap &o);
    Q_INVOKABLE QVariantList languages();

signals:
    void loaderChanged();
    void statusChanged();
    void logChanged();
    void drivesChanged();

private slots:
    void refresh();                              // pull engine state -> model
    void onLogMessage(int type, const QString &msg);

private:
    MainWindow *m_engine;
    DriveModel  m_model;

    int     m_loaderKind = 0;
    QString m_loaderFileName;
    QString m_loaderTypeText;
    double  m_loaderFill = 0.0;
    bool    m_loaderPlayEnabled = false;
    bool    m_loaderRetryEnabled = false;
    bool    m_loaderEjectEnabled = false;

    QString m_statusText;
    bool    m_sioRunning = false;
    bool    m_printerOn = false;
    bool    m_canAddSlot = true;
    QString m_logHtml;
};

#endif // QMLBRIDGE_H
