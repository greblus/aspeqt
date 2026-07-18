#include "mainwindow.h"
#include "ui_mainwindow.h"

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

#include <QEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QUrl>
#include <QFileDialog>
#include <QFile>
#include <QTemporaryFile>
#include <QStandardPaths>
#include <QDir>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <QScrollBar>
#include <QScrollArea>
#include <QScroller>
#include <QScrollerProperties>
#include <QProgressBar>
#include <QMouseEvent>
#include <QPainter>
#ifdef Q_OS_ANDROID
// "Remove slot" affordance for an empty slot: the tango "unreadable" emblem
// (red X badge).
static QIcon removeSlotIcon()
{
    return QIcon(":/icons/tango-icons/emblems/emblem-unreadable.svg");
}
// "Install DOS" affordance for a mounted folder: the hard-disk icon with a
// "DOS" caption overlaid.
static QIcon dosDriveIcon()
{
    static QIcon cached;
    if (cached.isNull()) {
        // Force devicePixelRatio 1 so the pixmap's physical size equals the
        // logical size QPainter draws in (otherwise the caption lands off-icon).
        const int S = 96;
        QPixmap pm = QIcon(":/icons/tango-icons/devices/drive-harddisk.svg").pixmap(QSize(S, S), 1.0);
        QPainter p(&pm);
        QFont f = p.font();
        f.setBold(true);
        f.setPixelSize(40);
        p.setFont(f);
        QRect r(0, S / 2 - 4, S, S / 2);
        p.setPen(QColor(255, 255, 255));           // white halo for contrast
        for (int dx = -2; dx <= 2; ++dx)
            for (int dy = -2; dy <= 2; ++dy)
                p.drawText(r.translated(dx, dy), Qt::AlignCenter, "DOS");
        p.setPen(QColor(40, 40, 40));
        p.drawText(r, Qt::AlignCenter, "DOS");
        p.end();
        cached = QIcon(pm);
    }
    return cached;
}
#endif
#include <QTranslator>
#include <QMessageBox>
#include <QWidget>
#include <QDrag>
#include <QtDebug>
//#include <QDesktopWidget>
#include <QFont>
#include <QTextCodec>
#include <QLayout>

#include "atarifilesystem.h"
#include "miscutils.h"

#ifdef Q_OS_ANDROID
#include <QJniObject>
#endif

#include <QScreen>
#include <QTimer>
#include "math.h"

AspeqtSettings *aspeqtSettings;
MainWindow *mainWindow;

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
        mainWindow->doLogMessage(localMsg.at(1), displayMsg);
    }
}

void MainWindow::doLogMessage(int type, const QString &msg)
{
    emit logMessage(type, msg);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{

    /* Setup the logging system */
    mainWindow = this;
    g_aspeQtAppPath = QCoreApplication::applicationDirPath();
    g_disablePicoHiSpeed = false;
    logFile = new QFile(QDir::temp().absoluteFilePath("aspeqt.log"));
    logFile->open(QFile::WriteOnly | QFile::Truncate | QFile::Unbuffered | QFile::Text);
    logMutex = new QMutex();
    connect(this, SIGNAL(logMessage(int,QString)), this, SLOT(uiMessage(int,QString)), Qt::QueuedConnection);
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
        QMessageBox::information(this, tr("Migrate Settings"), tr("This version of AspeQt uses a different repository "
                                          "for storing its global settings.\nWe will now migrate the existing "
                                          "settings to their new repository, note that settings stored in your existing "
                                          "AspeQt session files are not affected by this change."), QMessageBox::Ok);
        for (int i=0; i<oldKeys.size(); ++i) {
            newSettings.setValue(oldKeys.value(i), oldSettings.value(oldKeys.value(i)));
        }
        oldSettings.clear();
        QMessageBox::information(this, tr("Migrate Settings"), tr("Setting were migrated successfuly."), QMessageBox::Ok);
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
    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    QTextCodec::setCodecForLocale(codec);
    #endif

    
    /* Setup UI */
    ui->setupUi(this);

#ifdef Q_OS_ANDROID
    // Android's native options menu only opens submenus, it cannot trigger a
    // bare top-level action. So make the single-entry "Options" submenu open
    // the dialog as soon as it is tapped, giving a one-tap Options item.
    connect(ui->menu_Tools, &QMenu::aboutToShow, this, [this]{
        QTimer::singleShot(0, ui->actionOptions, &QAction::trigger);
    });
    // Promote "Quit" from the File submenu to its own one-tap top-level menu entry
    // (same aboutToShow trick, since a bare top-level action can't be triggered).
    ui->menu_File->removeAction(ui->actionQuit);
    {
        QMenu *quitMenu = ui->menuBar->addMenu(ui->actionQuit->text());
        quitMenu->addAction(ui->actionQuit);
        connect(quitMenu, &QMenu::aboutToShow, this, [this]{
            QTimer::singleShot(0, ui->actionQuit, &QAction::trigger);
        });
    }
    // Drop the Help menu (About + Documentation) from the menu bar entirely.
    ui->menuBar->removeAction(ui->menu_Help->menuAction());
#endif

    /* I love ugly hacks */
    QScreen *screen = qApp->screens().at(0);

    int scrh = screen->size().height();
    int scrw = screen->size().width();

    float resx = scrw/screen->physicalDotsPerInchX();
    float resy = scrh/screen->physicalDotsPerInchY();

    ssize = sqrt(resx*resx + resy*resy);

    QWidget *central = ui->centralWidget;
    QList<QToolButton *> allbtns = central->findChildren<QToolButton *>();

    if (scrw > scrh)
        btnsize = scrh*70/800;
    else
        btnsize = scrw*70/800;

    // Keep the drive rows compact: only a few px taller than the icons, so more
    // vertical space is left for the log (textEdit), especially in landscape.
    int iconPx = qRound((btnsize - 8) * 1.3);
    int btnH   = iconPx + 2;
    int rowH   = iconPx + 6;
    for (int i = 1; i <= 6; ++i) {
        QFrame *f = central->findChild<QFrame *>(QString("horizontalFrame_%1").arg(i));
        if (f) {
            f->setMinimumHeight(rowH);
            f->setMaximumHeight(rowH);
            if (f->layout()) {
                f->layout()->setContentsMargins(3, 1, 3, 1);
                f->layout()->setSpacing(4);   // tighter gap between buttons
            }
        }
    }

    foreach(QToolButton* btn, allbtns) {
        btn->setMinimumHeight(btnH);
        btn->setMinimumWidth(btnH);     // square: width == height
        btn->setMaximumHeight(btnH);
        btn->setMaximumWidth(btnH);
        btn->setIconSize(QSize(iconPx, iconPx));
    }

    if (ssize < 6) ui->textEdit->setVisible(false);
        else {
            // Give the top spacer a fixed height (~ the Android action bar) so drive
            // slot 1 is never hidden under it, and collapse the bottom spacer. The log
            // (textEdit) is then the only vertically-expanding widget, so it fills all
            // remaining height in both portrait and landscape.
            int abPx = qRound(0.6 * screen->physicalDotsPerInchY());
            ui->verticalSpacer_2->changeSize(0, abPx, QSizePolicy::Fixed, QSizePolicy::Fixed);
            ui->verticalSpacer->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
            ui->gridLayout->invalidate();
        }

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
               QMessageBox::question(this, tr("Session file error"),
               tr("Requested session file not found in the given directory path or the path is incorrect. AspeQt will continue with default session configuration."), QMessageBox::Ok);
               g_sessionFile = g_sessionFilePath = "";
           }
       } else {
           if (AspeQtArgs.at(1) != "") {
               g_sessionFile = AspeQtArgs.at(1);
               g_sessionFilePath = QDir::currentPath();
               sess.setFileName(g_sessionFile);
               if (!sess.exists()) {
                   QMessageBox::question(this, tr("Session file error"),
                   tr("Requested session file not found in the application's current directory path\n (No path was specified). AspeQt will continue with default session configuration."), QMessageBox::Ok);
                   g_sessionFile = g_sessionFilePath = "";
               }
           }
         }
    }
    // Pass Session file name, path and MainWindow title to AspeQtSettings //
    aspeqtSettings->setSessionFile(g_sessionFile, g_sessionFilePath);
    aspeqtSettings->setMainWindowTitle(g_mainWindowTitle);

    // Display Session name, and restore session parameters if session file was specified //
    g_mainWindowTitle = tr("AspeQt - Atari Serial Peripheral Emulator for Qt");
    if (g_sessionFile != "") {
        setWindowTitle(g_mainWindowTitle + tr(" -- Session: ") + g_sessionFile);
        aspeqtSettings->loadSessionFromFile(g_sessionFilePath+g_sessionFile);
    } else {
        setWindowTitle(g_mainWindowTitle);
    }
