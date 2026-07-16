#include "qmlbridge.h"

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

const SlotData *DriveModel::slotAt(int row) const
{
    if (row < 0 || row >= m_slots.size()) return nullptr;
    return &m_slots.at(row);
}

// ---------------------------------------------------------------------------
// AppController
// ---------------------------------------------------------------------------
AppController::AppController(QObject *parent) : QObject(parent)
{
    m_statusText = QStringLiteral("19200 bps");
    m_sioRunning = true;    // mock: emulation running (speed shown, stop icon)
    seedMockData();
}

bool AppController::canAddSlot() const
{
    return m_slots.size() < 15;   // MAX_DISKS
}

void AppController::appendLog(const QString &html)
{
    m_logHtml += html + QStringLiteral("<br>");
    emit logChanged();
}

// Spike-only: reproduce the reference screenshot so the layout can be judged
// against the real app before the SioWorker wiring lands.
void AppController::seedMockData()
{
    SlotData s1;
    s1.hwIndex = 0; s1.mounted = true; s1.isFolder = true;
    s1.fileName = QStringLiteral("Atari"); s1.typeText = QStringLiteral("Folder");

    SlotData s2;
    s2.hwIndex = 1; s2.mounted = true; s2.isFolder = false;
    s2.fileName = QStringLiteral("Pac Man (v1).atr");
    s2.typeText = QStringLiteral("Dysk 512 s. SD (64k)");

    SlotData s3;
    s3.hwIndex = 2;   // empty

    m_slots = { s1, s2, s3 };
    m_model.setSlots(m_slots);

    m_loaderKind = 1;   // xex
    m_loaderFileName = QStringLiteral("blue_max.xex");
    m_loaderTypeText = QStringLiteral("Plik exe (24k)");

    m_logHtml =
        QStringLiteral("<span style='color:#1a56b0'>Program załadowany do Atari.</span><br>") +
        QStringLiteral("[Dysk 1] Zamontowane 'Atari' jako 'Folder'.<br>") +
        QStringLiteral("[Dysk 2] Zamontowane 'Pac Man (v1).atr' jako 'Dysk 512 s. SD (64k)'.<br>");

    emit loaderChanged();
    emit statusChanged();
    emit logChanged();
    emit drivesChanged();
}

// -- drive-slot actions (spike stubs) ---------------------------------------
void AppController::mountDisk(int hwIndex)   { appendLog(QStringLiteral("[Dysk %1] Montowanie obrazu…").arg(hwIndex + 1)); }
void AppController::mountFolder(int hwIndex) { appendLog(QStringLiteral("[Dysk %1] Montowanie katalogu…").arg(hwIndex + 1)); }
void AppController::eject(int hwIndex)       { appendLog(QStringLiteral("[Dysk %1] Wysunięto.").arg(hwIndex + 1)); }
void AppController::removeSlot(int hwIndex)  { appendLog(QStringLiteral("[Dysk %1] Usunięto slot.").arg(hwIndex + 1)); }
void AppController::save(int hwIndex)        { appendLog(QStringLiteral("[Dysk %1] Zapis.").arg(hwIndex + 1)); }
void AppController::toggleAutoCommit(int hwIndex) { appendLog(QStringLiteral("[Dysk %1] Auto-zapis przełączony.").arg(hwIndex + 1)); }
void AppController::openEditor(int hwIndex)  { appendLog(QStringLiteral("[Dysk %1] Edytor dysku.").arg(hwIndex + 1)); }
void AppController::toggleWriteProtect(int hwIndex) { appendLog(QStringLiteral("[Dysk %1] Ochrona zapisu.").arg(hwIndex + 1)); }
void AppController::bootOptions()            { appendLog(QStringLiteral("Opcje bootowania.")); }

void AppController::addSlot()
{
    if (!canAddSlot()) return;
    SlotData s; s.hwIndex = m_slots.size();
    m_slots.append(s);
    m_model.setSlots(m_slots);
    emit drivesChanged();
}

// -- loader actions (spike stubs) -------------------------------------------
void AppController::loaderLoad()  { appendLog(QStringLiteral("Loader: wybierz plik exe/cas.")); }
void AppController::loaderPlay()  { appendLog(QStringLiteral("Loader: odtwarzanie kasety.")); }
void AppController::loaderRetry() { appendLog(QStringLiteral("Loader: ponowna próba.")); }
void AppController::loaderEject()
{
    m_loaderKind = 0;
    m_loaderFileName.clear();
    m_loaderTypeText.clear();
    m_loaderFill = 0.0;
    emit loaderChanged();
    appendLog(QStringLiteral("Loader: wysunięto."));
}

// -- status bar (spike stubs) -----------------------------------------------
void AppController::toggleSio()
{
    m_sioRunning = !m_sioRunning;
    emit statusChanged();
    appendLog(m_sioRunning ? QStringLiteral("Emulacja uruchomiona.")
                           : QStringLiteral("Emulacja zatrzymana."));
}

void AppController::togglePrinter()
{
    m_printerOn = !m_printerOn;
    emit statusChanged();
}

void AppController::clearLog()
{
    m_logHtml.clear();
    emit logChanged();
}
