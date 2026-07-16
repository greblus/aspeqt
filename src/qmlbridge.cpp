#include "qmlbridge.h"
#include "mainwindow.h"

#include <QVariant>

// ---------------------------------------------------------------------------
// DriveModel
// ---------------------------------------------------------------------------
DriveModel::DriveModel(QObject *parent) : QAbstractListModel(parent) {}

int DriveModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_slots.size();
}

QVariant DriveModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_slots.size())
        return {};
    const SlotData &s = m_slots.at(index.row());
    switch (role) {
    case HwIndexRole:        return s.hwIndex;
    case SlotNumberRole:     return s.hwIndex + 1;
    case MountedRole:        return s.mounted;
    case IsFolderRole:       return s.isFolder;
    case FileNameRole:       return s.fileName;
    case TypeTextRole:       return s.typeText;
    case ModifiedRole:       return s.modified;
    case WriteProtectedRole: return s.writeProtected;
    case AutoCommitRole:     return s.autoCommit;
    case EditOpenRole:       return s.editOpen;
    case IsBootSlotRole:     return s.hwIndex == 0;
    }
    return {};
}

QHash<int, QByteArray> DriveModel::roleNames() const
{
    return {
        { HwIndexRole,        "hwIndex" },
        { SlotNumberRole,     "slotNumber" },
        { MountedRole,        "mounted" },
        { IsFolderRole,       "isFolder" },
        { FileNameRole,       "fileName" },
        { TypeTextRole,       "typeText" },
        { ModifiedRole,       "modified" },
        { WriteProtectedRole, "writeProtected" },
        { AutoCommitRole,     "autoCommit" },
        { EditOpenRole,       "editOpen" },
        { IsBootSlotRole,     "isBootSlot" },
    };
}

void DriveModel::setSlots(const QVector<SlotData> &list)
{
    beginResetModel();
    m_slots = list;
    endResetModel();
}

// ---------------------------------------------------------------------------
// AppController
// ---------------------------------------------------------------------------
AppController::AppController(MainWindow *engine, QObject *parent)
    : QObject(parent), m_engine(engine)
{
    if (m_engine) {
        connect(m_engine, &MainWindow::qmlChanged, this, &AppController::refresh);
        connect(m_engine, &MainWindow::logMessage, this, &AppController::onLogMessage);
    }
    refresh();
}

void AppController::refresh()
{
    if (!m_engine) return;

    // drive slots
    QVector<SlotData> rows;
    const QVariantList list = m_engine->qmlDriveList();
    for (const QVariant &v : list) {
        const QVariantMap m = v.toMap();
        SlotData s;
        s.hwIndex        = m.value("hwIndex").toInt();
        s.mounted        = m.value("mounted").toBool();
        s.isFolder       = m.value("isFolder").toBool();
        s.fileName       = m.value("fileName").toString();
        s.typeText       = m.value("typeText").toString();
        s.modified       = m.value("modified").toBool();
        s.writeProtected = m.value("writeProtected").toBool();
        s.autoCommit     = m.value("autoCommit").toBool();
        s.editOpen       = m.value("editOpen").toBool();
        rows.append(s);
    }
    m_model.setSlots(rows);

    bool add = m_engine->qmlCanAddSlot();
    if (add != m_canAddSlot) { m_canAddSlot = add; }
    emit drivesChanged();

    // loader
    const QVariantMap l = m_engine->qmlLoaderState();
    m_loaderKind         = l.value("kind").toInt();
    m_loaderFileName     = l.value("fileName").toString();
    m_loaderTypeText     = l.value("typeText").toString();
    m_loaderFill         = l.value("fill").toDouble();
    m_loaderPlayEnabled  = l.value("playEnabled").toBool();
    m_loaderRetryEnabled = l.value("retryEnabled").toBool();
    m_loaderEjectEnabled = l.value("ejectEnabled").toBool();
    emit loaderChanged();

    // status bar
    const QVariantMap st = m_engine->qmlStatus();
    m_sioRunning = st.value("running").toBool();
    m_statusText = st.value("speed").toString();
    m_printerOn  = st.value("printerOn").toBool();
    emit statusChanged();
}

// Format one engine log line the same way MainWindow::uiMessage colours it.
void AppController::onLogMessage(int type, const QString &msg)
{
    QString text = msg;
    if (text.startsWith('"')) text.remove(0, 1);
    if (text.endsWith('"'))   text.chop(1);
    text = text.toHtmlEscaped();

    QString colour;
    switch (type) {
    case 'd': colour = "green"; break;
    case 'u': colour = "gray";  break;
    case 'n': colour = "black"; break;
    case 'i': colour = "blue";  break;
    case 'w': colour = "brown"; break;
    case 'e': colour = "red";   break;
    default:  colour = "purple"; break;
    }
    m_logHtml += QStringLiteral("<span style='color:%1'>%2</span><br>").arg(colour, text);
    emit logChanged();
}

// -- actions ----------------------------------------------------------------
void AppController::mountDisk(int hwIndex)         { if (m_engine) m_engine->qmlMountDisk(hwIndex); }
void AppController::mountFolder(int hwIndex)       { if (m_engine) m_engine->qmlMountFolder(hwIndex); }
void AppController::eject(int hwIndex)             { if (m_engine) m_engine->qmlEjectPressed(hwIndex); }
void AppController::removeSlot(int hwIndex)        { if (m_engine) m_engine->qmlEjectPressed(hwIndex); }
void AppController::save(int hwIndex)              { if (m_engine) m_engine->qmlSave(hwIndex); }
void AppController::toggleAutoCommit(int hwIndex)  { if (m_engine) m_engine->qmlToggleAutoCommit(hwIndex); }
void AppController::openEditor(int hwIndex)        { if (m_engine) m_engine->qmlEdit(hwIndex); }
void AppController::toggleWriteProtect(int hwIndex){ if (m_engine) m_engine->qmlToggleWriteProtect(hwIndex); }
void AppController::bootOptions()                  { if (m_engine) m_engine->qmlBootOptions(); }
void AppController::addSlot()                      { if (m_engine) m_engine->qmlAddSlot(); }
void AppController::swapSlots(int fromHw, int toHw){ if (m_engine) m_engine->qmlSwapSlots(fromHw, toHw); }

void AppController::loaderLoad()  { if (m_engine) m_engine->qmlLoaderLoad(); }
void AppController::loaderPlay()  { if (m_engine) m_engine->qmlLoaderPlay(); }
void AppController::loaderRetry() { if (m_engine) m_engine->qmlLoaderRetry(); }
void AppController::loaderEject() { if (m_engine) m_engine->qmlLoaderEject(); }

void AppController::toggleSio()     { if (m_engine) m_engine->qmlToggleSio(); }
void AppController::togglePrinter() { if (m_engine) m_engine->qmlTogglePrinter(); }
void AppController::clearLog()      { if (m_engine) { m_engine->qmlClearLog(); } m_logHtml.clear(); emit logChanged(); }
