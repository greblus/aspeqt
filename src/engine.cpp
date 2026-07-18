#include "engine.h"

#include "diskimage.h"
#include "diskimagepro.h"
#include "folderimage.h"
#include "atarifilesystem.h"
#include <QVariant>
#ifdef Q_OS_ANDROID
#include <QJniObject>
#include <jni.h>
#endif
#include "pclink.h"
#include "miscdevices.h"
#include "aspeqtsettings.h"
#include "autoboot.h"

#include <QUrl>
#include <QFile>
#include <QTemporaryFile>
#include <QStandardPaths>
#include <QDir>
#include <QTranslator>
#include <QtDebug>
#include <QFont>

#include "atarifilesystem.h"
#include "miscutils.h"

#ifdef Q_OS_ANDROID
#include <QJniObject>
#endif

#include <QScreen>
#include <QTimer>
#include "math.h"

AspeqtSettings *aspeqtSettings;
Engine *g_engine;

// Defined with the rest of the picker plumbing further down.
static QString pathFromPickedUrl(const QString &url);

QFile *logFile;
QMutex *logMutex;
QString g_exefileName;
QString g_aspeclFileName;
QString g_aspeQtAppPath;
QRect g_savedGeometry;
char g_aspeclSlotNo;
bool g_disablePicoHiSpeed;
bool g_printerEmu = true;
bool g_D9DOVisible = true;
bool g_miniMode = false;
bool g_shadeMode = false;
int g_savedWidth;
bool g_logOpen;
float ssize = 0;
int btnsize = 0;
int sbIcon = 0;   // status-bar icon size (20% smaller than the drive-button icons)

// ****************************** END OF GLOBALS ************************************//

// Displayed only in debug mode    "!d"
// Unimportant     (gray)          "!u"
// Normal          (black)         "!n"
// Important       (blue)          "!i"
// Warning         (brown)         "!w"
// Error           (red)           "!e"

void logMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
// void logMessageOutput(QtMsgType type, const char *msg)
{
    logMutex->lock();
    logFile->write(QString::number((quint64)QThread::currentThreadId(), 16).toLatin1());
    switch (type) {
        case QtDebugMsg:
            logFile->write(": [Debug]    ");
            break;
        case QtWarningMsg:
            logFile->write(": [Warning]  ");
            break;
        case QtCriticalMsg:
            logFile->write(": [Critical] ");
            break;
        case QtFatalMsg:
            logFile->write(": [Fatal]    ");
            break;
    }
    QByteArray localMsg = msg.toLocal8Bit();
    QByteArray displayMsg = localMsg.mid(3);
    logFile->write(displayMsg);
    logFile->write("\n");
    if (type == QtFatalMsg) {
        logFile->close();
        abort();
    }
    logMutex->unlock();

    if (msg[0] == '!') {
#ifdef QT_NO_DEBUG
        if (msg[1] == 'd') {
            return;
        }
#endif
        g_engine->doLogMessage(localMsg.at(1), displayMsg);
    }
}

void Engine::doLogMessage(int type, const QString &msg)
{
    emit logMessage(type, msg);
}

Engine::Engine(QObject *parent)
    : QObject(parent)
{

    /* Setup the logging system */
    g_engine = this;
    g_aspeQtAppPath = QCoreApplication::applicationDirPath();
    g_disablePicoHiSpeed = false;
    logFile = new QFile(QDir::temp().absoluteFilePath("aspeqt.log"));
    logFile->open(QFile::WriteOnly | QFile::Truncate | QFile::Unbuffered | QFile::Text);
    logMutex = new QMutex();
    qInstallMessageHandler(logMessageOutput);
    qDebug() << "!d" << tr("AspeQt started at %1.").arg(QDateTime::currentDateTime().toString());

    /* Remove old temporaries */
    QDir tempDir = QDir::temp();
    QStringList filters;
    filters << "aspeqt-*";
    QFileInfoList list = tempDir.entryInfoList(filters, QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files);
    foreach(QFileInfo file, list) {
        deltree(file.absoluteFilePath());
    }
    
    // Check to see if settings repository is already migrated, if not invoke migration process
    QSettings oldSettings("greblus.net", "AspeQt");
    oldSettings.setFallbacksEnabled(false);
    QSettings newSettings("greblus.net", "AspeQt");
    QStringList oldKeys = oldSettings.allKeys();
    if(oldKeys.size()>0){
        qWarning() << "!i" << tr("Migrating the global settings to their new repository "
                                 "(session files are not affected).");
        for (int i=0; i<oldKeys.size(); ++i) {
            newSettings.setValue(oldKeys.value(i), oldSettings.value(oldKeys.value(i)));
        }
        oldSettings.clear();
        qWarning() << "!i" << tr("Settings migrated successfully.");
    }
    /* Set application properties */
    QCoreApplication::setOrganizationName("Atari Forever!");
    QCoreApplication::setOrganizationDomain("greblus.net");
    QCoreApplication::setApplicationName("AspeQt");
    aspeqtSettings = new AspeqtSettings();

#ifdef Q_OS_ANDROID
    int serial_int = aspeqtSettings->serialPortInterface();
    QJniObject::setStaticField("net/greblus/SerialActivity", "m_serial", serial_int);
    QJniObject::callStaticMethod<void>("net/greblus/SerialActivity", "changeDevice", "(I)V", serial_int);
    QJniObject b_name = QJniObject::fromString(aspeqtSettings->bluetoothName());
    jstring bluetooth_name = b_name.object<jstring>();
    QJniObject::setStaticField("net/greblus/SerialActivity", "bluetoothName", bluetooth_name);
#endif

    /* Load translators */
    loadTranslators();

    /* set codec for locale on Windows */
    #ifdef Q_OS_WIN32
    #endif

    
    /* Setup UI */

    /* I love ugly hacks */
    QScreen *screen = qApp->screens().at(0);

    int scrh = screen->size().height();
    int scrw = screen->size().width();

    float resx = scrw/screen->physicalDotsPerInchX();
    float resy = scrh/screen->physicalDotsPerInchY();

    ssize = sqrt(resx*resx + resy*resy);

     /* Parse command line arguments:
      arg(1): session file (xxxxxxxx.aspeqt)   */

    QStringList AspeQtArgs = QCoreApplication::arguments();
    g_sessionFile = g_sessionFilePath = "";
    if (AspeQtArgs.size() > 1) {
       QFile sess;
       QString s = QDir::separator();             //
       int i = AspeQtArgs.at(1).lastIndexOf(s);   //
       if (i != -1) {
           i++;
           g_sessionFile = AspeQtArgs.at(1).right(AspeQtArgs.at(1).size() - i);
           g_sessionFilePath = AspeQtArgs.at(1).left(i);
           g_sessionFilePath = QDir::fromNativeSeparators(g_sessionFilePath);
           sess.setFileName(g_sessionFilePath+g_sessionFile);
           if (!sess.exists()) {
               qCritical() << "!e" << tr("Requested session file not found in the given directory path "
                                         "or the path is incorrect. AspeQt will continue with the default "
                                         "session configuration.");
               g_sessionFile = g_sessionFilePath = "";
           }
       } else {
           if (AspeQtArgs.at(1) != "") {
               g_sessionFile = AspeQtArgs.at(1);
               g_sessionFilePath = QDir::currentPath();
               sess.setFileName(g_sessionFile);
               if (!sess.exists()) {
                   qCritical() << "!e" << tr("Requested session file not found in the application's current "
                                             "directory (no path was specified). AspeQt will continue with "
                                             "the default session configuration.");
                   g_sessionFile = g_sessionFilePath = "";
               }
           }
         }
    }
    // Pass Session file name, path and Engine title to AspeQtSettings //
    aspeqtSettings->setSessionFile(g_sessionFile, g_sessionFilePath);
    aspeqtSettings->setMainWindowTitle(g_mainWindowTitle);

    // Display Session name, and restore session parameters if session file was specified //
    g_mainWindowTitle = tr("AspeQt - Atari Serial Peripheral Emulator for Qt");
    if (g_sessionFile != "") {
        aspeqtSettings->loadSessionFromFile(g_sessionFilePath+g_sessionFile);
    } else {
    }
#ifdef Q_OS_ANDROID
    // Fill the *available* screen (excludes the system status/navigation bars) and
    // keep filling it across rotations, so the bottom status bar stays on-screen.
    // A saved desktop size would leave the window at the portrait width (~half the
    // screen) in landscape.
#else
#endif

    // Slot presence comes from the session; the QML UI is the same everywhere.
    rebuildSlots();

    /* Connect SioWorker signals */
    sio = new SioWorker();
    connect(sio, SIGNAL(started()), this, SLOT(sioStarted()));
    connect(sio, SIGNAL(finished()), this, SLOT(sioFinished()));
    connect(sio, SIGNAL(statusChanged(QString)), this, SLOT(sioStatusChanged(QString)));
    shownFirstTime = true;

    PCLINK* pclink = new PCLINK(sio);
    sio->installDevice(0x6F, pclink);

#ifdef Q_OS_ANDROID
    // Loader state (used to be done by androidBuildLoaderSlot()).
    loaderEject();
#endif

#ifdef Q_OS_ANDROID
    // Plugging the cable in launches us; start emulating straight away instead
    // of making the user press start. Queued so the UI is up first.
    if (QJniObject::callStaticMethod<jboolean>("net/greblus/SerialActivity", "launchedByUsb", "()Z")) {
        QTimer::singleShot(0, this, [this]{
            if (!m_emulationRunning)
                toggleSio();
        });
    }
#endif

    /* Restore application state */
    for (int i = 0; i < m_numDisks; i++) {      //
        AspeqtSettings::ImageSettings is;
        is = aspeqtSettings->mountedImageSetting(i);
        mountFile(i, is.fileName, is.isWriteProtected);
    }

    // SmartDevice (ApeTime + URL submit)
    SmartDevice *smart = new SmartDevice(sio);
    sio->installDevice(SMART_CDEVIC, smart);

    // AspeQt Client  //
    AspeCl *acl = new AspeCl(sio);
    sio->installDevice(0x46, acl);

    printerOutput = new PrinterOutput(this);
    // Documentation Display


    Printer *printer = new Printer(sio);
    connect(printer, SIGNAL(print(QString)), printerOutput, SLOT(print(QString)));
    connect(printerOutput, &PrinterOutput::textChanged, this, &Engine::printerTextChanged);
    connect(printer, SIGNAL(print(QString)), this, SIGNAL(printerTextChanged()));
    sio->installDevice(0x40, printer);
    untitledName = 0;


    // Connections needed for remotely mounting a disk image & Toggle Auto-Commit //
    connect (acl, SIGNAL(findNewSlot(int,bool)), this, SLOT(firstEmptyDiskSlot(int,bool)));
    connect (this, SIGNAL(newSlot(int)), acl, SLOT(gotNewSlot(int)));
    connect (acl, SIGNAL(mountFile(int,QString)), this, SLOT(mountFileWithDefaultProtection(int,QString)));
    connect (this, SIGNAL(fileMounted(bool)), acl, SLOT(fileMounted(bool)));
    connect (acl, SIGNAL(toggleAutoCommit(int)), this, SLOT(autoCommit(int)));

}