#ifdef Q_OS_ANDROID
    // Fill the *available* screen (excludes the system status/navigation bars) and
    // keep filling it across rotations, so the bottom status bar stays on-screen.
    // A saved desktop size would leave the window at the portrait width (~half the
    // screen) in landscape.
    setGeometry(screen->availableGeometry());
    connect(screen, &QScreen::availableGeometryChanged, this, [this](const QRect &g){ setGeometry(g); });
#else
    setGeometry(aspeqtSettings->lastHorizontalPos(),aspeqtSettings->lastVerticalPos(),aspeqtSettings->lastWidth(),aspeqtSettings->lastHeight());
#endif

    /* Setup status bar */
    sbIcon = qRound((btnsize - 5) * 0.8 * 4.0 / 3.0);   // status bar 1/3 taller / icons bigger
    speedLabel = new QLabel(this);
    onOffLabel = new QLabel(this);
    prtOnOffLabel = new QLabel(this);
    clearMessagesLabel = new QLabel(this);
    speedLabel->setText(tr(""));
    onOffLabel->setMinimumWidth(17);
    prtOnOffLabel->setMinimumWidth(15);
    prtOnOffLabel->setPixmap(QIcon(":/icons/tango-icons/devices/printer.svg").pixmap(sbIcon, sbIcon, QIcon::Normal));  //
    prtOnOffLabel->setToolTip(ui->actionPrinterEmulation->toolTip());
    prtOnOffLabel->setStatusTip(ui->actionPrinterEmulation->statusTip());

    clearMessagesLabel->setMinimumWidth(17);
    clearMessagesLabel->setPixmap(QIcon(":/icons/tango-icons/actions/edit-clear.svg").pixmap(sbIcon, sbIcon, QIcon::Normal));
    clearMessagesLabel->setToolTip(tr("Clear messages"));
    clearMessagesLabel->setStatusTip(clearMessagesLabel->toolTip());

    speedLabel->setMinimumWidth(80);
#ifdef Q_OS_ANDROID
    // Keep the status bar (the bottom "toolbar") compact: ~20% shorter. Add a
    // right margin so the last icon (clear-log) isn't jammed against the edge-to-
    // edge screen edge / rounded corner where it's hard to tap.
    ui->statusBar->setContentsMargins(0, 0, sbIcon, 0);
    ui->statusBar->setFixedHeight(sbIcon + 4);
    // The status-bar message font defaults too large on Android; scale it to
    // the compact bar height.
    {
        QFont sbFont = ui->statusBar->font();
        sbFont.setPointSize(14);
        ui->statusBar->setFont(sbFont);
    }
#endif

    ui->statusBar->addPermanentWidget(speedLabel);
    ui->statusBar->addPermanentWidget(onOffLabel);
    ui->statusBar->addPermanentWidget(prtOnOffLabel);
    ui->statusBar->addPermanentWidget(clearMessagesLabel);
    changeFonts();

#ifdef Q_OS_ANDROID
    // Slot presence from the session (used to be done by androidBuildSlots()).
    androidRebuildSlots();
#else
    m_numDisks = g_numberOfDisks;
#endif

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

    /* Restore application state */
    for (int i = 0; i < m_numDisks; i++) {      //
        AspeqtSettings::ImageSettings is;
        is = aspeqtSettings->mountedImageSetting(i);
        mountFile(i, is.fileName, is.isWriteProtected);
    }
    updateRecentFileActions();

    // SmartDevice (ApeTime + URL submit)
    SmartDevice *smart = new SmartDevice(sio);
    sio->installDevice(SMART_CDEVIC, smart);

    // AspeQt Client  //
    AspeCl *acl = new AspeCl(sio);
    sio->installDevice(0x46, acl);

    textPrinterWindow = new TextPrinterWindow();
    // Documentation Display

    connect(textPrinterWindow, SIGNAL(closed()), this, SLOT(textPrinterWindowClosed()));

    Printer *printer = new Printer(sio);
    connect(printer, SIGNAL(print(QString)), textPrinterWindow, SLOT(print(QString)));
#ifdef ASPEQT_QML
    connect(printer, SIGNAL(print(QString)), this, SIGNAL(qmlPrinterTextChanged()));
#endif
    sio->installDevice(0x40, printer);
    untitledName = 0;

    connect(&trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
    trayIcon.setIcon(windowIcon());

    // Connections needed for remotely mounting a disk image & Toggle Auto-Commit //
    connect (acl, SIGNAL(findNewSlot(int,bool)), this, SLOT(firstEmptyDiskSlot(int,bool)));
    connect (this, SIGNAL(newSlot(int)), acl, SLOT(gotNewSlot(int)));
    connect (acl, SIGNAL(mountFile(int,QString)), this, SLOT(mountFileWithDefaultProtection(int,QString)));
    connect (this, SIGNAL(fileMounted(bool)), acl, SLOT(fileMounted(bool)));
    connect (acl, SIGNAL(toggleAutoCommit(int)), this, SLOT(autoCommit(int)));

}

MainWindow::~MainWindow()
{
    if (ui->actionStartEmulation->isChecked()) {
        ui->actionStartEmulation->trigger();
    }

    delete aspeqtSettings;
    delete sio;

    delete ui;

    qDebug() << "!d" << tr("AspeQt stopped at %1.").arg(QDateTime::currentDateTime().toString());
    qInstallMessageHandler(0);
    delete logMutex;
    delete logFile;
}

 void MainWindow::closeEvent(QCloseEvent *event)
{
    // Save various session settings  //
    if (aspeqtSettings->saveWindowsPos()) {
        if (g_miniMode) {
            saveMiniWindowGeometry();
        } else {
            saveWindowGeometry();
        }
    }
    if (g_sessionFile != "") aspeqtSettings->saveSessionToFile(g_sessionFilePath + "/" + g_sessionFile);
    aspeqtSettings->setD9DOVisible(g_D9DOVisible);
    bool wasRunning = ui->actionStartEmulation->isChecked();
    QMessageBox::StandardButton answer = QMessageBox::No;

    if (wasRunning) {
        ui->actionStartEmulation->trigger();
    }

    int toBeSaved = 0;

    for (int i = 0; i < m_numDisks; i++) {      //
        SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(i + 0x31));
        if (img && img->isModified()) {
            toBeSaved++;
        }
    }

    for (int i = 0; i < m_numDisks; i++) {      //
        SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(i + 0x31));
        if (img && img->isModified()) {
            toBeSaved--;
            answer = saveImageWhenClosing(i, answer, toBeSaved);
            if (answer == QMessageBox::NoToAll) {
                break;
            }
            if (answer == QMessageBox::Cancel) {
                if (wasRunning) {
                    ui->actionStartEmulation->trigger();
                }
                event->ignore();
                return;
            }
        }
    }

    delete textPrinterWindow;
    //

    for (int i = 0x31; i < 0x39; i++) {
        SimpleDiskImage *s = qobject_cast <SimpleDiskImage*> (sio->getDevice(i));
        if (s) {
            s->close();
        }
    }

    aspeqtSettings->sync();   // flush settings now; the process may be killed on exit
    event->accept();

}

void MainWindow::hideEvent(QHideEvent *event)
{
    if (aspeqtSettings->minimizeToTray()) {
        trayIcon.show();
        oldWindowFlags = windowFlags();
        oldWindowStates = windowState();
        setWindowFlags(Qt::Widget);
        hide();
        event->ignore();
        return;
    }
    QMainWindow::hideEvent(event);
}

void MainWindow::show()
{
    QMainWindow::show();
    if (shownFirstTime) {
        /* Open options dialog if it's the first time */
        if (aspeqtSettings->isFirstTime()) {
            if (QMessageBox::Yes == QMessageBox::question(this, tr("First run"),
                                       tr("You are running AspeQt for the first time.\n\nDo you want to open the options dialog?"),
                                       QMessageBox::Yes, QMessageBox::No)) {
                ui->actionOptions->trigger();
            }
        }
        qDebug() << "!d" << "Starting emulation";

        ui->actionStartEmulation->trigger();
    }
}
void MainWindow::androidRebuildSlots()
{
    for (int i = 0; i < MAX_DISKS; ++i)
        m_slotPresent[i] = false;
    m_numDisks = aspeqtSettings->numberOfDisks();
    if (m_numDisks < 1)          m_numDisks = DEFAULT_DISKS;
    if (m_numDisks > MAX_DISKS)  m_numDisks = MAX_DISKS;
    for (int i = 0; i < m_numDisks; ++i)
        if (aspeqtSettings->slotPresent(i))   // removed slot -> gap
            m_slotPresent[i] = true;
#ifdef ASPEQT_QML
    emit qmlChanged();
#endif
}

