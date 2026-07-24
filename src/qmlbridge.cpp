#include "qmlbridge.h"
#include "engine.h"
#include "aspeqtsettings.h"

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
        { IsBootSlotRole,     "isBootSlot" },
    };
}

void DriveModel::setSlots(const QVector<SlotData> &list)
{
    // A full reset destroys every delegate, which is fatal if it happens while
    // one of their signal handlers is still running (mount/eject buttons). When
    // only the contents changed, update in place and let the views repaint.
    bool sameShape = list.size() == m_slots.size();
    for (int i = 0; sameShape && i < list.size(); ++i)
        sameShape = list.at(i).sameShape(m_slots.at(i));

    if (!sameShape) {
        beginResetModel();
        m_slots = list;
        endResetModel();
        return;
    }

    for (int i = 0; i < list.size(); ++i) {
        if (m_slots.at(i) == list.at(i))
            continue;
        m_slots[i] = list.at(i);
        const QModelIndex ix = index(i, 0);
        emit dataChanged(ix, ix);
    }
}

// ---------------------------------------------------------------------------
// AppController
// ---------------------------------------------------------------------------
AppController::AppController(Engine *engine, QObject *parent)
    : QObject(parent), m_engine(engine)
{
    // Repaint the log at most ~10x a second no matter how fast lines arrive.
    m_logNotify.setSingleShot(true);
    m_logNotify.setInterval(33);
    connect(&m_logNotify, &QTimer::timeout, this, &AppController::logChanged);

    if (m_engine) {
        // Queued: the engine emits this from methods QML calls out of signal
        // handlers, and a structural change there would delete the delegate
        // whose handler is still on the stack (Qt aborts on that).
        connect(m_engine, &Engine::stateChanged, this, &AppController::refresh,
                Qt::QueuedConnection);
        connect(m_engine, &Engine::loaderProgress, this, &AppController::refreshLoader);
        connect(m_engine, &Engine::printerTextChanged, this, &AppController::refreshPrinter);
        connect(m_engine, &Engine::logMessage, this, &AppController::onLogMessage);
        connect(m_engine, &Engine::documentPicked, this, &AppController::documentPicked);
    }
    refresh();
}

void AppController::refresh()
{
    if (!m_engine) return;

    // drive slots
    QVector<SlotData> rows;
    const QVariantList list = m_engine->driveList();
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
        rows.append(s);
    }
    m_model.setSlots(rows);

    bool add = m_engine->canAddSlot();
    if (add != m_canAddSlot) { m_canAddSlot = add; }
    emit drivesChanged();

    // loader
    refreshLoader();

    // status bar
    const QVariantMap st = m_engine->status();
    m_sioRunning = st.value("running").toBool();
    m_statusText = st.value("speed").toString();
    m_printerOn  = st.value("printerOn").toBool();
    emit statusChanged();
}

void AppController::refreshLoader()
{
    if (!m_engine) return;
    const QVariantMap l = m_engine->loaderState();
    m_loaderKind         = l.value("kind").toInt();
    m_loaderFileName     = l.value("fileName").toString();
    m_loaderTypeText     = l.value("typeText").toString();
    m_loaderFill         = l.value("fill").toDouble();
    m_loaderPlayEnabled  = l.value("playEnabled").toBool();
    m_loaderRetryEnabled = l.value("retryEnabled").toBool();
    m_loaderEjectEnabled = l.value("ejectEnabled").toBool();
    m_loaderLoading      = l.value("loading").toBool();
    m_loaderCasPlaying   = l.value("casPlaying").toBool();
    emit loaderChanged();
}

// Format one engine log line the same way Engine::uiMessage colours it.
void AppController::onLogMessage(int type, const QString &msg)
{
    // Match the widget UI's uiMessage(): do NOT HTML-escape — messages may embed
    // intentional markup (e.g. <br> line breaks in the cassette-ready notice).
    QString text = msg;
    if (text.startsWith('"')) text.remove(0, 1);
    if (text.endsWith('"'))   text.chop(1);

    // Collapse a repeated line into "... [xN]" instead of printing it again
    // (the SIO path repeats a lot); this used to live in the widget log.
    if (text == m_lastLogLine && !m_logLines.isEmpty()) {
        m_logLines.removeLast();
        m_lastLogRepeat++;
        text = QStringLiteral("%1 [x%2]").arg(text).arg(m_lastLogRepeat);
    } else {
        m_lastLogLine = text;
        m_lastLogRepeat = 1;
    }

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
    m_logLines += QStringLiteral("<font color='%1'>%2</font><br>").arg(colour, text);
    while (m_logLines.size() > kMaxLogLines)
        m_logLines.removeFirst();
    m_logDirty = true;
    if (!m_logNotify.isActive())
        m_logNotify.start();
}