Engine::~Engine()
{
    if (m_emulationRunning) {
        toggleSio();
    }

    delete aspeqtSettings;
    delete sio;

    qDebug() << "!d" << tr("AspeQt stopped at %1.").arg(QDateTime::currentDateTime().toString());
    qInstallMessageHandler(0);
    delete logMutex;
    delete logFile;
}

// Shut the engine down: stop SIO, offer to save modified images, close the
// devices. Returns false when the user cancelled, so the caller leaves the
// application running (which close()/event->ignore() used to do -- except
// quit() then quit anyway, so Cancel did not actually cancel).
void Engine::shutdown()
{
    if (g_sessionFile != "") aspeqtSettings->saveSessionToFile(g_sessionFilePath + "/" + g_sessionFile);
    aspeqtSettings->setD9DOVisible(g_D9DOVisible);

    if (m_emulationRunning) {
        toggleSio();
    }

    for (int i = 0x31; i < 0x39; i++) {
        SimpleDiskImage *s = qobject_cast <SimpleDiskImage*> (sio->getDevice(i));
        if (s) {
            s->close();
        }
    }

    aspeqtSettings->sync();   // flush settings now; the process may be killed on exit
}

// Images with changes that are not on disk yet, so the UI can ask about them
// before ejecting, mounting over them or quitting.
QVariantList Engine::modifiedDisks()
{
    QVariantList out;
    for (int i = 0; i < m_numDisks; i++) {
        SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(i + 0x31));
        if (!img || !img->isModified())
            continue;
        QVariantMap m;
        m["hwIndex"] = i;
        m["slot"]    = i + 1;
        m["name"]    = friendlyName(img->originalFileName());
        out << m;
    }
    return out;
}


void Engine::rebuildSlots()
{
    for (int i = 0; i < MAX_DISKS; ++i)
        m_slotPresent[i] = false;
    m_numDisks = aspeqtSettings->numberOfDisks();
    if (m_numDisks < 1)          m_numDisks = DEFAULT_DISKS;
    if (m_numDisks > MAX_DISKS)  m_numDisks = MAX_DISKS;
    for (int i = 0; i < m_numDisks; ++i)
        if (aspeqtSettings->slotPresent(i))   // removed slot -> gap
            m_slotPresent[i] = true;
    emit stateChanged();
}

// Hide the "+" row once every hardware slot index is in use.
void Engine::addSlotAt()
{
    int i = -1;
    for (int k = 0; k < MAX_DISKS; ++k)
        if (!m_slotPresent[k]) { i = k; break; }
    if (i < 0) return;                       // all slots present
    m_slotPresent[i] = true;
    if (i >= m_numDisks) m_numDisks = i + 1;
    aspeqtSettings->setNumberOfDisks(m_numDisks);
    aspeqtSettings->setSlotPresent(i, true);
    deviceStatusChanged(i + 0x31);
}

// 2nd eject on an empty slot: drop this specific slot, leaving a number gap so
// the other slots keep their device numbers (important for DOS).
void Engine::removeSlot(int i)
{
    if (i < 0 || i >= MAX_DISKS || !m_slotPresent[i]) return;
    int present = 0;
    for (int k = 0; k < MAX_DISKS; ++k) if (m_slotPresent[k]) present++;
    if (present <= 1) return;                // keep at least one slot

    m_slotPresent[i] = false;
    m_autoCommit[i] = false;
    m_writeProtect[i] = false;
    aspeqtSettings->setSlotPresent(i, false);
    // Shrink the persisted range to the highest still-present slot.
    int hi = 0;
    for (int k = 0; k < MAX_DISKS; ++k) if (m_slotPresent[k]) hi = k + 1;
    m_numDisks = hi;
    aspeqtSettings->setNumberOfDisks(m_numDisks);
    emit stateChanged();
}

// Eject button: eject a mounted image; on an already-empty slot the button
// shows a trash icon and removes the slot instead.
void Engine::ejectPressedAt(int i)
{
    if (sio->getDevice(i + 0x31))
        ejectImage(i);
    else
        removeSlot(i);
}

// ---- top loader slot: inline XEX autoboot / CAS cassette player -------------

// Build the always-on-top loader slot (badge "cas/xex", load / play / retry /
// eject buttons, and a load progress bar) and insert it above the disk slots.
void Engine::loaderSetFill(double frac)
{
    m_loaderFill = frac;
    emit loaderProgress();   // light: only the loader fill
}

void Engine::loaderUpdateButtons()
{
    emit stateChanged();
}


// XEX: install the autoboot loader on D1 and let the running emulation boot it,
// driving the progress bar from the loader's blockRead signal.
void Engine::loaderLoadXex(const QString &path)
{
    loaderEject();                       // clear any previous load
    m_autoBootOld = sio->getDevice(0x31);
    m_autoBoot = new AutoBoot(sio, m_autoBootOld);
    if (!m_autoBoot->open(path, aspeqtSettings->useHighSpeedExeLoader())) {
        delete m_autoBoot;
        m_autoBoot = nullptr;
        m_autoBootOld = nullptr;
        qWarning() << "!i" << tr("Failed to load executable '%1'.").arg(friendlyName(path));
        return;
    }
    sio->uninstallDevice(0x31);
    sio->installDevice(0x31, m_autoBoot);
    connect(m_autoBoot, &AutoBoot::blockRead, this, &Engine::loaderBlockRead);
    connect(m_autoBoot, &AutoBoot::loaderDone, this, &Engine::loaderBooterDone);

    m_loaderKind = 1;
    m_loaderFile = path;
    m_loaderName = friendlyName(path);
    m_loaderTypeText = tr("Executable (%1k)").arg((QFileInfo(path).size() + 512) / 1024);
    loaderSetFill(0);
    loaderUpdateButtons();
    qDebug() << "!i" << tr("Loaded executable '%1'. Start (or reboot) your Atari to run it.")
                        .arg(friendlyName(path));
}

// CAS: load the cassette image, log the playback instructions and arm the play
// button (playback itself is started by the user, in sync with the Atari).
void Engine::loaderLoadCas(const QString &path)
{
    loaderEject();
    m_casWorker = new CassetteWorker;
    if (!m_casWorker->loadCasImage(path)) {
        delete m_casWorker;
        m_casWorker = nullptr;
        qWarning() << "!i" << tr("Failed to load cassette image '%1'.").arg(friendlyName(path));
        return;
    }
    m_casTotal = m_casWorker->mTotalDuration;
    m_casRemaining = m_casTotal;
    m_loaderKind = 2;
    m_loaderFile = path;
    m_loaderName = friendlyName(path);
    {
        int minutes = m_casTotal / 60000;
        int seconds = (m_casTotal - minutes * 60000) / 1000;
        m_loaderTypeText = tr("Cassette (%1:%2)").arg(minutes).arg(seconds, 2, 10, QChar('0'));
    }
    loaderSetFill(0);
    loaderUpdateButtons();

    // Use doLogMessage (not qDebug, which would escape the newlines) and render
    // the line breaks as <br> for the HTML log.
    QString msg = tr("AspeQt is ready to playback the cassette image file '%1'.\n\n"
                     "Do whatever is necessary in your Atari to load this cassette "
                     "image like rebooting while holding Option and Start buttons "
                     "or entering \"CLOAD\" in the BASIC prompt.\n\n"
                     "When you hear the beep sound, push the play button and press "
                     "a key on your Atari at about the same time.")
                  .arg(friendlyName(path));
    doLogMessage('i', msg.replace("\n", "<br>"));
}