// Hide the "+" row once every hardware slot index is in use.
void MainWindow::androidAddSlot()
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
void MainWindow::androidRemoveSlot(int i)
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
#ifdef ASPEQT_QML
    emit qmlChanged();
#endif
}

// Eject button: eject a mounted image; on an already-empty slot the button
// shows a trash icon and removes the slot instead.
void MainWindow::androidEjectPressed(int i)
{
    if (sio->getDevice(i + 0x31))
        ejectImage(i);
    else
        androidRemoveSlot(i);
}

// ---- top loader slot: inline XEX autoboot / CAS cassette player -------------

// Build the always-on-top loader slot (badge "cas/xex", load / play / retry /
// eject buttons, and a load progress bar) and insert it above the disk slots.
void MainWindow::loaderSetFill(double frac)
{
    m_loaderFill = frac;
#ifdef ASPEQT_QML
    emit qmlLoaderProgress();   // light: only the loader fill
#endif
}

void MainWindow::loaderUpdateButtons()
{
#ifdef ASPEQT_QML
    emit qmlChanged();
#endif
}


// XEX: install the autoboot loader on D1 and let the running emulation boot it,
// driving the progress bar from the loader's blockRead signal.
void MainWindow::loaderLoadXex(const QString &path)
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
    connect(m_autoBoot, &AutoBoot::blockRead, this, &MainWindow::loaderBlockRead);
    connect(m_autoBoot, &AutoBoot::loaderDone, this, &MainWindow::loaderBooterDone);

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
void MainWindow::loaderLoadCas(const QString &path)
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
void MainWindow::loaderPlayCas()
{
    if (m_loaderKind != 2 || !m_casWorker || m_casWorker->isRunning())
        return;
    // The cassette player opens the single serial port itself, so disk emulation
    // must be paused while it runs (AspeQt never drives cassette + disk at once).
    m_casWasRunning = ui->actionStartEmulation->isChecked();
    if (m_casWasRunning) {
        ui->actionStartEmulation->trigger();
        sio->wait();
        qApp->processEvents();
    }
    // Disk SIO is paused, but the cassette player IS driving the serial line, so
    // show an "active" status icon instead of the "disconnected" one that pausing
    // the SIO worker left behind.
    onOffLabel->setPixmap(QIcon(":/icons/tango-icons/actions/media-playback-stop.svg").pixmap(sbIcon, sbIcon, QIcon::Normal, QIcon::On));
    onOffLabel->setToolTip(tr("Playing cassette image"));
    onOffLabel->setStatusTip(tr("Playing cassette image"));
    connect(m_casWorker, &CassetteWorker::statusChanged, this, &MainWindow::loaderCasStatus, Qt::QueuedConnection);
    connect(m_casWorker, &QThread::finished, this, &MainWindow::loaderCasFinished);
    m_casWorker->start(QThread::TimeCriticalPriority);
    m_casTimer = new QTimer(this);
    connect(m_casTimer, &QTimer::timeout, this, &MainWindow::loaderCasTick);
    m_casTimer->start(1000);
    loaderUpdateButtons();
    qDebug() << "!i" << tr("Playing back cassette image.");
}

void MainWindow::loaderCasStatus(int remainingTime)
{
    if (!m_casWorker) return;
    m_casTotal = m_casWorker->mTotalDuration;
    m_casRemaining = remainingTime;
    loaderSetFill(m_casTotal > 0 ? double(m_casTotal - m_casRemaining) / m_casTotal : 0.0);
}

void MainWindow::loaderCasTick()
{
    if (m_casRemaining < 1000)
        m_casRemaining = 1000;
    loaderCasStatus(m_casRemaining - 1000);
}

void MainWindow::loaderCasFinished()
{
    if (m_casTimer) { m_casTimer->stop(); }
    loaderSetFill(0);            // fill disappears when done
    // Resume disk emulation if we paused it for the cassette.
    if (m_casWasRunning) {
        m_casWasRunning = false;
        if (!ui->actionStartEmulation->isChecked())
            ui->actionStartEmulation->trigger();
    }
    // If emulation didn't resume, restore the "disconnected" status icon (when it
    // does resume, sioStarted() sets the running icon).
    if (!ui->actionStartEmulation->isChecked()) {
        onOffLabel->setPixmap(QIcon(":/icons/tango-icons/actions/media-playback-start.svg").pixmap(sbIcon, sbIcon, QIcon::Normal, QIcon::On));
        onOffLabel->setToolTip(ui->actionStartEmulation->toolTip());
        onOffLabel->setStatusTip(ui->actionStartEmulation->statusTip());
    }
    loaderUpdateButtons();
    qDebug() << "!i" << tr("Cassette playback finished.");
}

void MainWindow::loaderBlockRead(int current, int all)
{
    loaderSetFill(all > 0 ? double(current) / all : 0.0);
}

void MainWindow::loaderBooterDone()
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
void MainWindow::loaderEject()
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
        if (!ui->actionStartEmulation->isChecked())
            ui->actionStartEmulation->trigger();
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
void MainWindow::loaderRetry()
{
    if (m_loaderFile.isEmpty()) return;
    QString path = m_loaderFile;   // loaderEject() (via load*) clears m_loaderKind
    int kind = m_loaderKind;
    if (kind == 2)
        loaderLoadCas(path);
    else
        loaderLoadXex(path);
}

void MainWindow::logChanged(QString text)
{
    emit sendLogTextChange(text);

}

void MainWindow::saveWindowGeometry()
{
    aspeqtSettings->setLastHorizontalPos(geometry().x());
    aspeqtSettings->setLastVerticalPos(geometry().y());
    aspeqtSettings->setLastWidth(geometry().width());
    aspeqtSettings->setLastHeight(geometry().height());
}

void MainWindow::saveMiniWindowGeometry()
{
    aspeqtSettings->setLastMiniHorizontalPos(geometry().x());
    aspeqtSettings->setLastMiniVerticalPos(geometry().y());
}

void MainWindow::on_actionToggleShade_triggered()
{
    if (g_shadeMode) {
        setWindowFlags(Qt::WindowSystemMenuHint);
        setWindowOpacity(1.0);
        g_shadeMode = false;
        QMainWindow::show();
    } else {
        setWindowFlags(Qt::FramelessWindowHint);
        setWindowOpacity(0.25);
        g_shadeMode = true;
        QMainWindow::show();
    }
}

// Toggle Mini Mode //
void MainWindow::on_actionToggleMiniMode_triggered()
{
    if(g_miniMode){
        ui->horizontalFrame_1->setFixedHeight(100);
        ui->buttonMountDisk_1->setFixedHeight(70);
        ui->buttonMountDisk_1->setFixedWidth(70);
        ui->buttonMountFolder_1->setFixedHeight(70);
        ui->buttonMountFolder_1->setFixedWidth(70);
        ui->buttonSave_1->setFixedHeight(70);
        ui->buttonSave_1->setFixedWidth(70);
        ui->autoSave_1->setFixedHeight(70);
        ui->autoSave_1->setFixedWidth(70);
        ui->buttonEditDisk_1->setFixedHeight(70);
        ui->buttonEditDisk_1->setFixedWidth(70);
        ui->buttonEject_1->setFixedHeight(70);
        ui->buttonEject_1->setFixedWidth(70);
        ui->horizontalFrame_2->setVisible(true);
        ui->horizontalFrame_3->setVisible(true);
        ui->horizontalFrame_4->setVisible(true);
        ui->horizontalFrame_5->setVisible(true);
        ui->horizontalFrame_6->setVisible(true);

        setMinimumHeight(426);
        setMaximumHeight(QWIDGETSIZE_MAX);
        if (ssize > 5) {
            ui->textEdit->setVisible(true);
        }
        saveMiniWindowGeometry();
        setGeometry(g_savedGeometry);
        setWindowOpacity(1.0);
        setWindowFlags(Qt::WindowSystemMenuHint);
        ui->actionToggleShade->setDisabled(true);
        QMainWindow::show();
        g_miniMode = false;
        g_shadeMode = false;
    } else {
        g_savedGeometry = geometry();
        ui->horizontalFrame_1->setFixedHeight(112);
        ui->buttonMountDisk_1->setFixedHeight(96);
        ui->buttonMountDisk_1->setFixedWidth(96);
        ui->buttonMountFolder_1->setFixedHeight(96);
        ui->buttonMountFolder_1->setFixedWidth(96);
        ui->buttonSave_1->setFixedHeight(96);
        ui->buttonSave_1->setFixedWidth(96);
        ui->autoSave_1->setFixedHeight(96);
        ui->autoSave_1->setFixedWidth(96);
        ui->buttonEditDisk_1->setFixedHeight(96);
        ui->buttonEditDisk_1->setFixedWidth(96);
        ui->buttonEject_1->setFixedHeight(96);
        ui->buttonEject_1->setFixedWidth(96);
        ui->horizontalFrame_2->setVisible(false);
        ui->horizontalFrame_3->setVisible(false);
        ui->horizontalFrame_4->setVisible(false);
        ui->horizontalFrame_5->setVisible(false);
        ui->horizontalFrame_6->setVisible(false);
        ui->textEdit->setVisible(false);
        setMinimumWidth(1000);
        setMinimumHeight(200);
//        setMaximumHeight(100);
        setGeometry(aspeqtSettings->lastMiniHorizontalPos(), aspeqtSettings->lastMiniVerticalPos(),
                    minimumWidth(), minimumHeight());
        ui->actionHideShowDrives->setDisabled(true);
        ui->actionToggleShade->setEnabled(true);
        if (aspeqtSettings->enableShade()) {
            setWindowOpacity(0.25);
            setWindowFlags(Qt::FramelessWindowHint);
            g_shadeMode = true;
        } else {
            g_shadeMode = false;
        }
        QMainWindow::show();
        g_miniMode = true;
    }
}

