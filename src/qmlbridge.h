#ifndef QMLBRIDGE_H
#define QMLBRIDGE_H

// Thin C++<->QML bridge for the QML UI spike (branch `qml`). It reproduces the
// Android main-window layout in Qt Quick. For this first pass the model is
// seeded with mock data matching the reference screenshot; wiring to the real
// SioWorker follows once the layout is approved.

#include <QAbstractListModel>
#include <QObject>
#include <QString>
#include <QVector>

// One drive slot as seen by the QML delegate. Mirrors the facts that
// MainWindow::deviceStatusChanged() paints onto the widget slots.
struct SlotData {
    int     hwIndex = 0;        // SIO disk index (device = 0x31 + hwIndex)
    bool    mounted = false;
    bool    isFolder = false;
    QString fileName;          // display name (no path)
    QString typeText;          // img->description(), e.g. "Dysk 512 s. SD (64k)"
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
    const SlotData *slotAt(int row) const;

private:
    QVector<SlotData> m_slots;
};

class AppController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(DriveModel *drives READ drives CONSTANT)
    // Loader slot (pinned above the disk slots).
    Q_PROPERTY(int     loaderKind        READ loaderKind        NOTIFY loaderChanged)
    Q_PROPERTY(QString loaderFileName    READ loaderFileName    NOTIFY loaderChanged)
    Q_PROPERTY(QString loaderTypeText    READ loaderTypeText    NOTIFY loaderChanged)
    Q_PROPERTY(double  loaderFill        READ loaderFill        NOTIFY loaderChanged)
    Q_PROPERTY(bool    loaderPlayEnabled READ loaderPlayEnabled NOTIFY loaderChanged)
    Q_PROPERTY(bool    loaderRetryEnabled READ loaderRetryEnabled NOTIFY loaderChanged)
    Q_PROPERTY(bool    loaderEjectEnabled READ loaderEjectEnabled NOTIFY loaderChanged)
    // Bottom status bar + log pane.
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)
    Q_PROPERTY(bool    sioRunning READ sioRunning NOTIFY statusChanged)
    Q_PROPERTY(bool    printerOn  READ printerOn  NOTIFY statusChanged)
    Q_PROPERTY(QString logHtml    READ logHtml    NOTIFY logChanged)
    Q_PROPERTY(bool    canAddSlot READ canAddSlot NOTIFY drivesChanged)

public:
    explicit AppController(QObject *parent = nullptr);

    DriveModel *drives() { return &m_model; }

    int     loaderKind() const        { return m_loaderKind; }
    QString loaderFileName() const    { return m_loaderFileName; }
    QString loaderTypeText() const    { return m_loaderTypeText; }
    double  loaderFill() const        { return m_loaderFill; }
    bool    loaderPlayEnabled() const { return m_loaderKind == 2; }
    bool    loaderRetryEnabled() const { return !m_loaderFileName.isEmpty(); }
    bool    loaderEjectEnabled() const { return m_loaderKind != 0; }

    QString statusText() const { return m_statusText; }
    bool    sioRunning() const { return m_sioRunning; }
    bool    printerOn() const  { return m_printerOn; }
    QString logHtml() const    { return m_logHtml; }
    bool    canAddSlot() const;

    // Actions invoked from QML. For the spike these log the intent and mutate
    // the mock model; they will be routed to the real SioWorker later.
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

    Q_INVOKABLE void loaderLoad();
    Q_INVOKABLE void loaderPlay();
    Q_INVOKABLE void loaderRetry();
    Q_INVOKABLE void loaderEject();

    // status bar
    Q_INVOKABLE void toggleSio();
    Q_INVOKABLE void togglePrinter();
    Q_INVOKABLE void clearLog();

signals:
    void loaderChanged();
    void statusChanged();
    void logChanged();
    void drivesChanged();

private:
    void appendLog(const QString &html);
    void seedMockData();      // spike-only

    DriveModel        m_model;
    QVector<SlotData> m_slots;

    int     m_loaderKind = 0;     // 0 none, 1 xex, 2 cas
    QString m_loaderFileName;
    QString m_loaderTypeText;
    double  m_loaderFill = 0.0;

    QString m_statusText;
    bool    m_sioRunning = false;
    bool    m_printerOn = false;
    QString m_logHtml;
};

#endif // QMLBRIDGE_H