// Play button: start streaming the loaded cassette image.
void Engine::loaderPlayCas()
{
    if (m_loaderKind != 2 || !m_casWorker || m_casWorker->isRunning())
        return;
    // The cassette player opens the single serial port itself, so disk emulation
    // must be paused while it runs (AspeQt never drives cassette + disk at once).
    m_casWasRunning = m_emulationRunning;
    if (m_casWasRunning) {
        toggleSio();
        sio->wait();
        qApp->processEvents();
    }
    connect(m_casWorker, &CassetteWorker::statusChanged, this, &Engine::loaderCasStatus, Qt::QueuedConnection);
    connect(m_casWorker, &QThread::finished, this, &Engine::loaderCasFinished);
    m_casWorker->start(QThread::TimeCriticalPriority);
    m_casTimer = new QTimer(this);
    connect(m_casTimer, &QTimer::timeout, this, &Engine::loaderCasTick);
    m_casTimer->start(1000);
    loaderUpdateButtons();
    qDebug() << "!i" << tr("Playing back cassette image.");
}

void Engine::loaderCasStatus(int remainingTime)
{
    if (!m_casWorker) return;
    m_casTotal = m_casWorker->mTotalDuration;
    m_casRemaining = remainingTime;
    loaderSetFill(m_casTotal > 0 ? double(m_casTotal - m_casRemaining) / m_casTotal : 0.0);
}

void Engine::loaderCasTick()
{
    if (m_casRemaining < 1000)
        m_casRemaining = 1000;
    loaderCasStatus(m_casRemaining - 1000);
}

void Engine::loaderCasFinished()
{
    if (m_casTimer) { m_casTimer->stop(); }
    loaderSetFill(0);            // fill disappears when done
    // Resume disk emulation if we paused it for the cassette.
    if (m_casWasRunning) {
        m_casWasRunning = false;
        if (!m_emulationRunning)
            toggleSio();
    }
    loaderUpdateButtons();
    qDebug() << "!i" << tr("Cassette playback finished.");
}

void Engine::loaderBlockRead(int current, int all)
{
    loaderSetFill(all > 0 ? double(current) / all : 0.0);
}

void Engine::loaderBooterDone()
{
    loaderSetFill(0);           // fill disappears once loaded
    qDebug() << "!i" << tr("Executable loaded into the Atari.");
    // The program now runs from Atari RAM; hand D1 back to whatever was there.
    if (m_autoBoot) {
        sio->uninstallDevice(0x31);
        if (m_autoBootOld)
            sio->installDevice(0x31, m_autoBootOld);
        m_autoBoot->deleteLater();
        m_autoBoot = nullptr;
        m_autoBootOld = nullptr;
    }
    loaderUpdateButtons();
}

// Eject: stop any playback/boot and clear the loader slot.
void Engine::loaderEject()
{
    if (m_casTimer) { m_casTimer->stop(); m_casTimer->deleteLater(); m_casTimer = nullptr; }
    if (m_casWorker) {
        if (m_casWorker->isRunning()) {
            m_casWorker->setPriority(QThread::NormalPriority);
            m_casWorker->wait();
        }
        m_casWorker->deleteLater();
        m_casWorker = nullptr;
    }
    // Resume disk emulation if it was paused for cassette playback.
    if (m_casWasRunning) {
        m_casWasRunning = false;
        if (!m_emulationRunning)
            toggleSio();
    }
    if (m_autoBoot) {
        sio->uninstallDevice(0x31);
        if (m_autoBootOld)
            sio->installDevice(0x31, m_autoBootOld);
        m_autoBoot->deleteLater();
        m_autoBoot = nullptr;
        m_autoBootOld = nullptr;
    }
    m_loaderKind = 0;
    m_loaderFile.clear();
    m_loaderName.clear();
    m_loaderTypeText.clear();
    loaderSetFill(0);
    loaderUpdateButtons();
}

// Retry: re-run the last load (e.g. after a failed boot).
void Engine::loaderRetry()
{
    if (m_loaderFile.isEmpty()) return;
    QString path = m_loaderFile;   // loaderEject() (via load*) clears m_loaderKind
    int kind = m_loaderKind;
    if (kind == 2)
        loaderLoadCas(path);
    else
        loaderLoadXex(path);
}





// Toggle Mini Mode //

// Toggle printer Emulation ON/OFF //


void Engine::sioStarted()
{
    m_emulationRunning = true;
    emit stateChanged();
}

void Engine::sioFinished()
{
    sio->wait();          // already finished: this just closes the port
    m_emulationRunning = false;
    m_sioStatus.clear();
    qWarning() << "!i" << tr("Emulation stopped.");
    emit stateChanged();
}

void Engine::sioStatusChanged(QString status)
{
    m_sioStatus = status;
    emit stateChanged();
}

void Engine::deviceStatusChanged(int deviceNo)
{
    if (deviceNo >= 0x31 && deviceNo <= 0x31 + MAX_DISKS - 1) {
        int no = deviceNo - 0x31;
        if (m_slotPresent[no]) {
            SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(deviceNo));
            if (img) {
                // Auto-commit: write the image out as soon as it is modified.
                if (img->isModified() && m_autoCommit[no]) {
                    // Reached from the SIO status path, possibly mid-transfer:
                    // report the failure, never block waiting for an answer.
                    if (!img->save()) {
                        qCritical() << "!e" << tr("[%1] Auto-commit failed.")
                                       .arg(img->deviceName());
                    }
                }
            } else {
                m_autoCommit[no] = false;
                m_writeProtect[no] = false;
            }
        }
    }
    emit stateChanged();
}




//

// Restart emulation and re-translate following a session load //
void Engine::setSession()
{
    bool restart;
    restart = m_emulationRunning;
    if (restart) {
        toggleSio();
       sio->wait();
        qApp->processEvents();
    }

    // load translators and retranslate
    loadTranslators();
    for (int i = 0; i < MAX_DISKS; i++) {
        deviceStatusChanged(0x31 + i);
    }

    toggleSio();
}


// Unconditional: whether unsaved changes matter is the UI's call, made before
// this is reached (the QML side knows which images are modified).
void Engine::ejectImage(int no)
{
    SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + 0x31));

    sio->uninstallDevice(no + 0x31);
    if (!img) {
        return;
    }
    delete img;
#ifdef Q_OS_ANDROID
    // Folder images are mounted from the SAF tree in place (nothing was copied),
    // so ejecting just drops the tracking entries.
    m_folderTree.remove(no);
    m_folderTemp.remove(no);
#endif
    m_writeProtect[no] = false;

    aspeqtSettings->unmountImage(no);
    deviceStatusChanged(no + 0x31);
    qDebug() << "!n" << tr("Unmounted disk %1").arg(no + 1);
}

int Engine::firstEmptyDiskSlot(int startFrom, bool createOne)
{
    int i;
    for (i = startFrom; i < m_numDisks; i++) {  //
        // Skip removed slots (gaps): nothing may mount there.
        if (!m_slotPresent[i]) {
            continue;
        }
        if (!sio->getDevice(0x31 + i)) {
            break;
        }
    }
    if (i > m_numDisks-1) {   //
        if (createOne) {
            i = m_numDisks-1;
            // Land on a real (present) slot, never a gap.
            while (i >= 0 && !m_slotPresent[i]) i--;
        } else {
            i = -1;
        }
    }
    emit newSlot(i);        //
    return i;
}

// Make boot executable dialog persistant until it's manually closed //

void Engine::mountFileWithDefaultProtection(int no, const QString &fileName)
{
    // If fileName was passed from AspeCL it is an 8.1 name, so we need to find
    // the full PC name in order to validate it.  //
    QString atariFileName, atariLongName, path;

    g_aspeclFileName = fileName;
    atariFileName = fileName;

    if(atariFileName.left(1) == "*") {
        FolderImage fi(sio);
        atariFileName = atariFileName.mid(1);
        path = aspeqtSettings->lastFolderImageDir();
        atariLongName = fi.longName(path, atariFileName);
        if(atariLongName == "") {
            sio->port()->writeDataNak();
            return;
        } else {
            atariFileName =  path + "/" + atariLongName;
        }
     }

    bool prot = aspeqtSettings->getImageSettingsFromName(atariFileName).isWriteProtected;
    mountFile(no, atariFileName, prot);
}