// Toggle printer Emulation ON/OFF //
void MainWindow::on_actionPrinterEmulation_triggered()
{
    if (g_printerEmu) {
        ui->actionPrinterEmulation->setText(QApplication::translate("MainWindow", "Start printer emulation", 0));
        ui->actionPrinterEmulation->setStatusTip(QApplication::translate("MainWindow", "Start printer emulation", 0));
        ui->actionPrinterEmulation->setIcon(QIcon(":/icons/tango-icons/status/printer-error.svg").pixmap(btnsize-5, btnsize-5, QIcon::Normal, QIcon::On));
        prtOnOffLabel->setPixmap(QIcon(":/icons/tango-icons/status/printer-error.svg").pixmap(sbIcon, sbIcon, QIcon::Normal, QIcon::On));
        prtOnOffLabel->setToolTip(tr("Start printer emulation"));
        prtOnOffLabel->setStatusTip(prtOnOffLabel->toolTip());
        g_printerEmu = false;
        qWarning() << "!i" << tr("Printer emulation stopped.");
    } else {
        ui->actionPrinterEmulation->setText(QApplication::translate("MainWindow", "Stop printer emulation", 0));
        ui->actionPrinterEmulation->setStatusTip(QApplication::translate("MainWindow", "Stop printer emulation", 0));
        ui->actionPrinterEmulation->setIcon(QIcon(":/icons/tango-icons/devices/printer.svg").pixmap(btnsize-5, btnsize-5, QIcon::Normal, QIcon::On));
        prtOnOffLabel->setPixmap(QIcon(":/icons/tango-icons/devices/printer.svg").pixmap(sbIcon, sbIcon, QIcon::Normal, QIcon::On));
        prtOnOffLabel->setToolTip(tr("Stop printer emulation"));
        prtOnOffLabel->setStatusTip(prtOnOffLabel->toolTip());
        g_printerEmu = true;
        qWarning() << "!i" << tr("Printer emulation started.");
    }
}

void MainWindow::on_actionStartEmulation_triggered()
{
    if (ui->actionStartEmulation->isChecked()) {
        sio->start(QThread::TimeCriticalPriority);
    } else {
        sio->setPriority(QThread::NormalPriority);
        sio->wait();
        qApp->processEvents();
    }
}

void MainWindow::sioStarted()
{
    ui->actionStartEmulation->setText(tr("&Stop emulation"));
    ui->actionStartEmulation->setToolTip(tr("Stop SIO peripheral emulation"));
    ui->actionStartEmulation->setStatusTip(tr("Stop SIO peripheral emulation"));
    onOffLabel->setPixmap(QIcon(":/icons/tango-icons/actions/media-playback-stop.svg").pixmap(sbIcon, sbIcon, QIcon::Normal, QIcon::On));
    onOffLabel->setToolTip(ui->actionStartEmulation->toolTip());
    onOffLabel->setStatusTip(ui->actionStartEmulation->statusTip());
    m_emulationRunning = true;
#ifdef ASPEQT_QML
    emit qmlChanged();
#endif
}

void MainWindow::sioFinished()
{
    ui->actionStartEmulation->setText(tr("&Start emulation"));
    ui->actionStartEmulation->setToolTip(tr("Start SIO peripheral emulation"));
    ui->actionStartEmulation->setStatusTip(tr("Start SIO peripheral emulation"));
    ui->actionStartEmulation->setChecked(false);
    onOffLabel->setPixmap(QIcon(":/icons/tango-icons/actions/media-playback-start.svg").pixmap(sbIcon, sbIcon, QIcon::Normal, QIcon::On));
    onOffLabel->setToolTip(ui->actionStartEmulation->toolTip());
    onOffLabel->setStatusTip(ui->actionStartEmulation->statusTip());
    speedLabel->hide();
    speedLabel->clear();
    m_emulationRunning = false;
    qWarning() << "!i" << tr("Emulation stopped.");
#ifdef ASPEQT_QML
    emit qmlChanged();
#endif
}

void MainWindow::sioStatusChanged(QString status)
{
    speedLabel->setText(status);
    speedLabel->show();
#ifdef ASPEQT_QML
    emit qmlChanged();
#endif
}

void MainWindow::deviceStatusChanged(int deviceNo)
{
    if (deviceNo >= 0x31 && deviceNo <= 0x31 + MAX_DISKS - 1) {
        int no = deviceNo - 0x31;
        if (m_slotPresent[no]) {
            SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(deviceNo));
            if (img) {
                // Auto-commit: write the image out as soon as it is modified.
                if (img->isModified() && m_autoCommit[no]) {
                    if (!img->save()) {
                        if (QMessageBox::question(this, tr("Save failed"),
                                tr("'%1' cannot be saved, do you want to save the image with another name?")
                                .arg(img->originalFileName()),
                                QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
                            saveDiskAs(no);
                        }
                    }
                }
            } else {
                m_autoCommit[no] = false;
                m_writeProtect[no] = false;
            }
        }
    }
#ifdef ASPEQT_QML
    emit qmlChanged();
#endif
}

void MainWindow::uiMessage(int t, QString message)
{
    if (message.at(0) == '"') {
        message.remove(0, 1);
    }
    if (message.at(message.count() - 1) == '"') {
        message.resize(message.count() - 1);
    }

    if (message == lastMessage) {
        lastMessageRepeat++;
        message = QString("%1 [x%2]").arg(message).arg(lastMessageRepeat);
        ui->textEdit->moveCursor(QTextCursor::End);
        QTextCursor cursor = ui->textEdit->textCursor();
        cursor.select(QTextCursor::BlockUnderCursor);
        cursor.removeSelectedText();
    } else {
        lastMessage = message;
        lastMessageRepeat = 1;
    }

    ui->statusBar->showMessage(message, 3000);

    switch (t) {
        case 'd':
            message = QString("<span style='color:green'>%1</span>").arg(message);
            break;
        case 'u':
            message = QString("<span style='color:gray'>%1</span>").arg(message);
            break;
        case 'n':
            message = QString("<span style='color:black'>%1</span>").arg(message);
            break;
        case 'i':
            message = QString("<span style='color:blue'>%1</span>").arg(message);
            break;
        case 'w':
            message = QString("<span style='color:brown'>%1</span>").arg(message);
            break;
        case 'e':
            message = QString("<span style='color:red'>%1</span>").arg(message);
            break;
        default:
            message = QString("<span style='color:purple'>%1</span>").arg(message);
            break;
    }

    ui->textEdit->append(message);
    ui->textEdit->verticalScrollBar()->setSliderPosition(ui->textEdit->verticalScrollBar()->maximum());
    ui->textEdit->horizontalScrollBar()->setSliderPosition(ui->textEdit->horizontalScrollBar()->minimum());
    logChanged(message);
}