QString AppController::logHtml() const
{
    rebuildLogCaches();
    return m_logCache;
}

QString AppController::logTailHtml() const
{
    rebuildLogCaches();
    return m_logTailCache;
}

void AppController::rebuildLogCaches() const
{
    if (!m_logDirty) return;
    m_logCache = m_logLines.join(QString());
    const int from = qMax(0, m_logLines.size() - kTailLines);
    m_logTailCache = from == 0 ? m_logCache
                               : m_logLines.mid(from).join(QString());
    m_logDirty = false;
}

// -- actions ----------------------------------------------------------------
void AppController::eject(int hwIndex)             { if (m_engine) m_engine->ejectPressed(hwIndex); }
void AppController::removeSlot(int hwIndex)        { if (m_engine) m_engine->ejectPressed(hwIndex); }
int  AppController::save(int hwIndex)              { return m_engine ? m_engine->saveDisk(hwIndex) : Engine::SaveFailed; }
int  AppController::toggleAutoCommit(int hwIndex)  { return m_engine ? m_engine->toggleAutoCommitDisk(hwIndex) : Engine::SaveFailed; }
void AppController::toggleWriteProtect(int hwIndex){ if (m_engine) m_engine->toggleWriteProtect(hwIndex); }
int AppController::addSlot()                       { return m_engine ? m_engine->addSlot() : -1; }
void AppController::swapSlots(int fromHw, int toHw){ if (m_engine) m_engine->swapSlots(fromHw, toHw); }

void AppController::loaderPlay()  { if (m_engine) m_engine->loaderPlay(); }
void AppController::loaderRetry() { if (m_engine) m_engine->loaderRetry(); }
void AppController::loaderEject() { if (m_engine) m_engine->loaderEject(); }

void AppController::toggleSio()     { if (m_engine) m_engine->toggleSio(); }
void AppController::togglePrinter() { if (m_engine) m_engine->togglePrinter(); }
void AppController::clearLog()
{
    if (m_engine) m_engine->clearLog();
    m_logLines.clear();
    m_logDirty = true;
    m_logNotify.stop();
    emit logChanged();
}

void AppController::createDisk(int sc, int ss) { if (m_engine) m_engine->createDisk(sc, ss); }
void AppController::printerClear()      { if (m_engine) m_engine->printerClear(); }
bool AppController::printerSavePath(const QString &url, bool asPdf) { return m_engine ? m_engine->printerSavePath(url, asPdf) : false; }
void AppController::refreshPrinter()
{
    if (!m_engine) return;
    m_printerText = m_engine->printerText();
    m_printerTextAtascii = m_engine->printerTextAtascii();
    emit printerTextChanged();
}
void AppController::ejectAll()          { if (m_engine) m_engine->ejectAll(); }
QVariantList AppController::modifiedDisks() { return m_engine ? m_engine->modifiedDisks() : QVariantList(); }
void AppController::quit()              { if (m_engine) m_engine->quit(); }
QStringList AppController::recentFiles(){ return m_engine ? m_engine->recentFiles() : QStringList(); }
void AppController::mountRecent(int i)  { if (m_engine) m_engine->mountRecent(i); }

QVariantMap AppController::loadOptions()          { return m_engine ? m_engine->loadOptions() : QVariantMap(); }
void AppController::applyOptions(const QVariantMap &o) { if (m_engine) m_engine->applyOptions(o); }
QVariantList AppController::languages()           { return m_engine ? m_engine->languages() : QVariantList(); }