void Engine::mountFile(int no, const QString &fileName, bool /*prot*/)
{
    SimpleDiskImage *disk;
    bool isDir = false;

    if (fileName.isEmpty()) {
        if(g_aspeclFileName.left(1) == "*") emit fileMounted(false);  //
        return;
    }

    FileTypes::FileType type = FileTypes::getFileType(fileName);

    // Executables (.xex/.com/.exe) and cassettes (.cas) don't belong in a disk
    // slot: send them to the loader (cas/xex) slot. Detected by content type and
    // by extension (a .com Atari binary has the same 0xFF 0xFF magic as .xex).
    {
        QString nm = fileName;
#ifdef Q_OS_ANDROID
        if (fileName.startsWith("content:")) nm = friendlyName(fileName);
#endif
        nm = nm.toLower();
        bool isCas = type == FileTypes::Cas || type == FileTypes::CasGz
                     || nm.endsWith(".cas") || nm.endsWith(".cas.gz");
        bool isExe = type == FileTypes::Xex || type == FileTypes::XexGz
                     || nm.endsWith(".xex") || nm.endsWith(".com") || nm.endsWith(".exe")
                     || nm.endsWith(".xex.gz") || nm.endsWith(".com.gz") || nm.endsWith(".exe.gz");
        if (isCas || isExe) {
            QString path = fileName;
#ifdef Q_OS_ANDROID
            if (fileName.startsWith("content:")) path = androidLocalCopy(fileName);
#endif
            if (!path.isEmpty()) {
                if (isCas) loaderLoadCas(path);
                else       loaderLoadXex(path);
#ifdef Q_OS_ANDROID
                QJniObject::callStaticMethod<void>("net/greblus/SerialActivity", "showToast",
                    "(Ljava/lang/String;)V",
                    QJniObject::fromString(tr("Loaded into the cas/xex slot.")).object<jstring>());
#endif
            }
            return;
        }
    }

    if (type == FileTypes::Dir) {
        disk = new FolderImage(sio);
        isDir = true;
    } else if (type == FileTypes::Pro || type == FileTypes::ProGz) {
        disk = new DiskImagePro(sio);
//    } else if (type == FileTypes::Atx || type == FileTypes::AtxGz) {

    } else {
        disk = new SimpleDiskImage(sio);
    }

    if (disk) {
        if (!disk->open(fileName, type)) {
            aspeqtSettings->unmountImage(no);
            delete disk;
            if(g_aspeclFileName.left(1) == "*") emit fileMounted(false);  //
            return;
        }
#ifdef Q_OS_ANDROID
        // Persist read access to the picked document so a saved session can
        // re-mount it after the app is restarted (the image is loaded into a
        // temp working copy, so read access is sufficient).
        if (fileName.startsWith("content:"))
            androidTakePersistable(fileName, false);
#endif
        ejectImage(no);

        sio->installDevice(0x31 + no, disk);

#ifdef Q_OS_ANDROID
        // Keep the "Install DOS" button working for a folder that came back from
        // a restored session: it is mounted through this path (not through
        // mountFolderImage), so record its working dir here too. Otherwise
        // m_folderTemp stays empty and the button reports "not a mounted folder"
        // even though the folder's files show up in the viewer. (The SAF tree URI
        // for write-back isn't restored, so the DOS files land in the mounted
        // working copy — enough for the Atari to boot DOS from it.) For any
        // non-folder mount, drop stale folder tracking so a reused slot is clean.
        if (isDir) {
            if (fileName.startsWith("content:"))
                m_folderTree[no] = fileName;   // enables Install DOS after restore
        } else {
            m_folderTemp.remove(no);
            m_folderTree.remove(no);
        }
#endif

        PCLINK* pclink = reinterpret_cast<PCLINK*>(sio->getDevice(0x6F));
        if(isDir || pclink->hasLink(no+1))
        {
           sio->uninstallDevice(0x6F);
           if(isDir)
           {
               pclink->setLink(no+1, QDir::toNativeSeparators(fileName).toLatin1());
           }
           else
           {
               pclink->resetLink(no+1);
           }
               sio->installDevice(0x6F, pclink);
        }

        m_writeProtect[no] = disk->isReadOnly();

        aspeqtSettings->mountImage(no, fileName, disk->isReadOnly());
        connect(disk, SIGNAL(statusChanged(int)), this, SLOT(deviceStatusChanged(int)), Qt::QueuedConnection);
        deviceStatusChanged(0x31 + no);

        // Extract the file name without the path //
        QString filenamelabel = friendlyName(fileName);

        qDebug() << "!n" << tr("[%1] Mounted '%2' as '%3'.")
                .arg(disk->deviceName())
                .arg(filenamelabel)
                .arg(disk->description());
        if(g_aspeclFileName.left(1) == "*") emit fileMounted(true);  //

    }
}

#ifdef Q_OS_ANDROID


QString Engine::androidDisplayName(const QString &uri)
{
    QJniObject juri = QJniObject::fromString(uri);
    QJniObject res = QJniObject::callStaticObjectMethod(
        "net/greblus/SerialActivity", "displayName",
        "(Ljava/lang/String;)Ljava/lang/String;", juri.object<jstring>());
    return res.isValid() ? res.toString() : QString();
}

void Engine::androidTakePersistable(const QString &uri, bool write)
{
    QJniObject juri = QJniObject::fromString(uri);
    QJniObject::callStaticMethod<void>(
        "net/greblus/SerialActivity", "takePersistable",
        "(Ljava/lang/String;Z)V", juri.object<jstring>(), (jboolean)write);
}


int Engine::androidCopyTreeToDir(const QString &tree, const QString &dest)
{
    QJniObject jt = QJniObject::fromString(tree);
    QJniObject jd = QJniObject::fromString(dest);
    return QJniObject::callStaticMethod<jint>(
        "net/greblus/SerialActivity", "copyTreeToDir",
        "(Ljava/lang/String;Ljava/lang/String;)I", jt.object<jstring>(), jd.object<jstring>());
}

int Engine::androidCopyDirToTree(const QString &src, const QString &tree)
{
    QJniObject js = QJniObject::fromString(src);
    QJniObject jt = QJniObject::fromString(tree);
    return QJniObject::callStaticMethod<jint>(
        "net/greblus/SerialActivity", "copyDirToTree",
        "(Ljava/lang/String;Ljava/lang/String;)I", js.object<jstring>(), jt.object<jstring>());
}

int Engine::androidCopyUriToFile(const QString &uri, const QString &dest)
{
    QJniObject ju = QJniObject::fromString(uri);
    QJniObject jd = QJniObject::fromString(dest);
    return QJniObject::callStaticMethod<jint>(
        "net/greblus/SerialActivity", "copyUriToFile",
        "(Ljava/lang/String;Ljava/lang/String;)I", ju.object<jstring>(), jd.object<jstring>());
}

QString Engine::androidReadablePath(const QString &uri, int slot)
{
    if (!uri.startsWith("content:"))
        return uri;
    QFile probe(uri);
    if (probe.open(QIODevice::ReadOnly)) {   // Qt can read it directly
        probe.close();
        return uri;
    }
    // Qt's QFile can't open this SAF URI; copy the document to a temp file
    // (named with its real name so the slot label is correct) and read that.
    QString name = friendlyName(uri);
    if (name.isEmpty())
        name = QStringLiteral("image");
    QString tmpDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                   + "/mount/" + QString::number(slot);
    QDir(tmpDir).removeRecursively();
    QDir().mkpath(tmpDir);
    QString tmpPath = tmpDir + "/" + name;
    if (androidCopyUriToFile(uri, tmpPath) < 0)
        return uri;
    return tmpPath;
}

QString Engine::androidLocalCopy(const QString &uri)
{
    if (!uri.startsWith("content:"))
        return uri;
    QString name = friendlyName(uri);
    if (name.isEmpty())
        name = QStringLiteral("file");
    QString tmpDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/loader";
    QDir(tmpDir).removeRecursively();
    QDir().mkpath(tmpDir);
    QString tmpPath = tmpDir + "/" + name;
    if (androidCopyUriToFile(uri, tmpPath) < 0)
        return QString();
    return tmpPath;
}

QString Engine::androidChildOrCreate(const QString &tree, const QString &name)
{
    QJniObject jt = QJniObject::fromString(tree);
    QJniObject jn = QJniObject::fromString(name);
    QJniObject r = QJniObject::callStaticObjectMethod(
        "net/greblus/SerialActivity", "ensureInTree",
        "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;",
        jt.object<jstring>(), jn.object<jstring>());
    return r.isValid() ? r.toString() : QString();
}


#endif

void Engine::installDos(int no)
{
    // The two bundled files the Atari needs to boot DOS from a folder.
    const QByteArray boot = readBundled(":/dos/boot.bin");
    const QByteArray dos  = readBundled(":/dos/picodos.sys");
    if (boot.isEmpty() || dos.isEmpty()) {
        toast(tr("Could not copy the DOS files into the folder."));
        return;
    }

#ifdef Q_OS_ANDROID
    // A SAF tree is not a path: find-or-create each document and stream into it.
    const QString tree = m_folderTree.value(no);
    if (tree.isEmpty()) {
        toast(tr("This slot does not hold a mounted folder."));
        return;
    }
    const bool ok = writeIntoTree(tree, "$boot.bin", boot)
                 && writeIntoTree(tree, "picodos.sys", dos);
#else
    FolderImage *folder = qobject_cast<FolderImage *>(sio->getDevice(no + 0x31));
    if (!folder) {
        toast(tr("This slot does not hold a mounted folder."));
        return;
    }
    const QString dir = folder->originalFileName();
    const bool ok = writeIntoDir(dir, "$boot.bin", boot)
                 && writeIntoDir(dir, "picodos.sys", dos);
#endif

    if (ok) {
        deviceStatusChanged(no + 0x31);   // re-read the folder
        qDebug() << "!i" << tr("Installed high-speed MyPicoDOS into the folder. "
                               "Reboot your Atari to load DOS.");
    } else {
        toast(tr("Could not copy the DOS files into the folder."));
    }
}