void MainWindow::changeFonts()
{
    if (aspeqtSettings->useLargeFont()) {
        QFont font("Arial Black", 16, QFont::Normal);
        ui->labelFileName_1->setFont(font);
        ui->labelFileName_2->setFont(font);
        ui->labelFileName_3->setFont(font);
        ui->labelFileName_4->setFont(font);
        ui->labelFileName_5->setFont(font);
        ui->labelFileName_6->setFont(font);
        font = QFont("Arial Black", 14, QFont::Normal);
        ui->labelImageProperties_1->setFont(font);
        ui->labelImageProperties_2->setFont(font);
        ui->labelImageProperties_3->setFont(font);
        ui->labelImageProperties_4->setFont(font);
        ui->labelImageProperties_5->setFont(font);
        ui->labelImageProperties_6->setFont(font);
} else {
        QFont font("MS Shell Dlg 2,10", 10, QFont::Normal);
        ui->labelFileName_1->setFont(font);
        ui->labelFileName_2->setFont(font);
        ui->labelFileName_3->setFont(font);
        ui->labelFileName_4->setFont(font);
        ui->labelFileName_5->setFont(font);
        ui->labelFileName_6->setFont(font);
    }
}

//

// Restart emulation and re-translate following a session load //
void MainWindow::setSession()
{
    bool restart;
    restart = ui->actionStartEmulation->isChecked();
    if (restart) {
        ui->actionStartEmulation->trigger();
       sio->wait();
        qApp->processEvents();
    }

    // load translators and retranslate
    loadTranslators();
    ui->retranslateUi(this);
    for (int i = 0; i < MAX_DISKS; i++) {
        deviceStatusChanged(0x31 + i);
    }

    ui->actionStartEmulation->trigger();
}
void MainWindow::updateRecentFileActions()
{
    ui->actionMountRecent_0->setText(aspeqtSettings->recentImageSetting(0).fileName);
    ui->actionMountRecent_1->setText(aspeqtSettings->recentImageSetting(1).fileName);
    ui->actionMountRecent_2->setText(aspeqtSettings->recentImageSetting(2).fileName);
    ui->actionMountRecent_3->setText(aspeqtSettings->recentImageSetting(3).fileName);
    ui->actionMountRecent_4->setText(aspeqtSettings->recentImageSetting(4).fileName);
    ui->actionMountRecent_5->setText(aspeqtSettings->recentImageSetting(5).fileName);
    ui->actionMountRecent_6->setText(aspeqtSettings->recentImageSetting(6).fileName);
    ui->actionMountRecent_7->setText(aspeqtSettings->recentImageSetting(7).fileName);
    ui->actionMountRecent_8->setText(aspeqtSettings->recentImageSetting(8).fileName);
    ui->actionMountRecent_9->setText(aspeqtSettings->recentImageSetting(9).fileName);

    ui->actionMountRecent_0->setVisible(!ui->actionMountRecent_0->text().isEmpty());
    ui->actionMountRecent_1->setVisible(!ui->actionMountRecent_1->text().isEmpty());
    ui->actionMountRecent_2->setVisible(!ui->actionMountRecent_2->text().isEmpty());
    ui->actionMountRecent_3->setVisible(!ui->actionMountRecent_3->text().isEmpty());
    ui->actionMountRecent_4->setVisible(!ui->actionMountRecent_4->text().isEmpty());
    ui->actionMountRecent_5->setVisible(!ui->actionMountRecent_5->text().isEmpty());
    ui->actionMountRecent_6->setVisible(!ui->actionMountRecent_6->text().isEmpty());
    ui->actionMountRecent_7->setVisible(!ui->actionMountRecent_7->text().isEmpty());
    ui->actionMountRecent_8->setVisible(!ui->actionMountRecent_8->text().isEmpty());
    ui->actionMountRecent_9->setVisible(!ui->actionMountRecent_9->text().isEmpty());
}


bool MainWindow::ejectImage(int no, bool ask)
{
    SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + 0x31));

    if (ask && img && img->isModified()) {
        QMessageBox::StandardButton answer;
        answer = saveImageWhenClosing(no, QMessageBox::No, 0);
        if (answer == QMessageBox::Cancel) {
            return false;
        }
    }

    sio->uninstallDevice(no + 0x31);
    if (!img) {
        return true;
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
    updateRecentFileActions();
    deviceStatusChanged(no + 0x31);
    qDebug() << "!n" << tr("Unmounted disk %1").arg(no + 1);
    return true;
}

int MainWindow::firstEmptyDiskSlot(int startFrom, bool createOne)
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

void MainWindow::mountFileWithDefaultProtection(int no, const QString &fileName)
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

void MainWindow::mountFile(int no, const QString &fileName, bool /*prot*/)
{
    SimpleDiskImage *disk;
    bool isDir = false;

    if (fileName.isEmpty()) {
        if(g_aspeclFileName.left(1) == "*") emit fileMounted(false);  //
        return;
    }

    FileTypes::FileType type = FileTypes::getFileType(fileName);

#ifdef ASPEQT_QML
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
#endif

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
        if (!ejectImage(no)) {
            aspeqtSettings->unmountImage(no);
            delete disk;
            if(g_aspeclFileName.left(1) == "*") emit fileMounted(false);  //
            return;
        }

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
        updateRecentFileActions();
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

QString MainWindow::androidSaveUrl(const QString &caption, const QString &filter)
{
    QUrl u = QFileDialog::getSaveFileUrl(this, caption, QUrl(), filter);
    return u.isEmpty() ? QString() : androidContentUri(u);
}

QString MainWindow::androidDisplayName(const QString &uri)
{
    QJniObject juri = QJniObject::fromString(uri);
    QJniObject res = QJniObject::callStaticObjectMethod(
        "net/greblus/SerialActivity", "displayName",
        "(Ljava/lang/String;)Ljava/lang/String;", juri.object<jstring>());
    return res.isValid() ? res.toString() : QString();
}

void MainWindow::androidTakePersistable(const QString &uri, bool write)
{
    QJniObject juri = QJniObject::fromString(uri);
    QJniObject::callStaticMethod<void>(
        "net/greblus/SerialActivity", "takePersistable",
        "(Ljava/lang/String;Z)V", juri.object<jstring>(), (jboolean)write);
}

QString MainWindow::androidTreeName(const QString &tree)
{
    QJniObject jt = QJniObject::fromString(tree);
    QJniObject res = QJniObject::callStaticObjectMethod(
        "net/greblus/SerialActivity", "treeDisplayName",
        "(Ljava/lang/String;)Ljava/lang/String;", jt.object<jstring>());
    return res.isValid() ? res.toString() : QString();
}

int MainWindow::androidCopyTreeToDir(const QString &tree, const QString &dest)
{
    QJniObject jt = QJniObject::fromString(tree);
    QJniObject jd = QJniObject::fromString(dest);
    return QJniObject::callStaticMethod<jint>(
        "net/greblus/SerialActivity", "copyTreeToDir",
        "(Ljava/lang/String;Ljava/lang/String;)I", jt.object<jstring>(), jd.object<jstring>());
}

int MainWindow::androidCopyDirToTree(const QString &src, const QString &tree)
{
    QJniObject js = QJniObject::fromString(src);
    QJniObject jt = QJniObject::fromString(tree);
    return QJniObject::callStaticMethod<jint>(
        "net/greblus/SerialActivity", "copyDirToTree",
        "(Ljava/lang/String;Ljava/lang/String;)I", js.object<jstring>(), jt.object<jstring>());
}

int MainWindow::androidCopyUriToFile(const QString &uri, const QString &dest)
{
    QJniObject ju = QJniObject::fromString(uri);
    QJniObject jd = QJniObject::fromString(dest);
    return QJniObject::callStaticMethod<jint>(
        "net/greblus/SerialActivity", "copyUriToFile",
        "(Ljava/lang/String;Ljava/lang/String;)I", ju.object<jstring>(), jd.object<jstring>());
}

QString MainWindow::androidReadablePath(const QString &uri, int slot)
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

QString MainWindow::androidLocalCopy(const QString &uri)
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

QString MainWindow::androidChildOrCreate(const QString &tree, const QString &name)
{
    QJniObject jt = QJniObject::fromString(tree);
    QJniObject jn = QJniObject::fromString(name);
    QJniObject r = QJniObject::callStaticObjectMethod(
        "net/greblus/SerialActivity", "ensureInTree",
        "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;",
        jt.object<jstring>(), jn.object<jstring>());
    return r.isValid() ? r.toString() : QString();
}

void MainWindow::androidInstallDos(int no)
{
    QString tree = m_folderTree.value(no);
    if (tree.isEmpty()) {
        QMessageBox::warning(this, tr("Install DOS"), tr("This slot does not hold a mounted folder."));
        return;
    }
    if (QMessageBox::question(this, tr("Install DOS"),
            tr("Copy high-speed MyPicoDOS ($boot.bin + picodos.sys) into this folder? "
               "The Atari will then be able to boot DOS from it."),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;

    // Write the two bundled files straight into the SAF folder (find-or-create
    // the document, then stream the resource into it via a descriptor).
    auto put = [&](const QString &res, const QString &dstName) -> bool {
        QString childUri = androidChildOrCreate(tree, dstName);
        if (childUri.isEmpty())
            return false;
        QFile src(res);
        ContentFile dst(childUri);
        if (!src.open(QIODevice::ReadOnly) || !dst.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return false;
        return dst.write(src.readAll()) >= 0;
    };

    bool ok = put(":/dos/boot.bin", "$boot.bin") && put(":/dos/picodos.sys", "picodos.sys");
    if (ok) {
        deviceStatusChanged(no + 0x31);   // re-read the folder
        qDebug() << "!i" << tr("Installed high-speed MyPicoDOS into the folder. "
                               "Reboot your Atari to load DOS.");
    } else {
        QMessageBox::warning(this, tr("Install DOS"), tr("Could not copy the DOS files into the folder."));
    }
}

#endif

QString MainWindow::friendlyName(const QString &name)
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



void MainWindow::toggleWriteProtection(int no)
{
    SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + 0x31));
    if (!img) return;
    m_writeProtect[no] = !m_writeProtect[no];
    img->setReadOnly(m_writeProtect[no]);
    aspeqtSettings->setMountedImageSetting(no, img->originalFileName(), m_writeProtect[no]);
#ifdef ASPEQT_QML
    emit qmlChanged();
#endif
}


QMessageBox::StandardButton MainWindow::saveImageWhenClosing(int no, QMessageBox::StandardButton previousAnswer, int number)
{
    SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + 0x31));

    if (previousAnswer != QMessageBox::YesToAll) {
        QMessageBox::StandardButtons buttons;
        if (number) {
            buttons = QMessageBox::Yes | QMessageBox::No | QMessageBox::YesToAll | QMessageBox::NoToAll | QMessageBox::Cancel;
        } else {
            buttons = QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel;
        }
        previousAnswer = QMessageBox::question(this, tr("Image file unsaved"), tr("'%1' has unsaved changes, do you want to save it?")
                                       .arg(img->originalFileName()), buttons);
    }
    if (previousAnswer == QMessageBox::Yes || previousAnswer == QMessageBox::YesToAll) {
        saveDisk(no);
    }
    if (previousAnswer == QMessageBox::Close) {
        previousAnswer = QMessageBox::Cancel;
    }
    return previousAnswer;
}