QVariantList AppController::phonebookEntries()    { return m_engine ? m_engine->phonebookEntries() : QVariantList(); }
bool AppController::phonebookSave(const QVariantList &l) { return m_engine ? m_engine->phonebookSave(l) : false; }
QString AppController::phonebookPath()            { return m_engine ? m_engine->phonebookPath() : QString(); }
QString AppController::phonebookUseBundled()      { return m_engine ? m_engine->phonebookUseBundled() : QString(); }
QString AppController::phonebookBundledPath()     { return m_engine ? m_engine->phonebookBundledPath() : QString(); }
void AppController::phonebookDial(const QVariantMap &e)  { if (m_engine) m_engine->phonebookDial(e); }

bool AppController::diskOpen(int hw)      { return m_engine ? m_engine->diskOpen(hw) : false; }
void AppController::diskClose()           { if (m_engine) m_engine->diskClose(); }
QVariantList AppController::diskEntries()  { return m_engine ? m_engine->diskEntries() : QVariantList(); }
QString AppController::diskPath()          { return m_engine ? m_engine->diskPath() : QString(); }
bool AppController::diskCanParent()        { return m_engine ? m_engine->diskCanParent() : false; }
bool AppController::diskReadOnly()         { return m_engine ? m_engine->diskReadOnly() : true; }
int AppController::diskFsType()            { return m_engine ? m_engine->diskFsType() : 0; }
bool AppController::diskSetFsType(int i)   { return m_engine ? m_engine->diskSetFsType(i) : false; }
void AppController::toast(const QString &t)  { if (m_engine) m_engine->toast(t); }
QString AppController::startDir(const QString &kind) { return m_engine ? m_engine->startDir(kind) : QString(); }
void AppController::mountDiskPath(int i, const QString &url)   { if (m_engine) m_engine->mountDiskPath(i, url); }
void AppController::mountNetworkTemp(const QString &path) { if (m_engine) m_engine->mountNetworkTemp(path); }
bool AppController::isNetworkTempSlot(int i)   { return m_engine && m_engine->isNetworkTempSlot(i); }
QString AppController::netTempName(int i)      { return m_engine ? m_engine->netTempName(i) : QString(); }
void AppController::clearNetworkTemp(int i)    { if (m_engine) m_engine->clearNetworkTemp(i); }
QVariantList AppController::networkTempDisks() { return m_engine ? m_engine->networkTempDisks() : QVariantList(); }

// The R: device (850/modem) emulation option. The network browser is only
// offered when it is on, so a QML binding gates the menu item on this.
bool AppController::modemEnabled() const { return aspeqtSettings && aspeqtSettings->rDeviceEnabled(); }
void AppController::mountFolderPath(int i, const QString &url) { if (m_engine) m_engine->mountFolderPath(i, url); }
void AppController::loaderLoadPath(const QString &url)         { if (m_engine) m_engine->loaderLoadPath(url); }
bool AppController::saveAsPath(int i, const QString &url)      { return m_engine ? m_engine->saveAsPath(i, url) : false; }
void AppController::installDos(int i)                          { if (m_engine) m_engine->installDos(i); }
void AppController::openSessionPath(const QString &url)        { if (m_engine) m_engine->openSessionPath(url); }
void AppController::saveSessionPath(const QString &url)        { if (m_engine) m_engine->saveSessionPath(url); }
void AppController::pickDocument(int r, const QString &m)      { if (m_engine) m_engine->pickDocument(r, m); }
void AppController::createDocument(int r, const QString &m, const QString &n) { if (m_engine) m_engine->createDocument(r, m, n); }
void AppController::pickFolder(int r)                          { if (m_engine) m_engine->pickFolder(r); }
void AppController::diskEnter(int row)     { if (m_engine) m_engine->diskEnter(row); }
void AppController::diskParent()           { if (m_engine) m_engine->diskParent(); }
void AppController::diskSetTextConversion(bool on) { if (m_engine) m_engine->diskSetTextConversion(on); }
bool AppController::diskExtractPath(const QVariantList &r, const QString &url) { return m_engine ? m_engine->diskExtractPath(r, url) : false; }
bool AppController::diskDelete(const QVariantList &rows)  { return m_engine ? m_engine->diskDelete(rows) : false; }
bool AppController::diskAddFilesPath(const QString &url) { return m_engine ? m_engine->diskAddFilesPath(url) : false; }