QByteArray Engine::readBundled(const QString &resource)
{
    QFile f(resource);
    return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
}

#ifdef Q_OS_ANDROID
bool Engine::writeIntoTree(const QString &tree, const QString &name, const QByteArray &bytes)
{
    const QString childUri = androidChildOrCreate(tree, name);
    if (childUri.isEmpty())
        return false;
    ContentFile dst(childUri);
    return dst.open(QIODevice::WriteOnly | QIODevice::Truncate) && dst.write(bytes) >= 0;
}
#else
bool Engine::writeIntoDir(const QString &dir, const QString &name, const QByteArray &bytes)
{
    QFile dst(dir + "/" + name);
    return dst.open(QIODevice::WriteOnly | QIODevice::Truncate) && dst.write(bytes) >= 0;
}
#endif

QString Engine::friendlyName(const QString &name)
{
#ifdef Q_OS_ANDROID
    if (name.startsWith("content:")) {
        QString dn = androidDisplayName(name);
        if (!dn.isEmpty()) return dn;
        // Some providers don't answer the DISPLAY_NAME query. Fall back to the
        // SAF document id in the last URI segment (e.g. percent-encoded
        // "primary:Download/Folder/name.atr") and take just the base name.
        QString seg = QUrl::fromPercentEncoding(name.mid(name.lastIndexOf('/') + 1).toUtf8());
        int cut = qMax(seg.lastIndexOf('/'), seg.lastIndexOf(':'));
        return (cut >= 0) ? seg.mid(cut + 1) : seg;
    }
#endif
    int i = name.lastIndexOf('/');
    if (i < 0) i = name.lastIndexOf('\\');
    return (i >= 0) ? name.mid(i + 1) : name;
}

#ifndef Q_OS_ANDROID
// Outside Android there is no SAF: paths are already readable and already named.
QString Engine::androidDisplayName(const QString &uri) { return friendlyName(uri); }
QString Engine::androidLocalCopy(const QString &uri)   { return uri; }
int     Engine::androidCopyDirToTree(const QString &, const QString &) { return -1; }
#endif



void Engine::toggleWriteProtection(int no)
{
    SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + 0x31));
    if (!img) return;
    m_writeProtect[no] = !m_writeProtect[no];
    img->setReadOnly(m_writeProtect[no]);
    aspeqtSettings->setMountedImageSetting(no, img->originalFileName(), m_writeProtect[no]);
    emit stateChanged();
}



void Engine::loadTranslators()
{
    qApp->removeTranslator(&aspeqt_qt_translator);
    qApp->removeTranslator(&aspeqt_translator);
    if (aspeqtSettings->i18nLanguage().compare("auto") == 0) {
        QString locale = QLocale::system().name();
        aspeqt_translator.load(":/translations/i18n/aspeqt_" + locale);
        aspeqt_qt_translator.load(":/translations/i18n/qt_" + locale);
        aspeqt_qt_translator.load(":/translations/i18n/qtbase_" + locale);
        qApp->installTranslator(&aspeqt_qt_translator);
        qApp->installTranslator(&aspeqt_translator);
    } else if (aspeqtSettings->i18nLanguage().compare("en") != 0) {
        aspeqt_translator.load(":/translations/i18n/aspeqt_" + aspeqtSettings->i18nLanguage());
        aspeqt_qt_translator.load(":/translations/i18n/qt_" + aspeqtSettings->i18nLanguage());
        aspeqt_qt_translator.load(":/translations/i18n/qtbase_" + aspeqtSettings->i18nLanguage());
        qApp->installTranslator(&aspeqt_qt_translator);
        qApp->installTranslator(&aspeqt_translator);
    }
}

int Engine::saveDisk(int no)
{
    SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + 0x31));
    if (!img)
        return SaveFailed;

    if (img->isUnnamed())
        return SaveNeedsName;

    img->lock();
    const bool saved = img->save();
    img->unlock();
    if (!saved)
        return SaveNeedsName;   // offer "save under another name" in the UI

    deviceStatusChanged(0x31 + no);
    return SaveOk;
}
//

// Remote auto-commit toggle sent by the Atari-side AspeCl client.
void Engine::autoCommit(int no)
{
    if (no < 0 || no >= MAX_DISKS) return;
    if (sio->getDevice(no + 0x31)) toggleAutoCommitDisk(no);
}

int Engine::toggleAutoCommitDisk(int no)
{
    SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + 0x31));
    if (!img) return SaveFailed;

    // Auto-commit is engine state now (it used to live in the slot widget's
    // checkable action). Toggling it also commits pending changes, as before.
    m_autoCommit[no] = !m_autoCommit[no];
    qDebug() << "!n" << (m_autoCommit[no] ? tr("[Disk %1] Auto-commit ON.").arg(no + 1)
                                          : tr("[Disk %1] Auto-commit OFF.").arg(no + 1));
    emit stateChanged();

    if (img->isUnnamed())
        return SaveNeedsName;

    img->lock();
    const bool saved = img->save();
    img->unlock();
    if (!saved)
        return SaveNeedsName;

    emit stateChanged();
    return SaveOk;
}
//
bool Engine::saveAsPath(int no, const QString &url)
{
    SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + 0x31));
    if (!img)
        return false;
    const QString fileName = pathFromPickedUrl(url);
    if (fileName.isEmpty())
        return false;

    bool saved = false;
    if (fileName.startsWith(QLatin1String("content:"))) {
        // A SAF document has no extension of its own, so the format comes from
        // the display name the user typed (ATR unless they said otherwise).
        const QString dn = androidDisplayName(fileName);
        FileTypes::FileType st = FileTypes::Atr;
        if (dn.endsWith(".xfd", Qt::CaseInsensitive)) st = FileTypes::Xfd;
        else if (dn.endsWith(".dcm", Qt::CaseInsensitive)) st = FileTypes::Dcm;
        else if (dn.endsWith(".scp", Qt::CaseInsensitive)) st = FileTypes::Scp;
        else if (dn.endsWith(".di", Qt::CaseInsensitive))  st = FileTypes::Di;
        img->lock();
        saved = img->saveAs(fileName, st);
        img->unlock();
    } else {
        img->lock();
        saved = img->saveAs(fileName);
        img->unlock();
        if (saved)
            aspeqtSettings->setLastDiskImageDir(QFileInfo(fileName).absolutePath());
    }

    if (!saved) {
        toast(tr("'%1' cannot be saved.").arg(friendlyName(fileName)));
        return false;
    }

    aspeqtSettings->unmountImage(no);
    aspeqtSettings->mountImage(no, fileName, img->isReadOnly());
    deviceStatusChanged(0x31 + no);
    return true;
}










void Engine::openSessionPath(const QString &url)
{
    const QString picked = pathFromPickedUrl(url);
    if (picked.isEmpty()) {
        return;
    }
    QString fileName;   // path QSettings can read
    QTemporaryFile tmp; // kept alive until the end: holds the local copy
    if (picked.startsWith(QLatin1String("content:"))) {
    // QSettings can't read a content:// URI, so copy the picked session into a
    // local temp file and load QSettings from there.
        if (!tmp.open()) {
            return;
        }
        {
            ContentFile in(picked);
            if (!in.open(QIODevice::ReadOnly)) {
                qCritical() << "!e" << tr("Cannot read '%1'.").arg(friendlyName(picked));
                return;
            }
            tmp.write(in.readAll());
        }
        tmp.flush();
        tmp.close();
        fileName = tmp.fileName();
        g_sessionFile = friendlyName(picked);
        g_sessionFilePath = QString();
    } else {
        fileName = picked;
        aspeqtSettings->setLastSessionDir(QFileInfo(fileName).absolutePath());
        g_sessionFile = QFileInfo(fileName).fileName();
        g_sessionFilePath = QFileInfo(fileName).absolutePath();
    }
// First eject existing images, then mount session images and restore mainwindow position and size //
    ejectAll();

// Pass Session file name, path and Engine title to AspeQtSettings //
#ifdef Q_OS_ANDROID
    // Android always launches into the default session (no named-session file
    // argument), so keep the default as the write target (empty session name).
    // Otherwise mounts from a loaded session are not persisted and the slots
    // come up empty after a restart.
    aspeqtSettings->setSessionFile(QString(), QString());
#else
    aspeqtSettings->setSessionFile(g_sessionFile, g_sessionFilePath);
#endif
    aspeqtSettings->setMainWindowTitle(g_mainWindowTitle);

    aspeqtSettings->loadSessionFromFile(fileName);

#ifdef Q_OS_ANDROID
    // Rebuild the slot column (count + gaps) to match the loaded session before
    // restoring its mounts.
    rebuildSlots();
#endif

    for (int i = 0; i < m_numDisks; i++) {  //
        AspeqtSettings::ImageSettings is;
        is = aspeqtSettings->mountedImageSetting(i);
        mountFile(i, is.fileName, is.isWriteProtected);
    }

    setSession();
}
void Engine::saveSessionPath(const QString &url)
{
    const QString picked = pathFromPickedUrl(url);
    if (picked.isEmpty()) {
        return;
    }

// Save mainwindow position and size to session file //

    if (!picked.startsWith(QLatin1String("content:"))) {
        aspeqtSettings->setLastSessionDir(QFileInfo(picked).absolutePath());
        aspeqtSettings->saveSessionToFile(picked);
        return;
    }

    // QSettings needs a real path: write to a temp file, then copy the bytes to
    // the SAF content:// target.
    QTemporaryFile tmp;
    if (!tmp.open()) {
        return;
    }
    const QString tmpPath = tmp.fileName();
    tmp.close();
    aspeqtSettings->saveSessionToFile(tmpPath);
    QFile in(tmpPath);
    ContentFile out(picked);
    if (in.open(QIODevice::ReadOnly) && out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        out.write(in.readAll());
    } else {
        qCritical() << "!e" << tr("Cannot write to '%1'.").arg(friendlyName(picked));
    }
}