void MainWindow::loadTranslators()
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

void MainWindow::saveDisk(int no)
{
#ifdef Q_OS_ANDROID
    // For a mounted folder the "save" button installs high-speed DOS instead.
    if (qobject_cast<FolderImage *>(sio->getDevice(no + 0x31))) {
        androidInstallDos(no);
        return;
    }
#endif
    SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + 0x31));

    if (img->isUnnamed()) {
        saveDiskAs(no);
        return;
    }

    bool saved;

    img->lock();
    saved = img-> save();
    img->unlock();
    if (!saved) {
        if (QMessageBox::question(this, tr("Save failed"), tr("'%1' cannot be saved, do you want to save the image with another name?")
            .arg(img->originalFileName()), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
            saveDiskAs(no);
        }
    }
}
//
void MainWindow::autoCommit(int no)
{
    if (no < 0 || no >= MAX_DISKS) return;
    if (sio->getDevice(no + 0x31)) autoSaveDisk(no);
}

void MainWindow::autoSaveDisk(int no)
{
    SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + 0x31));
    if (!img) return;

    // Auto-commit is engine state now (it used to live in the slot widget's
    // checkable action). Toggling it also commits pending changes, as before.
    m_autoCommit[no] = !m_autoCommit[no];
    qDebug() << "!n" << (m_autoCommit[no] ? tr("[Disk %1] Auto-commit ON.").arg(no + 1)
                                          : tr("[Disk %1] Auto-commit OFF.").arg(no + 1));

    if (img->isUnnamed()) {
        saveDiskAs(no);
        return;
    }

    img->lock();
    bool saved = img->save();
    img->unlock();
    if (!saved) {
        if (QMessageBox::question(this, tr("Save failed"), tr("'%1' cannot be saved, do you want to save the image with another name?")
            .arg(img->originalFileName()), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
            saveDiskAs(no);
        }
    }
#ifdef ASPEQT_QML
    emit qmlChanged();
#endif
}
//
void MainWindow::saveDiskAs(int no)
{
    SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + 0x31));
    QString dir, fileName;
    bool saved = false;

    if (img->isUnnamed()) {
        dir = aspeqtSettings->lastDiskImageDir();
    } else {
        dir = QFileInfo(img->originalFileName()).absolutePath();
    }

    do {
        #ifdef Q_OS_ANDROID
            // SAF create-document: the content:// URI has no extension, so pick
            // the format from the display name the user typed (default ATR).
            fileName = androidSaveUrl(tr("Save image as"),
                                      tr("ATR image (*.atr);;XFD image (*.xfd);;All files (*)"));
            if (fileName.isEmpty()) {
                return;
            }
            QString dn = androidDisplayName(fileName);
            FileTypes::FileType st = FileTypes::Atr;
            if (dn.endsWith(".xfd", Qt::CaseInsensitive)) st = FileTypes::Xfd;
            else if (dn.endsWith(".dcm", Qt::CaseInsensitive)) st = FileTypes::Dcm;
            else if (dn.endsWith(".scp", Qt::CaseInsensitive)) st = FileTypes::Scp;
            else if (dn.endsWith(".di", Qt::CaseInsensitive))  st = FileTypes::Di;
            img->lock();
            saved = img->saveAs(fileName, st);
            img->unlock();
        #else
        fileName = QFileDialog::getSaveFileName(this, tr("Save image as"),
                                 dir,
                                 tr(
//                                                    "All Atari disk images (*.atr *.xfd *.atx *.pro);;"
                                                    "All Atari disk images (*.atr *.xfd *.pro);;"
                                                    "SIO2PC ATR images (*.atr);;"
                                                    "XFormer XFD images (*.xfd);;"
//                                                    "ATX images (*.atx);;"
                                                    "Pro images (*.pro);;"
                                                    "All files (*)"));
        if (fileName.isEmpty()) {
            return;
        }

        img->lock();
        saved = img->saveAs(fileName);
        img->unlock();
        #endif

        if (!saved) {
            if (QMessageBox::question(this, tr("Save failed"), tr("'%1' cannot be saved, do you want to save the image with another name?")
                .arg(fileName), QMessageBox::Yes, QMessageBox::No) == QMessageBox::No) {
                break;
            }
        }

    } while (!saved);

    if (saved) {
        #ifndef Q_OS_ANDROID
        aspeqtSettings->setLastDiskImageDir(QFileInfo(fileName).absolutePath());
        #endif
    }
    aspeqtSettings->unmountImage(no);
    aspeqtSettings->mountImage(no, fileName, img->isReadOnly());
}

void MainWindow::revertDisk(int no)
{
    SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + 0x31));
    if (QMessageBox::question(this, tr("Revert to last saved"),
            tr("Do you really want to revert '%1' to its last saved state? You will lose the changes that has been made.")
            .arg(img->originalFileName()), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
        img->lock();
        img->reopen();
        img->unlock();
        deviceStatusChanged(0x31 + no);
    }
}



void MainWindow::on_actionEject_1_triggered() {ejectImage(0);}
void MainWindow::on_actionEject_2_triggered() {ejectImage(1);}
void MainWindow::on_actionEject_3_triggered() {ejectImage(2);}
void MainWindow::on_actionEject_4_triggered() {ejectImage(3);}
void MainWindow::on_actionEject_5_triggered() {ejectImage(4);}
void MainWindow::on_actionEject_6_triggered() {ejectImage(5);}

void MainWindow::on_actionWriteProtect_1_triggered() {toggleWriteProtection(0);}
void MainWindow::on_actionWriteProtect_2_triggered() {toggleWriteProtection(1);}
void MainWindow::on_actionWriteProtect_3_triggered() {toggleWriteProtection(2);}
void MainWindow::on_actionWriteProtect_4_triggered() {toggleWriteProtection(3);}
void MainWindow::on_actionWriteProtect_5_triggered() {toggleWriteProtection(4);}
void MainWindow::on_actionWriteProtect_6_triggered() {toggleWriteProtection(5);}