void Engine::on_actionQuit_triggered()
{
}




// ===========================================================================
// QML bridge (branch `qml`): expose the engine's state as QVariant and route
// QML button presses to the existing widget-era slots. See qmlbridge.{h,cpp}.
// ===========================================================================
QVariantList Engine::driveList()
{
    QVariantList out;
    for (int i = 0; i < MAX_DISKS; ++i) {
        if (!m_slotPresent[i]) continue;   // only slots that are present
        QVariantMap m;
        m["hwIndex"] = i;
        SimpleDiskImage *img = qobject_cast<SimpleDiskImage *>(sio->getDevice(0x31 + i));
        if (img) {
            const QString orig = img->originalFileName();
            int slash = orig.lastIndexOf('/');
            QString disp = slash >= 0 ? orig.mid(slash + 1) : orig;
#ifdef Q_OS_ANDROID
            if (orig.startsWith("content:")) disp = friendlyName(orig);
#endif
            m["mounted"]        = true;
            m["isFolder"]       = (qobject_cast<FolderImage *>(img) != nullptr);
            m["fileName"]       = disp;
            m["typeText"]       = img->description();
            m["modified"]       = img->isModified();
            m["autoCommit"]     = m_autoCommit[i];
            m["writeProtected"] = m_writeProtect[i];
        } else {
            m["mounted"]        = false;
            m["isFolder"]       = false;
            m["fileName"]       = QString();
            m["typeText"]       = QString();
            m["modified"]       = false;
            m["autoCommit"]     = false;
            m["writeProtected"] = false;
        }
        out << m;
    }
    return out;
}

QVariantMap Engine::loaderState()
{
    QVariantMap m;
    m["kind"]        = m_loaderKind;
    m["fileName"]    = m_loaderName;
    m["typeText"]    = m_loaderTypeText;
    m["fill"]        = m_loaderFill;
    bool casReady    = (m_loaderKind == 2) && m_casWorker && !m_casWorker->isRunning();
    m["playEnabled"]  = casReady;
    m["retryEnabled"] = !m_loaderFile.isEmpty();
    m["ejectEnabled"] = m_loaderKind != 0;
    m["loading"]      = m_loaderKind == 1 && m_loaderFill > 0.0 && m_loaderFill < 1.0;
    m["casPlaying"]   = m_casWorker && m_casWorker->isRunning();
    return m;
}

QVariantMap Engine::status()
{
    extern bool g_printerEmu;
    QVariantMap m;
    // Disk SIO is paused while a cassette plays, but the player is driving the
    // serial line, so the status icon stays "connected" -- as it did when this
    // was a status-bar pixmap.
    m["running"]   = m_emulationRunning || (m_casWorker && m_casWorker->isRunning());
    m["speed"]     = m_sioStatus;
    m["printerOn"] = g_printerEmu;
    return m;
}

bool Engine::canAddSlot()
{
    for (int i = 0; i < MAX_DISKS; ++i)
        if (!m_slotPresent[i]) return true;
    return false;
}

void Engine::ejectPressed(int i)       { ejectPressedAt(i); }
void Engine::toggleWriteProtect(int i) { toggleWriteProtection(i); }
int Engine::addSlot()
{
    // addSlotAt() fills the lowest empty index; return it so the QML side
    // can scroll to and flash the new slot.
    int i = -1;
    for (int k = 0; k < MAX_DISKS; ++k)
        if (!m_slotPresent[k]) { i = k; break; }
    if (i < 0) return -1;
    addSlotAt();
    return i;
}

// Swap two drives (drag-reorder). Same effect as the widget UI's drop handler:
// device numbers stay put, the mounted images/links exchange places.
void Engine::swapSlots(int source, int slot)
{
    if (source == slot || source < 0 || slot < 0) return;
    if (!m_slotPresent[source] || !m_slotPresent[slot]) return;

    sio->swapDevices(slot + 0x31, source + 0x31);
    aspeqtSettings->swapImages(slot, source);

    PCLINK *pclink = reinterpret_cast<PCLINK *>(sio->getDevice(0x6F));
    if (pclink && (pclink->hasLink(slot + 1) || pclink->hasLink(source + 1))) {
        sio->uninstallDevice(0x6F);
        pclink->swapLinks(slot + 1, source + 1);
        sio->installDevice(0x6F, pclink);
    }
    qDebug() << "!n" << tr("Swapped disk %1 with disk %2.").arg(slot + 1).arg(source + 1);
    emit stateChanged();
}
void Engine::loaderPlay()              { loaderPlayCas(); }
void Engine::toggleSio()
{
    // While a cassette plays it owns the serial line and disk SIO is paused --
    // which is why the status icon shows "connected". Stop the playback here:
    // starting SIO instead would put two threads on the same port (the loading
    // then stutters and never disconnects). loaderCasFinished() brings disk
    // emulation back if it was running before.
    if (m_casWorker && m_casWorker->isRunning()) {
        // Pressing disconnect means "stay disconnected", so don't let
        // loaderCasFinished() bring disk SIO back (a tape reaching its end
        // still does, which is what m_casWasRunning is for).
        m_casWasRunning = false;
        m_casWorker->setPriority(QThread::NormalPriority);
        m_casWorker->wait();
        return;
    }

    if (m_emulationRunning) {
        // Non-blocking: wait() here would freeze the UI until the worker leaves
        // the SIO command it is serving, which is exactly when the user presses
        // stop. sioFinished() closes the port once the thread is really done.
        sio->setPriority(QThread::NormalPriority);
        sio->requestStop();
    } else {
        sio->start(QThread::TimeCriticalPriority);
    }
}
void Engine::togglePrinter()
{
    g_printerEmu = !g_printerEmu;
    qWarning() << "!i" << (g_printerEmu ? tr("Printer emulation started.")
                                        : tr("Printer emulation stopped."));
    emit stateChanged();
}
void Engine::clearLog()
{
    emit stateChanged();
}


// Create + format + mount a new disk image (port of on_actionNewImage_triggered
// without the widget dialog; geometry chosen in the QML CreateDiskDialog).
void Engine::createDisk(int sectorCount, int sectorSize)
{
    if (sectorCount <= 0 || sectorSize <= 0) return;
    SimpleDiskImage *disk = new SimpleDiskImage(sio);
    connect(disk, SIGNAL(statusChanged(int)), this, SLOT(deviceStatusChanged(int)), Qt::QueuedConnection);
    if (!disk->create(++untitledName)) { delete disk; return; }

    DiskGeometry g;
    uint size = (uint)sectorCount * sectorSize;
    if (sectorSize == 256) {
        if (sectorCount >= 3) size -= 384;
        else                  size -= sectorCount * 128;
    }
    g.initialize(size, sectorSize);
    if (!disk->format(g)) { delete disk; return; }

    int no = firstEmptyDiskSlot(0, true);
    ejectImage(no);
    sio->installDevice(0x31 + no, disk);
    deviceStatusChanged(0x31 + no);
    qDebug() << "!n" << tr("[%1] Mounted '%2' as '%3'.")
            .arg(disk->deviceName())
            .arg(friendlyName(disk->originalFileName()))
            .arg(disk->description());
    emit stateChanged();
}
void Engine::ejectAll()
{
    for (int i = m_numDisks - 1; i >= 0; i--) {
        ejectImage(i);
    }
}
QString Engine::printerText()   { return printerOutput ? printerOutput->plainText() : QString(); }
QString Engine::printerTextAtascii() { return printerOutput ? printerOutput->atasciiText() : QString(); }
void Engine::printerClear()     { if (printerOutput) printerOutput->clearText(); }
// The QML side picks the destination (SAF on Android), so no dialog here.
bool Engine::printerSavePath(const QString &url, bool asPdf)
{
    if (!printerOutput)
        return false;
    const QString picked = pathFromPickedUrl(url);
    if (picked.isEmpty())
        return false;

    ContentFile out(picked);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        toast(tr("Cannot save the printout, see the log."));
        qCritical() << "!e" << tr("Cannot write to '%1'.").arg(friendlyName(picked));
        return false;
    }
    const bool ok = asPdf ? printerOutput->savePdf(&out) : printerOutput->saveText(&out);
    out.close();
    if (!ok) {
        toast(tr("Cannot save the printout, see the log."));
        qCritical() << "!e" << tr("Cannot write to '%1'.").arg(friendlyName(picked));
    }
    return ok;
}
void Engine::quit()             { shutdown(); qApp->quit(); }

QStringList Engine::recentFiles()
{
    QStringList out;
    for (int i = 0; i < 10; ++i) {
        const QString fn = aspeqtSettings->recentImageSetting(i).fileName;
        if (fn.isEmpty()) { out << QString(); continue; }
        int slash = fn.lastIndexOf('/');
        QString disp = slash >= 0 ? fn.mid(slash + 1) : fn;
#ifdef Q_OS_ANDROID
        if (fn.startsWith("content:")) disp = friendlyName(fn);
#endif
        out << disp;
    }
    return out;
}

QVariantMap Engine::loadOptions()
{
    QVariantMap o;
    o["iface"]            = (aspeqtSettings->serialPortInterface() == SIO2BT) ? 1 : 0;
    o["handshake"]        = aspeqtSettings->serialPortHandshakingMethod();
    o["baud"]             = aspeqtSettings->serialPortMaximumSpeed();
    o["btName"]           = aspeqtSettings->bluetoothName();
    o["ackDelay"]         = aspeqtSettings->writeACKDelay();
    o["useDivisors"]      = aspeqtSettings->serialPortUsePokeyDivisors();
    o["pokeyDivisor"]     = aspeqtSettings->serialPortPokeyDivisor();
    o["hsExeLoader"]      = aspeqtSettings->useHighSpeedExeLoader();
    o["useCustomCasBaud"] = aspeqtSettings->useCustomCasBaud();
    o["customCasBaud"]    = aspeqtSettings->customCasBaud();
    o["filterUscore"]     = aspeqtSettings->filterUnderscore();
    o["saveWinPos"]       = aspeqtSettings->saveWindowsPos();
    o["largeFont"]        = aspeqtSettings->useLargeFont();
    o["language"]         = aspeqtSettings->i18nLanguage();
    return o;
}

void Engine::applyOptions(const QVariantMap &o)
{
    int ifaceVal = o.value("iface").toInt() == 1 ? SIO2BT : 0;
    aspeqtSettings->setSerialPortName(ifaceVal == SIO2BT ? "SIO2BT" : "SIO2PC");
    aspeqtSettings->setSerialPortInterface(ifaceVal);
    aspeqtSettings->setWriteACKDelay(o.value("ackDelay").toInt());
    aspeqtSettings->setBluetoothName(o.value("btName").toString());
    aspeqtSettings->setSerialPortHandshakingMethod(o.value("handshake").toInt());
    aspeqtSettings->setSerialPortMaximumSpeed(o.value("baud").toInt());
    aspeqtSettings->setSerialPortUsePokeyDivisors(o.value("useDivisors").toBool());
    aspeqtSettings->setSerialPortPokeyDivisor(o.value("pokeyDivisor").toInt());
    aspeqtSettings->setUseHighSpeedExeLoader(o.value("hsExeLoader").toBool());
    aspeqtSettings->setUseCustomCasBaud(o.value("useCustomCasBaud").toBool());
    aspeqtSettings->setCustomCasBaud(o.value("customCasBaud").toInt());
    // Always on now (their checkboxes were removed from the QML options).
    aspeqtSettings->setsaveWindowsPos(true);
    aspeqtSettings->setfilterUnderscore(o.value("filterUscore").toBool());
    aspeqtSettings->setUseLargeFont(true);
#ifdef Q_OS_ANDROID
    int serial_int = aspeqtSettings->serialPortInterface();
    QJniObject::callStaticMethod<void>("net/greblus/SerialActivity", "changeDevice", "(I)V", serial_int);
    QJniObject b_name = QJniObject::fromString(aspeqtSettings->bluetoothName());
    jstring bluetooth_name = b_name.object<jstring>();
    QJniObject::setStaticField("net/greblus/SerialActivity", "bluetoothName", bluetooth_name);
#endif
    aspeqtSettings->setBackend(0);
    aspeqtSettings->setI18nLanguage(o.value("language").toString());
    emit stateChanged();
}

// Available UI languages: Automatic + English + every bundled aspeqt_*.qm
// (native name = that translation's rendering of "English").
QVariantList Engine::languages()
{
    QVariantList langs;
    langs << QVariantMap{ { "code", "auto" }, { "name", tr("Automatic") } };
    langs << QVariantMap{ { "code", "en" },   { "name", "English" } };
    QDir dir(":/translations/i18n/");
    QStringList filters;
    filters << "aspeqt_*.qm";
    const QStringList entries = dir.entryList(filters, QDir::Files, QDir::Name);
    for (const QString &f : entries) {
        QString code = f.mid(7);
        code.replace(".qm", "");
        QString name = code;
        QTranslator t;
        if (t.load(":/translations/i18n/" + f)) {
            QString native = t.translate("OptionsDialog", "English");
            if (!native.isEmpty()) name = native;
        }
        langs << QVariantMap{ { "code", code }, { "name", name } };
    }
    return langs;
}

// -- disk viewer/editor bridge ----------------------------------------------
static AtariFileSystem *createDiskFs(int index, SimpleDiskImage *disk)
{
    switch (index) {
    case 1: return new Dos10FileSystem(disk);
    case 2: return new Dos20FileSystem(disk);
    case 3: return new Dos25FileSystem(disk);
    case 4: return new MyDosFileSystem(disk);
    case 5: return new SpartaDosFileSystem(disk);
    }
    return nullptr;
}

bool Engine::diskOpen(int hwIndex)
{
    diskClose();
    SimpleDiskImage *img = qobject_cast<SimpleDiskImage *>(sio->getDevice(0x31 + hwIndex));
    if (!img) return false;
    m_dvDisk = img;
    m_dvDisk->lock();
    // Open even when the type is unknown (0): the viewer shows a filesystem-type
    // override combo so the user can pick it manually.
    m_dvFsType = img->defaultFileSystem();
    m_dvFs = createDiskFs(m_dvFsType, img);
    m_dvPaths.clear();
    m_dvDirs.clear();
    if (m_dvFs) m_dvDirs.append(m_dvFs->rootDir());
    return true;
}

bool Engine::diskReadOnly()
{
    // Folder images are read-only virtual disks (writeSector is a no-op).
    return !m_dvDisk || qobject_cast<FolderImage *>(m_dvDisk) != nullptr;
}

int Engine::diskFsType() { return m_dvFsType; }


// ---------------------------------------------------------------------------
// Picker results. The QML side asks the user (see FilePicker.qml) and calls
// these with the chosen URL, so the engine never opens a dialog itself.
// ---------------------------------------------------------------------------

// A Qt Quick dialog hands back a URL: keep Android's content:// documents in
// the exact encoding the SAF descriptors expect, and turn file:// into a path.
static QString pathFromPickedUrl(const QString &url)
{
    // Android's SAF picker hands back its own URI string; re-encoding it (or
    // even round-tripping it through QUrl) breaks names containing spaces or
    // brackets, so it is passed through verbatim. Only the desktop dialogs
    // return file:// URLs that need turning into a path.
    if (url.startsWith(QLatin1String("content:")))
        return url;
    if (url.startsWith(QLatin1String("file:")))
        return QUrl(url).toLocalFile();
    return url;
}

void Engine::pickDocument(int reqId, const QString &mimeType)
{
#ifdef Q_OS_ANDROID
    QJniObject::callStaticMethod<void>("net/greblus/SerialActivity", "pickDocument",
        "(ILjava/lang/String;)V", (jint)reqId,
        QJniObject::fromString(mimeType).object<jstring>());
#else
    Q_UNUSED(reqId) Q_UNUSED(mimeType)
#endif
}

void Engine::createDocument(int reqId, const QString &mimeType, const QString &suggestedName)
{
#ifdef Q_OS_ANDROID
    QJniObject::callStaticMethod<void>("net/greblus/SerialActivity", "createDocument",
        "(ILjava/lang/String;Ljava/lang/String;)V", (jint)reqId,
        QJniObject::fromString(mimeType).object<jstring>(),
        QJniObject::fromString(suggestedName).object<jstring>());
#else
    Q_UNUSED(reqId) Q_UNUSED(mimeType) Q_UNUSED(suggestedName)
#endif
}

void Engine::pickFolder(int reqId)
{
#ifdef Q_OS_ANDROID
    QJniObject::callStaticMethod<void>("net/greblus/SerialActivity", "pickFolder",
        "(I)V", (jint)reqId);
#else
    Q_UNUSED(reqId)
#endif
}

void Engine::onDocumentPicked(int reqId, const QString &uri)
{
    emit documentPicked(reqId, uri);
}

QString Engine::startDir(const QString &kind)
{
    if (kind == QLatin1String("disk"))    return aspeqtSettings->lastDiskImageDir();
    if (kind == QLatin1String("folder"))  return aspeqtSettings->lastFolderImageDir();
    if (kind == QLatin1String("exe"))     return aspeqtSettings->lastExeDir();
    if (kind == QLatin1String("session")) return aspeqtSettings->lastSessionDir();
    return QString();
}