void MainWindow::on_actionMountRecent_0_triggered() {mountFileWithDefaultProtection(firstEmptyDiskSlot(), ui->actionMountRecent_0->text());}
void MainWindow::on_actionMountRecent_1_triggered() {mountFileWithDefaultProtection(firstEmptyDiskSlot(), ui->actionMountRecent_1->text());}
void MainWindow::on_actionMountRecent_2_triggered() {mountFileWithDefaultProtection(firstEmptyDiskSlot(), ui->actionMountRecent_2->text());}
void MainWindow::on_actionMountRecent_3_triggered() {mountFileWithDefaultProtection(firstEmptyDiskSlot(), ui->actionMountRecent_3->text());}
void MainWindow::on_actionMountRecent_4_triggered() {mountFileWithDefaultProtection(firstEmptyDiskSlot(), ui->actionMountRecent_4->text());}
void MainWindow::on_actionMountRecent_5_triggered() {mountFileWithDefaultProtection(firstEmptyDiskSlot(), ui->actionMountRecent_5->text());}
void MainWindow::on_actionMountRecent_6_triggered() {mountFileWithDefaultProtection(firstEmptyDiskSlot(), ui->actionMountRecent_6->text());}
void MainWindow::on_actionMountRecent_7_triggered() {mountFileWithDefaultProtection(firstEmptyDiskSlot(), ui->actionMountRecent_7->text());}
void MainWindow::on_actionMountRecent_8_triggered() {mountFileWithDefaultProtection(firstEmptyDiskSlot(), ui->actionMountRecent_8->text());}
void MainWindow::on_actionMountRecent_9_triggered() {mountFileWithDefaultProtection(firstEmptyDiskSlot(), ui->actionMountRecent_9->text());}




void MainWindow::qmlOpenSessionPath(const QString &url)
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
            QFile in(picked);
            if (!in.open(QIODevice::ReadOnly)) {
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
    qmlEjectAll();

// Pass Session file name, path and MainWindow title to AspeQtSettings //
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

    setWindowTitle(g_mainWindowTitle + tr(" -- Session: ") + g_sessionFile);
    setGeometry(aspeqtSettings->lastHorizontalPos(), aspeqtSettings->lastVerticalPos(), aspeqtSettings->lastWidth() , aspeqtSettings->lastHeight());

#ifdef Q_OS_ANDROID
    // Rebuild the slot column (count + gaps) to match the loaded session before
    // restoring its mounts.
    androidRebuildSlots();
#endif

    for (int i = 0; i < m_numDisks; i++) {  //
        AspeqtSettings::ImageSettings is;
        is = aspeqtSettings->mountedImageSetting(i);
        mountFile(i, is.fileName, is.isWriteProtected);
    }

    setSession();
}
void MainWindow::qmlSaveSessionPath(const QString &url)
{
    const QString picked = pathFromPickedUrl(url);
    if (picked.isEmpty()) {
        return;
    }

// Save mainwindow position and size to session file //
    if (aspeqtSettings->saveWindowsPos()) {
        aspeqtSettings->setLastHorizontalPos(geometry().x());
        aspeqtSettings->setLastVerticalPos(geometry().y());
        aspeqtSettings->setLastWidth(geometry().width());
        aspeqtSettings->setLastHeight(geometry().height());
    }

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
    QFile in(tmpPath), out(picked);
    if (in.open(QIODevice::ReadOnly) && out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        out.write(in.readAll());
    }
}





void MainWindow::on_actionQuit_triggered()
{
    close();
}

void MainWindow::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick) {
        setWindowFlags(oldWindowFlags);
        setWindowState(oldWindowStates);
        show();
        activateWindow();
        raise();
        trayIcon.hide();
    }
}



#ifdef ASPEQT_QML
// ===========================================================================
// QML bridge (branch `qml`): expose the engine's state as QVariant and route
// QML button presses to the existing widget-era slots. See qmlbridge.{h,cpp}.
// ===========================================================================
QVariantList MainWindow::qmlDriveList()
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
            m["editOpen"]       = img->editDialog() != nullptr;
            m["autoCommit"]     = m_autoCommit[i];
            m["writeProtected"] = m_writeProtect[i];
        } else {
            m["mounted"]        = false;
            m["isFolder"]       = false;
            m["fileName"]       = QString();
            m["typeText"]       = QString();
            m["modified"]       = false;
            m["editOpen"]       = false;
            m["autoCommit"]     = false;
            m["writeProtected"] = false;
        }
        out << m;
    }
    return out;
}

QVariantMap MainWindow::qmlLoaderState()
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

QVariantMap MainWindow::qmlStatus()
{
    extern bool g_printerEmu;
    QVariantMap m;
    m["running"]   = m_emulationRunning;
    m["speed"]     = speedLabel ? speedLabel->text() : QString();
    m["printerOn"] = g_printerEmu;
    return m;
}

bool MainWindow::qmlCanAddSlot()
{
    for (int i = 0; i < MAX_DISKS; ++i)
        if (!m_slotPresent[i]) return true;
    return false;
}

void MainWindow::qmlEjectPressed(int i)       { androidEjectPressed(i); }
void MainWindow::qmlSave(int i)               { saveDisk(i); }
void MainWindow::qmlToggleAutoCommit(int i)   { autoSaveDisk(i); }
void MainWindow::qmlToggleWriteProtect(int i) { toggleWriteProtection(i); }
int MainWindow::qmlAddSlot()
{
    // androidAddSlot() fills the lowest empty index; return it so the QML side
    // can scroll to and flash the new slot.
    int i = -1;
    for (int k = 0; k < MAX_DISKS; ++k)
        if (!m_slotPresent[k]) { i = k; break; }
    if (i < 0) return -1;
    androidAddSlot();
    return i;
}

// Swap two drives (drag-reorder). Same effect as the widget UI's drop handler:
// device numbers stay put, the mounted images/links exchange places.
void MainWindow::qmlSwapSlots(int source, int slot)
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
    emit qmlChanged();
}
void MainWindow::qmlLoaderPlay()              { loaderPlayCas(); }
void MainWindow::qmlLoaderRetry()             { loaderRetry(); }
void MainWindow::qmlLoaderEject()             { loaderEject(); }
void MainWindow::qmlToggleSio()               { ui->actionStartEmulation->trigger(); }
void MainWindow::qmlTogglePrinter()           { ui->actionPrinterEmulation->trigger(); emit qmlChanged(); }
void MainWindow::qmlClearLog()
{
    ui->textEdit->clear();
    emit sendLogText(QString());
    emit qmlChanged();
}


// Create + format + mount a new disk image (port of on_actionNewImage_triggered
// without the widget dialog; geometry chosen in the QML CreateDiskDialog).
void MainWindow::qmlCreateDisk(int sectorCount, int sectorSize)
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
    if (!ejectImage(no)) { delete disk; return; }
    sio->installDevice(0x31 + no, disk);
    deviceStatusChanged(0x31 + no);
    qDebug() << "!n" << tr("[%1] Mounted '%2' as '%3'.")
            .arg(disk->deviceName())
            .arg(friendlyName(disk->originalFileName()))
            .arg(disk->description());
    emit qmlChanged();
}
void MainWindow::qmlEjectAll()
{
    QMessageBox::StandardButton answer = QMessageBox::No;

    int toBeSaved = 0;

    for (int i = 0; i < m_numDisks; i++) {
        SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(i + 0x31));
        if (img && img->isModified()) {
            toBeSaved++;
        }
    }

    if (!toBeSaved) {
        for (int i = m_numDisks-1; i >= 0; i--) {
            ejectImage(i);
        }
        return;
    }

    bool wasRunning = ui->actionStartEmulation->isChecked();
    if (wasRunning) {
        ui->actionStartEmulation->trigger();
    }

    for (int i = m_numDisks-1; i >= 0; i--) {
        SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(i + 0x31));
        if (img && img->isModified()) {
            toBeSaved--;
            answer = saveImageWhenClosing(i, answer, toBeSaved);
            if (answer == QMessageBox::NoToAll) {
                break;
            }
            if (answer == QMessageBox::Cancel) {
                if (wasRunning) {
                    ui->actionStartEmulation->trigger();
                }
                return;
            }
        }
    }
    for (int i = m_numDisks-1; i >= 0; i--) {
        ejectImage(i, false);
    }
    if (wasRunning) {
        ui->actionStartEmulation->trigger();
    }
}
QString MainWindow::qmlPrinterText()   { return textPrinterWindow ? textPrinterWindow->qmlText() : QString(); }
QString MainWindow::qmlPrinterTextAtascii() { return textPrinterWindow ? textPrinterWindow->qmlTextAtascii() : QString(); }
void MainWindow::qmlPrinterClear()     { if (textPrinterWindow) textPrinterWindow->qmlClear(); emit qmlPrinterTextChanged(); }
void MainWindow::qmlPrinterSave()      { if (textPrinterWindow) textPrinterWindow->qmlSave(); }
void MainWindow::qmlQuit()             { close(); qApp->quit(); }

QStringList MainWindow::qmlRecentFiles()
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

QVariantMap MainWindow::qmlLoadOptions()
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

void MainWindow::qmlApplyOptions(const QVariantMap &o)
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
    emit qmlChanged();
}

// Available UI languages: Automatic + English + every bundled aspeqt_*.qm
// (native name = that translation's rendering of "English").
QVariantList MainWindow::qmlLanguages()
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

bool MainWindow::qmlDiskOpen(int hwIndex)
{
    qmlDiskClose();
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

bool MainWindow::qmlDiskReadOnly()
{
    // Folder images are read-only virtual disks (writeSector is a no-op).
    return !m_dvDisk || qobject_cast<FolderImage *>(m_dvDisk) != nullptr;
}

int MainWindow::qmlDiskFsType() { return m_dvFsType; }


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

void MainWindow::pickDocument(int reqId, const QString &mimeType)
{
#ifdef Q_OS_ANDROID
    QJniObject::callStaticMethod<void>("net/greblus/SerialActivity", "pickDocument",
        "(ILjava/lang/String;)V", (jint)reqId,
        QJniObject::fromString(mimeType).object<jstring>());
#else
    Q_UNUSED(reqId) Q_UNUSED(mimeType)
#endif
}

void MainWindow::createDocument(int reqId, const QString &mimeType, const QString &suggestedName)
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

void MainWindow::pickFolder(int reqId)
{
#ifdef Q_OS_ANDROID
    QJniObject::callStaticMethod<void>("net/greblus/SerialActivity", "pickFolder",
        "(I)V", (jint)reqId);
#else
    Q_UNUSED(reqId)
#endif
}

void MainWindow::documentPicked(int reqId, const QString &uri)
{
    emit qmlDocumentPicked(reqId, uri);
}

QString MainWindow::qmlStartDir(const QString &kind)
{
    if (kind == QLatin1String("disk"))    return aspeqtSettings->lastDiskImageDir();
    if (kind == QLatin1String("folder"))  return aspeqtSettings->lastFolderImageDir();
    if (kind == QLatin1String("exe"))     return aspeqtSettings->lastExeDir();
    if (kind == QLatin1String("session")) return aspeqtSettings->lastSessionDir();
    return QString();
}

void MainWindow::qmlMountDiskPath(int no, const QString &url)
{
    const QString fileName = pathFromPickedUrl(url);
    if (fileName.isEmpty())
        return;
    if (!fileName.startsWith(QLatin1String("content:")))
        aspeqtSettings->setLastDiskImageDir(QFileInfo(fileName).absolutePath());
    mountFileWithDefaultProtection(no, fileName);
}

void MainWindow::qmlMountFolderPath(int no, const QString &url)
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

void MainWindow::qmlLoaderLoadPath(const QString &url)
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
        qmlToast(tr("Pick an Atari executable (.xex/.com/.exe) or a cassette image (.cas)."));
}

void MainWindow::qmlToast(const QString &text)
{
#ifdef Q_OS_ANDROID
    QJniObject::callStaticMethod<void>("net/greblus/SerialActivity", "showToast",
        "(Ljava/lang/String;)V", QJniObject::fromString(text).object<jstring>());
#else
    Q_UNUSED(text)
#endif
}

bool MainWindow::qmlDiskSetFsType(int index)
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

void MainWindow::qmlDiskClose()
{
    if (m_dvFs)   { delete m_dvFs; m_dvFs = nullptr; }
    if (m_dvDisk) { m_dvDisk->unlock(); m_dvDisk = nullptr; }
    m_dvDirs.clear();
    m_dvPaths.clear();
}

QVariantList MainWindow::qmlDiskEntries()
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

QString MainWindow::qmlDiskPath()
{
    if (!m_dvFs || !m_dvDisk) return QString();
    QString p = QString("D%1:").arg(m_dvDisk->deviceNo() - 0x30);
    for (const QString &s : m_dvPaths) p.append(s + ">");
    return p;
}

bool MainWindow::qmlDiskCanParent() { return !m_dvPaths.isEmpty(); }

void MainWindow::qmlDiskEnter(int row)
{
    if (!m_dvFs || m_dvDirs.isEmpty()) return;
    const QList<AtariDirEntry> entries = m_dvFs->getEntries(m_dvDirs.last());
    if (row < 0 || row >= entries.size()) return;
    const AtariDirEntry &e = entries.at(row);
    if (!(e.attributes & AtariDirEntry::Directory)) return;
    m_dvPaths.append(e.name());
    m_dvDirs.append(e.firstSector);
}

void MainWindow::qmlDiskParent()
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

void MainWindow::qmlDiskSetTextConversion(bool on)
{
    if (m_dvFs) m_dvFs->setTextConversion(on);
}

bool MainWindow::qmlDiskExtractPath(const QVariantList &rows, const QString &url)
{
    if (!m_dvFs || m_dvDirs.isEmpty() || rows.isEmpty()) return false;
    QList<AtariDirEntry> sel = dvPickRows(m_dvFs, m_dvDirs.last(), rows);
    if (sel.isEmpty()) return false;

    const QString target = pathFromPickedUrl(url);
    if (target.isEmpty()) return false;
    aspeqtSettings->setLastExtractDir(target);

    if (!target.startsWith(QLatin1String("content:"))) {
        if (!m_dvFs->extractRecursive(sel, target)) {
            qmlToast(tr("Cannot extract the files, see the log."));
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
        qmlToast(tr("Cannot extract the files, see the log."));
        return false;
    }
    if (androidCopyDirToTree(tmpDir, target) < 0) {
        qmlToast(tr("Cannot extract the files, see the log."));
        return false;
    }
    QDir(tmpDir).removeRecursively();
    return true;
}

bool MainWindow::qmlDiskDelete(const QVariantList &rows)
{
    if (!m_dvFs || m_dvDirs.isEmpty() || rows.isEmpty()) return false;
    // Same native confirmation dialog as the MyPicoDOS install prompt.
    if (QMessageBox::question(this, tr("Confirmation"),
            tr("Are you sure you want to delete selected files?"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return false;
    QList<AtariDirEntry> sel = dvPickRows(m_dvFs, m_dvDirs.last(), rows);
    if (sel.isEmpty()) return false;
    if (!m_dvFs->deleteRecursive(sel)) {
        qmlToast(tr("Cannot delete the files, see the log."));
        return false;
    }
    return true;
}

bool MainWindow::qmlDiskAddFilesPath(const QString &url)
{
    if (!m_dvFs || m_dvDirs.isEmpty()) return false;
    const QString picked = pathFromPickedUrl(url);
    if (picked.isEmpty()) return false;

    QStringList files;
    if (picked.startsWith(QLatin1String("content:"))) {
        // insertRecursive works on real files, so stage the document under its
        // display name (which is also the name it gets on the Atari disk).
        QFile src(picked);
        if (!src.open(QIODevice::ReadOnly)) return false;
        const QByteArray bytes = src.readAll();
        src.close();
        QString displayName = androidDisplayName(picked);
        if (displayName.isEmpty()) displayName = QStringLiteral("FILE");
        const QString tmpDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/addfile";
        QDir(tmpDir).removeRecursively();
        QDir().mkpath(tmpDir);
        const QString tmpPath = tmpDir + "/" + displayName;
        QFile dst(tmpPath);
        if (!dst.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
        dst.write(bytes);
        dst.close();
        files.append(tmpPath);
    } else {
        files.append(picked);
    }

    if (m_dvFs->insertRecursive(m_dvDirs.last(), files).isEmpty()) {
        qmlToast(tr("Cannot add the file, see the log."));
        return false;
    }
    return true;
}

void MainWindow::qmlMountRecent(int index)
{
    switch (index) {
    case 0: on_actionMountRecent_0_triggered(); break;
    case 1: on_actionMountRecent_1_triggered(); break;
    case 2: on_actionMountRecent_2_triggered(); break;
    case 3: on_actionMountRecent_3_triggered(); break;
    case 4: on_actionMountRecent_4_triggered(); break;
    case 5: on_actionMountRecent_5_triggered(); break;
    case 6: on_actionMountRecent_6_triggered(); break;
    case 7: on_actionMountRecent_7_triggered(); break;
    case 8: on_actionMountRecent_8_triggered(); break;
    case 9: on_actionMountRecent_9_triggered(); break;
    }
}
#endif // ASPEQT_QML