void Engine::mountDiskPath(int no, const QString &url)
{
    const QString fileName = pathFromPickedUrl(url);
    if (fileName.isEmpty())
        return;
    if (!fileName.startsWith(QLatin1String("content:")))
        aspeqtSettings->setLastDiskImageDir(QFileInfo(fileName).absolutePath());
    mountFileWithDefaultProtection(no, fileName);
}

void Engine::mountFolderPath(int no, const QString &url)
{
    const QString fileName = pathFromPickedUrl(url);
    if (fileName.isEmpty())
        return;
#ifdef Q_OS_ANDROID
    // The folder image reads files in place through content:// descriptors, so
    // the tree permission has to survive a restart.
    androidTakePersistable(fileName, true);
    m_folderTree[no] = fileName;
#else
    aspeqtSettings->setLastFolderImageDir(fileName);
#endif
    mountFileWithDefaultProtection(no, fileName);
}

void Engine::loaderLoadPath(const QString &url)
{
    const QString picked = pathFromPickedUrl(url);
    if (picked.isEmpty())
        return;
    // Always copy a content:// pick to a real temp file: QFile on a SAF stream
    // can pass a one-shot open() probe yet fail the repeated sequential reads /
    // atEnd() that CAS parsing and the boot loader need.
    const QString path = androidLocalCopy(picked);
    if (path.isEmpty()) {
        qWarning() << "!i" << tr("Failed to load '%1'.").arg(friendlyName(picked));
        return;
    }
    if (!picked.startsWith(QLatin1String("content:")))
        aspeqtSettings->setLastExeDir(QFileInfo(picked).absolutePath());

    const FileTypes::FileType t = FileTypes::getFileType(path);
    if (t == FileTypes::Cas || t == FileTypes::CasGz)
        loaderLoadCas(path);
    else if (t == FileTypes::Xex || t == FileTypes::XexGz)
        loaderLoadXex(path);
    else
        toast(tr("Pick an Atari executable (.xex/.com/.exe) or a cassette image (.cas)."));
}

void Engine::toast(const QString &text)
{
#ifdef Q_OS_ANDROID
    QJniObject::callStaticMethod<void>("net/greblus/SerialActivity", "showToast",
        "(Ljava/lang/String;)V", QJniObject::fromString(text).object<jstring>());
#else
    Q_UNUSED(text)
#endif
}

bool Engine::diskSetFsType(int index)
{
    if (!m_dvDisk) return false;

    // Build the new file system first: if the image plainly isn't of that DOS
    // type, keep the current one and let the caller tell the user.
    AtariFileSystem *fs = createDiskFs(index, m_dvDisk);
    if (fs && !fs->isValid()) {
        delete fs;
        return false;
    }

    if (m_dvFs) { delete m_dvFs; m_dvFs = nullptr; }
    m_dvFsType = index;
    m_dvFs = fs;
    m_dvPaths.clear();
    m_dvDirs.clear();
    if (m_dvFs) m_dvDirs.append(m_dvFs->rootDir());
    return true;
}

void Engine::diskClose()
{
    if (m_dvFs)   { delete m_dvFs; m_dvFs = nullptr; }
    if (m_dvDisk) { m_dvDisk->unlock(); m_dvDisk = nullptr; }
    m_dvDirs.clear();
    m_dvPaths.clear();
}

QVariantList Engine::diskEntries()
{
    QVariantList out;
    if (!m_dvFs || m_dvDirs.isEmpty()) return out;
    const QList<AtariDirEntry> entries = m_dvFs->getEntries(m_dvDirs.last());
    for (const AtariDirEntry &e : entries) {
        QVariantMap m;
        m["name"]  = e.niceName();
        m["size"]  = e.size;
        m["isDir"] = bool(e.attributes & AtariDirEntry::Directory);
        m["attrs"] = e.attributeNames();
        m["date"]  = e.dateTime.isValid() ? e.dateTime.toString("yyyy-MM-dd") : QString();
        out << m;
    }
    return out;
}

QString Engine::diskPath()
{
    if (!m_dvFs || !m_dvDisk) return QString();
    QString p = QString("D%1:").arg(m_dvDisk->deviceNo() - 0x30);
    for (const QString &s : m_dvPaths) p.append(s + ">");
    return p;
}

bool Engine::diskCanParent() { return !m_dvPaths.isEmpty(); }

void Engine::diskEnter(int row)
{
    if (!m_dvFs || m_dvDirs.isEmpty()) return;
    const QList<AtariDirEntry> entries = m_dvFs->getEntries(m_dvDirs.last());
    if (row < 0 || row >= entries.size()) return;
    const AtariDirEntry &e = entries.at(row);
    if (!(e.attributes & AtariDirEntry::Directory)) return;
    m_dvPaths.append(e.name());
    m_dvDirs.append(e.firstSector);
}

void Engine::diskParent()
{
    if (m_dvPaths.isEmpty()) return;
    m_dvPaths.removeLast();
    m_dvDirs.removeLast();
}

static QList<AtariDirEntry> dvPickRows(AtariFileSystem *fs, quint16 dir, const QVariantList &rows)
{
    QList<AtariDirEntry> out;
    const QList<AtariDirEntry> entries = fs->getEntries(dir);
    for (const QVariant &v : rows) {
        int r = v.toInt();
        if (r >= 0 && r < entries.size()) out.append(entries.at(r));
    }
    return out;
}

void Engine::diskSetTextConversion(bool on)
{
    if (m_dvFs) m_dvFs->setTextConversion(on);
}

bool Engine::diskExtractPath(const QVariantList &rows, const QString &url)
{
    if (!m_dvFs || m_dvDirs.isEmpty() || rows.isEmpty()) return false;
    QList<AtariDirEntry> sel = dvPickRows(m_dvFs, m_dvDirs.last(), rows);
    if (sel.isEmpty()) return false;

    const QString target = pathFromPickedUrl(url);
    if (target.isEmpty()) return false;
    aspeqtSettings->setLastExtractDir(target);

    if (!target.startsWith(QLatin1String("content:"))) {
        if (!m_dvFs->extractRecursive(sel, target)) {
            toast(tr("Cannot extract the files, see the log."));
            return false;
        }
        return true;
    }

    // A SAF tree can't be written through QFile: extract into a scratch dir and
    // hand the whole directory to the content resolver.
    const QString tmpDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/extract";
    QDir(tmpDir).removeRecursively();
    QDir().mkpath(tmpDir);
    if (!m_dvFs->extractRecursive(sel, tmpDir)) {
        toast(tr("Cannot extract the files, see the log."));
        return false;
    }
    if (androidCopyDirToTree(tmpDir, target) < 0) {
        toast(tr("Cannot extract the files, see the log."));
        return false;
    }
    QDir(tmpDir).removeRecursively();
    return true;
}

bool Engine::diskDelete(const QVariantList &rows)
{
    if (!m_dvFs || m_dvDirs.isEmpty() || rows.isEmpty()) return false;
    QList<AtariDirEntry> sel = dvPickRows(m_dvFs, m_dvDirs.last(), rows);
    if (sel.isEmpty()) return false;
    if (!m_dvFs->deleteRecursive(sel)) {
        toast(tr("Cannot delete the files, see the log."));
        return false;
    }
    return true;
}

bool Engine::diskAddFilesPath(const QString &url)
{
    if (!m_dvFs || m_dvDirs.isEmpty()) return false;
    const QString picked = pathFromPickedUrl(url);
    if (picked.isEmpty()) return false;

    QStringList files;
    if (picked.startsWith(QLatin1String("content:"))) {
        // insertRecursive works on real files, so stage the document under its
        // display name (which is also the name it gets on the Atari disk).
        // ContentFile, not QFile: Qt's Android file engine re-encodes the URI
        // and fails on names holding parens or spaces (silently, until now).
        ContentFile src(picked);
        if (!src.open(QIODevice::ReadOnly)) {
            toast(tr("Cannot add the file, see the log."));
            qCritical() << "!e" << tr("Cannot read '%1'.").arg(friendlyName(picked));
            return false;
        }
        const QByteArray bytes = src.readAll();
        src.close();
        QString displayName = androidDisplayName(picked);
        if (displayName.isEmpty()) displayName = QStringLiteral("FILE");
        const QString tmpDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/addfile";
        QDir(tmpDir).removeRecursively();
        QDir().mkpath(tmpDir);
        const QString tmpPath = tmpDir + "/" + displayName;
        QFile dst(tmpPath);
        if (!dst.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            toast(tr("Cannot add the file, see the log."));
            qCritical() << "!e" << tr("Cannot write to '%1'.").arg(tmpPath);
            return false;
        }
        dst.write(bytes);
        dst.close();
        files.append(tmpPath);
    } else {
        files.append(picked);
    }

    if (m_dvFs->insertRecursive(m_dvDirs.last(), files).isEmpty()) {
        toast(tr("Cannot add the file, see the log."));
        return false;
    }
    return true;
}

void Engine::mountRecent(int index)
{
    // Straight from the settings: this used to read the text of the (invisible)
    // widget menu action, so it only worked if that menu had been refreshed.
    if (index < 0 || index >= 10)
        return;
    const QString fileName = aspeqtSettings->recentImageSetting(index).fileName;
    if (fileName.isEmpty())
        return;
    mountFileWithDefaultProtection(firstEmptyDiskSlot(), fileName);
}
