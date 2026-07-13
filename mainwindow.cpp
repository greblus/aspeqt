#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "diskimage.h"
#include "diskimagepro.h"
#include "folderimage.h"
#include "pclink.h"
#include "miscdevices.h"
#include "aspeqtsettings.h"
#include "autobootdialog.h"
#include "autoboot.h"
#include "cassettedialog.h"
#include "bootoptionsdialog.h"
#include "logdisplaydialog.h"

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
    ui->textEdit->installEventFilter(mainWindow);
    changeFonts();

    /* Initialize diskWidgets array and tool button actions */

#ifdef Q_OS_ANDROID
    // Android uses dynamic, programmatically-built slots in a scroll area; the
    // .ui's fixed 6 frames are discarded. androidBuildSlots() sets m_numDisks
    // from the session, builds the scroll column and the first m_numDisks slots.
    m_numDisks = aspeqtSettings->numberOfDisks();
    if (m_numDisks < 1)          m_numDisks = DEFAULT_DISKS;
    if (m_numDisks > MAX_DISKS)  m_numDisks = MAX_DISKS;
    androidBuildSlots();
    // The top loader slot replaces the executable/cassette menu items.
    ui->actionBootExe->setVisible(false);
    ui->actionPlaybackCassette->setVisible(false);
#else
    m_numDisks = g_numberOfDisks;
    for (int i = 0; i < m_numDisks; i++) {      //

        diskWidgets[i].fileNameLabel = findChild <QLabel*> (QString("labelFileName_%1").arg(i + 1));
        diskWidgets[i].imagePropertiesLabel = findChild <QLabel*> (QString("labelImageProperties_%1").arg(i + 1));
        diskWidgets[i].ejectAction = findChild <QAction*> (QString("actionEject_%1").arg(i + 1));
        diskWidgets[i].writeProtectAction = findChild <QAction*> (QString("actionWriteProtect_%1").arg(i + 1));
        diskWidgets[i].editAction = findChild <QAction*> (QString("actionEditDisk_%1").arg(i + 1));
        diskWidgets[i].mountDiskAction = findChild <QAction*> (QString("actionMountDisk_%1").arg(i + 1));
        diskWidgets[i].mountFolderAction = findChild <QAction*> (QString("actionMountFolder_%1").arg(i + 1));
        diskWidgets[i].saveAction = findChild <QAction*> (QString("actionSave_%1").arg(i + 1));
        diskWidgets[i].autoSaveAction = findChild <QAction*> (QString("actionAutoSave_%1").arg(i + 1));  //
        if(i == 0)
            diskWidgets[i].bootOptionAction = findChild <QAction*> (QString("actionBootOption"));       //
        diskWidgets[i].revertAction = findChild <QAction*> (QString("actionRevert_%1").arg(i + 1));
        diskWidgets[i].saveAsAction = findChild <QAction*> (QString("actionSaveAs_%1").arg(i + 1));
        diskWidgets[i].frame = findChild <QFrame*> (QString("horizontalFrame_%1").arg(i + 1));

        if(i == 0)
            diskWidgets[i].frame->insertAction(0, diskWidgets[i].bootOptionAction); //
        diskWidgets[i].frame->insertAction(0, diskWidgets[i].saveAction);
        diskWidgets[i].frame->insertAction(0, diskWidgets[i].autoSaveAction);       //
        diskWidgets[i].frame->insertAction(0, diskWidgets[i].saveAsAction);
        diskWidgets[i].frame->insertAction(0, diskWidgets[i].revertAction);
        diskWidgets[i].frame->insertAction(0, diskWidgets[i].mountDiskAction);
        diskWidgets[i].frame->insertAction(0, diskWidgets[i].mountFolderAction);
        diskWidgets[i].frame->insertAction(0, diskWidgets[i].ejectAction);
        diskWidgets[i].frame->insertAction(0, diskWidgets[i].writeProtectAction);
        diskWidgets[i].frame->insertAction(0, diskWidgets[i].editAction);

        findChild <QToolButton*> (QString("buttonMountDisk_%1").arg(i + 1)) -> setDefaultAction(diskWidgets[i].mountDiskAction);
        findChild <QToolButton*> (QString("buttonMountFolder_%1").arg(i + 1)) -> setDefaultAction(diskWidgets[i].mountFolderAction);
        findChild <QToolButton*> (QString("buttonEject_%1").arg(i + 1)) -> setDefaultAction(diskWidgets[i].ejectAction);
        findChild <QToolButton*> (QString("buttonSave_%1").arg(i + 1)) -> setDefaultAction(diskWidgets[i].saveAction);
        findChild <QToolButton*> (QString("autoSave_%1").arg(i + 1)) -> setDefaultAction(diskWidgets[i].autoSaveAction);  //
        findChild <QToolButton*> (QString("buttonEditDisk_%1").arg(i + 1)) -> setDefaultAction(diskWidgets[i].editAction);
    }
#endif

    /* Connect SioWorker signals */
    sio = new SioWorker();
    connect(sio, SIGNAL(started()), this, SLOT(sioStarted()));
    connect(sio, SIGNAL(finished()), this, SLOT(sioFinished()));
    connect(sio, SIGNAL(statusChanged(QString)), this, SLOT(sioStatusChanged(QString)));
    shownFirstTime = true;

    PCLINK* pclink = new PCLINK(sio);
    sio->installDevice(0x6F, pclink);

    /* Restore application state */
    for (int i = 0; i < m_numDisks; i++) {      //
        AspeqtSettings::ImageSettings is;
        is = aspeqtSettings->mountedImageSetting(i);
        mountFile(i, is.fileName, is.isWriteProtected);
    }
    updateRecentFileActions();

    setAcceptDrops(true);

    // SmartDevice (ApeTime + URL submit)
    SmartDevice *smart = new SmartDevice(sio);
    sio->installDevice(SMART_CDEVIC, smart);

    // AspeQt Client  //
    AspeCl *acl = new AspeCl(sio);
    sio->installDevice(0x46, acl);

    textPrinterWindow = new TextPrinterWindow();
    // Documentation Display
    docDisplayWindow = new DocDisplayWindow();

    connect(textPrinterWindow, SIGNAL(closed()), this, SLOT(textPrinterWindowClosed()));

    Printer *printer = new Printer(sio);
    connect(printer, SIGNAL(print(QString)), textPrinterWindow, SLOT(print(QString)));
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

 void MainWindow::mousePressEvent(QMouseEvent *event)
 {
     int slot = containingDiskSlot(event->pos());

     if (event->button() == Qt::LeftButton
         && slot >= 0) {

         QDrag *drag = new QDrag((QWidget*)this);
         QMimeData *mimeData = new QMimeData;

         mimeData->setData("application/x-aspeqt-disk-image", QByteArray(1, slot));
         drag->setMimeData(mimeData);

         drag->exec();
     }

     if (event->button() == Qt::LeftButton && onOffLabel->geometry().translated(ui->statusBar->geometry().topLeft()).contains(event->pos())) {
         ui->actionStartEmulation->trigger();
     }
     if (event->button() == Qt::LeftButton && prtOnOffLabel->geometry().translated(ui->statusBar->geometry().topLeft()).contains(event->pos())) {
         ui->actionPrinterEmulation->trigger();     //
     }
      if (event->button() == Qt::LeftButton && clearMessagesLabel->geometry().translated(ui->statusBar->geometry().topLeft()).contains(event->pos())) {
         ui->textEdit->clear();
         emit sendLogText("");
     }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    int i = containingDiskSlot(event->pos());
    if (i >= 0 && (event->mimeData()->hasUrls() ||
                   event->mimeData()->hasFormat("application/x-aspeqt-disk-image"))) {
        event->acceptProposedAction();
    } else {
        i = -1;
    }
    for (int j = 0; j < m_numDisks; j++) { //
        if (!diskWidgets[j].frame) continue;   // skip removed slots (gaps)
        if (i == j) {
            diskWidgets[j].frame->setFrameShadow(QFrame::Sunken);
        } else {
            diskWidgets[j].frame->setFrameShadow(QFrame::Raised);
        }
    }
}

void MainWindow::dragMoveEvent(QDragMoveEvent *event)
{
    int i = containingDiskSlot(event->pos());
    if (i >= 0 && (event->mimeData()->hasUrls() ||
                   event->mimeData()->hasFormat("application/x-aspeqt-disk-image"))) {
        event->acceptProposedAction();
    } else {
        i = -1;
    }
    for (int j = 0; j < m_numDisks; j++) { //
        if (!diskWidgets[j].frame) continue;   // skip removed slots (gaps)
        if (i == j) {
            diskWidgets[j].frame->setFrameShadow(QFrame::Sunken);
        } else {
            diskWidgets[j].frame->setFrameShadow(QFrame::Raised);
        }
    }
}

void MainWindow::dragLeaveEvent(QDragLeaveEvent *)
{
    for (int j = 0; j < m_numDisks; j++) { //
        if (!diskWidgets[j].frame) continue;   // skip removed slots (gaps)
        diskWidgets[j].frame->setFrameShadow(QFrame::Raised);
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    for (int j = 0; j < m_numDisks; j++) { //
        if (!diskWidgets[j].frame) continue;   // skip removed slots (gaps)
        diskWidgets[j].frame->setFrameShadow(QFrame::Raised);
    }
    int slot = containingDiskSlot(event->pos());
    if (!(event->mimeData()->hasUrls() ||
          event->mimeData()->hasFormat("application/x-aspeqt-disk-image")) ||
          slot < 0) {
        return;
    }

    if (event->mimeData()->hasFormat("application/x-aspeqt-disk-image")) {
        int source = event->mimeData()->data("application/x-aspeqt-disk-image").at(0);

        if (slot == source) {
            return;
        }

#ifdef Q_OS_ANDROID
        // Finger scrolling occasionally starts an accidental drag that would
        // silently swap two drives, so confirm first on touch.
        if (QMessageBox::question(this, tr("Swap drives"),
                tr("Swap drive %1 with drive %2?").arg(source + 1).arg(slot + 1),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) {
            return;
        }
#endif

        sio->swapDevices(slot + 0x31, source + 0x31);

        aspeqtSettings->swapImages(slot, source);

        PCLINK* pclink = reinterpret_cast<PCLINK*>(sio->getDevice(0x6F));
        if(pclink->hasLink(slot+1) || pclink->hasLink(source+1))
        {
            sio->uninstallDevice(0x6F);
            pclink->swapLinks(slot+1, source+1);
            sio->installDevice(0x6F, pclink);
        }

        qDebug() << "!n" << tr("Swapped disk %1 with disk %2.").arg(slot + 1).arg(source + 1);

        return;
    }

    QStringList files;
    foreach (QUrl url, event->mimeData()->urls()) {
        if (!url.toLocalFile().isEmpty()) {
            files.append(url.toLocalFile());
        }
    }
    if (files.isEmpty()) {
        return;
    }

    FileTypes::FileType type = FileTypes::getFileType(files.at(0));

    if (type == FileTypes::Xex) {
        g_exefileName = files.at(0);  //
        bootExe(files.at(0));
        return;
    }

    if (type == FileTypes::Cas) {
        bool restart;
        restart = ui->actionStartEmulation->isChecked();
        if (restart) {
            ui->actionStartEmulation->trigger();
            sio->wait();
            qApp->processEvents();
        }

        CassetteDialog *dlg = new CassetteDialog(this, files.at(0));
        dlg->exec();
        delete dlg;

        if (restart) {
            ui->actionStartEmulation->trigger();
        }
        return;
    }

    mountFileWithDefaultProtection(slot, files[0]);
    files.removeAt(0);
    while (!files.isEmpty() && (slot = firstEmptyDiskSlot(slot, false)) >= 0) {
        mountFileWithDefaultProtection(slot, files[0]);
        files.removeAt(0);
    }
    slot = 0;
    while (!files.isEmpty() && (slot = firstEmptyDiskSlot(slot, false)) >= 0) {
        mountFileWithDefaultProtection(slot, files[0]);
        files.removeAt(0);
    }
    foreach(QString file, files) {
        qCritical() << "!e" << tr("Cannot mount '%1': No empty disk slots.").arg(file);
    }
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
    delete docDisplayWindow;

    for (int i = 0x31; i < 0x39; i++) {
        SimpleDiskImage *s = qobject_cast <SimpleDiskImage*> (sio->getDevice(i));
        if (s) {
            s->close();
        }
    }

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
#ifdef Q_OS_ANDROID
    // Full-screen immersive; androidRelayout() (also called on every resize)
    // applies the system-bar insets and sizes the drive rows / log.
    QMainWindow::showMaximized();
    QTimer::singleShot(300, this, [this]{ androidRelayout(); });
    QTimer::singleShot(1200, this, [this]{ androidRelayout(); });
#else
    QMainWindow::show();
#endif
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
void MainWindow::enterEvent(QEvent *)
{
    if (g_miniMode && g_shadeMode) {
       setWindowOpacity(1.0);
    }
}
void MainWindow::leaveEvent(QEvent *)
{
    if (g_miniMode && g_shadeMode) {
       setWindowOpacity(0.25);
    }
}
void MainWindow::resizeEvent(QResizeEvent *)
{
#ifdef Q_OS_ANDROID
    androidRelayout();
    // A live rotation leaves Qt/Android with a stale surface (overlapping old
    // rows, a bogus window height, a half-width log). Re-showing the top-level
    // window once per orientation flip recreates the surface cleanly; the guard
    // stops the re-show's own resize events from looping.
    static int lastOri = -1;
    QScreen *scr = screen();
    int ori = (scr && scr->size().width() > scr->size().height()) ? 1 : 0;
    if (ori != lastOri) {
        lastOri = ori;
        QTimer::singleShot(500, this, [this]{
            QMainWindow::hide();
            QMainWindow::showMaximized();
            androidRelayout();
        });
    }
#endif
}

#ifdef Q_OS_ANDROID
// Create all widgets and actions for one drive slot and fill diskWidgets[i].
// Buttons carry the same object names the desktop .ui uses so the rest of the
// code (deviceStatusChanged, androidRelayout) keeps working unchanged.
void MainWindow::buildSlotFrame(int i)
{
    DiskWidgets &d = diskWidgets[i];

    QFrame *f = new QFrame(m_slotContainer);
    f->setObjectName(QString("horizontalFrame_%1").arg(i + 1));
    // Border comes from the stylesheet (deviceStatusChanged repaints it per
    // mount state); NoFrame avoids the native bevel fighting the CSS border.
    f->setFrameShape(QFrame::NoFrame);
    f->setStyleSheet(QString("QFrame#%1 { background:#F7F7F7; border:1px solid #B0B0B0;"
                             " border-radius:6px; }").arg(f->objectName()));
    f->setContextMenuPolicy(Qt::ActionsContextMenu);
    d.frame = f;

    QLabel *numLbl = new QLabel(f);
    numLbl->setObjectName(QString("slotNum_%1").arg(i + 1));
    d.fileNameLabel = new QLabel(f);
    d.fileNameLabel->setObjectName(QString("labelFileName_%1").arg(i + 1));
    d.imagePropertiesLabel = new QLabel(f);
    d.imagePropertiesLabel->setObjectName(QString("labelImageProperties_%1").arg(i + 1));
    // Same fonts the desktop applies via changeFonts() (the dynamic labels are
    // not covered by that ui-> based helper).
    if (aspeqtSettings->useLargeFont()) {
        d.fileNameLabel->setFont(QFont("Arial Black", 16, QFont::Normal));
        d.imagePropertiesLabel->setFont(QFont("Arial Black", 14, QFont::Normal));
    } else {
        d.fileNameLabel->setFont(QFont("MS Shell Dlg 2", 10, QFont::Normal));
        d.imagePropertiesLabel->setFont(QFont("MS Shell Dlg 2", 10, QFont::Normal));
    }

    auto mkAct = [&](const QString &icon, bool checkable, const QString &text) {
        QAction *a = new QAction(f);
        if (!icon.isEmpty()) a->setIcon(QIcon(icon));
        a->setCheckable(checkable);
        a->setText(text);
        a->setToolTip(text);
        return a;
    };
    d.mountDiskAction    = mkAct(":/icons/tango-icons/devices/drive-optical.svg", false, tr("Mount disk image"));
    d.mountFolderAction  = mkAct(":/icons/tango-icons/places/folder.svg", false, tr("Mount folder image"));
    d.saveAction         = mkAct(":/icons/tango-icons/devices/media-floppy.svg", false, tr("Save disk"));
    d.autoSaveAction     = mkAct(":/icons/tango-icons/actions/document-save-as.svg", true, tr("Auto-commit"));
    d.editAction         = mkAct(":/icons/tango-icons/apps/system-file-manager.svg", true, tr("Disk explorer"));
    // A new slot starts empty: show the placeholder hint (deviceStatusChanged()
    // replaces it with the file name once something is mounted).
    d.fileNameLabel->setText(tr("Mount a disk image or folder."));
    d.fileNameLabel->setStyleSheet("color:#B0B0B0; font-style:italic; font-weight:normal; font-size:12px;");
    d.ejectAction        = mkAct(":/icons/tango-icons/actions/media-eject.svg", false, tr("Eject"));
    // A new slot starts empty, so the eject button starts as the "remove slot"
    // trash button; deviceStatusChanged() swaps in the eject icon once mounted.
    d.ejectAction->setIcon(removeSlotIcon());
    d.ejectAction->setToolTip(tr("Remove slot"));
    d.writeProtectAction = mkAct(":/icons/silk-icons/icons/lock_open.png", true, tr("Write protect"));
    d.saveAsAction       = mkAct(":/icons/silk-icons/icons/drive_rename.png", false, tr("Save disk as"));
    d.revertAction       = mkAct(":/icons/silk-icons/icons/arrow_undo.png", false, tr("Revert to last saved"));
    d.bootOptionAction   = (i == 0) ? mkAct(":/icons/oxygen-icons/16x16/actions/flag_green.png", false, tr("Boot options")) : nullptr;

    connect(d.mountDiskAction,   &QAction::triggered, this, [this, i]{ mountDiskImage(i); });
    connect(d.mountFolderAction, &QAction::triggered, this, [this, i]{ mountFolderImage(i); });
    connect(d.saveAction,        &QAction::triggered, this, [this, i]{ saveDisk(i); });
    connect(d.autoSaveAction,    &QAction::triggered, this, [this, i]{ autoSaveDisk(i); });
    connect(d.editAction,        &QAction::triggered, this, [this, i]{ openEditor(i); });
    connect(d.ejectAction,       &QAction::triggered, this, [this, i]{ androidEjectPressed(i); });
    connect(d.writeProtectAction,&QAction::triggered, this, [this, i]{ toggleWriteProtection(i); });
    connect(d.saveAsAction,      &QAction::triggered, this, [this, i]{ saveDiskAs(i); });
    connect(d.revertAction,      &QAction::triggered, this, [this, i]{ revertDisk(i); });
    if (d.bootOptionAction)
        connect(d.bootOptionAction, &QAction::triggered, this, [this]{ on_actionBootOption_triggered(); });

    if (d.bootOptionAction) f->insertAction(0, d.bootOptionAction);
    f->insertAction(0, d.saveAction);
    f->insertAction(0, d.autoSaveAction);
    f->insertAction(0, d.saveAsAction);
    f->insertAction(0, d.revertAction);
    f->insertAction(0, d.mountDiskAction);
    f->insertAction(0, d.mountFolderAction);
    f->insertAction(0, d.ejectAction);
    f->insertAction(0, d.writeProtectAction);
    f->insertAction(0, d.editAction);

    auto mkBtn = [&](const QString &name, QAction *a) {
        QToolButton *b = new QToolButton(f);
        b->setObjectName(name.arg(i + 1));
        b->setDefaultAction(a);
        return b;
    };
    mkBtn("buttonMountDisk_%1",   d.mountDiskAction);
    mkBtn("buttonMountFolder_%1", d.mountFolderAction);
    mkBtn("buttonSave_%1",        d.saveAction);
    mkBtn("autoSave_%1",          d.autoSaveAction);
    mkBtn("buttonEditDisk_%1",    d.editAction);
    mkBtn("buttonEject_%1",       d.ejectAction);

    layoutSlotFrame(i);
}

// (Re)build the inner layout of slot i:
//   [ number badge | icons row / name+type row ]
// The five action icons are centred between the badge and the eject icon.
void MainWindow::layoutSlotFrame(int i)
{
    QFrame *f = diskWidgets[i].frame;
    if (!f) return;

    QLabel *fileLbl = diskWidgets[i].fileNameLabel;
    QLabel *typeLbl = diskWidgets[i].imagePropertiesLabel;
    QLabel *numLbl  = f->findChild<QLabel *>(QString("slotNum_%1").arg(i + 1));

    QList<QToolButton *> btns;
    btns << f->findChild<QToolButton *>(QString("buttonMountDisk_%1").arg(i + 1))
         << f->findChild<QToolButton *>(QString("buttonMountFolder_%1").arg(i + 1))
         << f->findChild<QToolButton *>(QString("buttonSave_%1").arg(i + 1))
         << f->findChild<QToolButton *>(QString("autoSave_%1").arg(i + 1))
         << f->findChild<QToolButton *>(QString("buttonEditDisk_%1").arg(i + 1))
         << f->findChild<QToolButton *>(QString("buttonEject_%1").arg(i + 1));

    delete f->layout();

    QHBoxLayout *outer = new QHBoxLayout(f);
    outer->setContentsMargins(8, 3, 8, 3);
    outer->setSpacing(8);
    if (numLbl) {
        numLbl->setText(QString::number(i + 1));
        numLbl->setAlignment(Qt::AlignCenter);
        numLbl->setFixedSize(30, 30);
        numLbl->setStyleSheet("QLabel { background:#E0A030; color:white;"
                              " border-radius:8px; font-weight:bold; font-size:14px; }");
        outer->addWidget(numLbl, 0, Qt::AlignVCenter);
    }

    QVBoxLayout *col = new QVBoxLayout();
    col->setSpacing(2);

    QHBoxLayout *btnRow = new QHBoxLayout();
    btnRow->setSpacing(6);
    btnRow->addStretch();
    for (int k = 0; k < btns.size() - 1; ++k)   // first five, centred
        if (btns[k]) btnRow->addWidget(btns[k], 0, Qt::AlignVCenter);
    btnRow->addStretch();
    btnRow->addSpacing(6);
    if (btns.last())                            // eject -> right edge
        btnRow->addWidget(btns.last(), 0, Qt::AlignVCenter);

    QHBoxLayout *lblRow = new QHBoxLayout();
    lblRow->setSpacing(8);
    if (fileLbl) {
        fileLbl->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        QFont ff = fileLbl->font();
        ff.setBold(true);
        fileLbl->setFont(ff);
        lblRow->addWidget(fileLbl, 0, Qt::AlignBottom);
    }
    lblRow->addStretch();
    if (typeLbl) {
        // Type right-aligned, kept off the right edge by the same gap the name
        // has on the left.
        typeLbl->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        QFont tf = typeLbl->font();
        tf.setPointSize(qMax(1, tf.pointSize() - 1));
        typeLbl->setFont(tf);
        typeLbl->setStyleSheet("color:#8A8A8A;");
        lblRow->addWidget(typeLbl, 0, Qt::AlignBottom);
        lblRow->addSpacing(6);
    }

    col->addLayout(btnRow);
    col->addLayout(lblRow);
    outer->addLayout(col);
}

// Build the scrollable slot column and the first m_numDisks slots, then drop it
// into the grid where the .ui's fixed drive frames used to be.
void MainWindow::androidBuildSlots()
{
    m_slotContainer = new QWidget;
    m_slotBox = new QVBoxLayout(m_slotContainer);
    // Match the original grid's row spacing and side margins so the slot column
    // keeps the exact geometry it had as six fixed rows.
    m_slotBox->setContentsMargins(ui->gridLayout->contentsMargins().left(), 0,
                                  ui->gridLayout->contentsMargins().right(), 0);
    m_slotBox->setSpacing(ui->gridLayout->spacing());

    m_slotScroll = new QScrollArea(ui->centralWidget);
    m_slotScroll->setObjectName("slotScroll");
    m_slotScroll->setWidgetResizable(true);
    // Finger-scroll only (vertical); no scrollbars, no horizontal scrolling.
    m_slotScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_slotScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_slotScroll->setFrameShape(QFrame::NoFrame);
    // Transparent so the window background shows through, matching the area
    // around the slots instead of a grey scroll-area fill.
    m_slotScroll->setStyleSheet("QScrollArea { background:transparent; border:none; }");
    m_slotScroll->viewport()->setAutoFillBackground(false);
    m_slotScroll->setWidget(m_slotContainer);
    m_slotContainer->setObjectName("slotContainer");
    m_slotContainer->setAutoFillBackground(false);
    m_slotContainer->setStyleSheet("QWidget#slotContainer { background:transparent; }");
    // Kinetic drag-scroll. Keeping the content exactly viewport-wide (see
    // androidRelayout) leaves no horizontal range, so QScroller only pans
    // vertically and horizontal swipes fall through to the slot frames.
    QScroller::grabGesture(m_slotScroll->viewport(), QScroller::LeftMouseButtonGesture);
    {
        QScroller *sc = QScroller::scroller(m_slotScroll->viewport());
        QScrollerProperties sp = sc->scrollerProperties();
        sp.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy,
                           QVariant::fromValue(QScrollerProperties::OvershootAlwaysOff));
        sc->setScrollerProperties(sp);
    }

    // Trailing "+" row that adds a slot; styled like a slot box.
    m_addSlotRow = new QFrame(m_slotContainer);
    m_addSlotRow->setObjectName("addSlotRow");
    // Same look as an empty slot.
    m_addSlotRow->setStyleSheet("QFrame#addSlotRow { background:#F7F7F7;"
                                " border:1px solid #B0B0B0; border-radius:6px; }");
    QHBoxLayout *al = new QHBoxLayout(m_addSlotRow);
    al->setContentsMargins(8, 2, 8, 2);
    m_addSlotBtn = new QToolButton(m_addSlotRow);
    m_addSlotBtn->setObjectName("buttonAddSlot");
    m_addSlotBtn->setAutoRaise(true);
    m_addSlotBtn->setIcon(QIcon(":/icons/tango-icons/actions/list-add.svg"));
    m_addSlotBtn->setToolTip(tr("Add drive slot"));
    // The whole row is the click target, not just the icon.
    m_addSlotBtn->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_addSlotRow->setCursor(Qt::PointingHandCursor);
    m_addSlotRow->installEventFilter(this);
    al->addStretch();
    al->addWidget(m_addSlotBtn);
    al->addStretch();

    m_slotBox->addWidget(m_addSlotRow);
    m_slotBox->addStretch();

    // The always-on-top loader slot (inserts itself at box position 0).
    androidBuildLoaderSlot();

    // Remove the .ui's fixed frames from the grid and put the scroll area in
    // their place (row 1, spanning the six old drive rows).
    for (int i = 1; i <= 6; ++i)
        if (QFrame *old = ui->centralWidget->findChild<QFrame *>(QString("horizontalFrame_%1").arg(i))) {
            ui->gridLayout->removeWidget(old);
            old->hide();
        }
    ui->gridLayout->addWidget(m_slotScroll, 1, 0, 6, 1);

    androidRebuildSlots();
}

// Layout position (in m_slotBox) for slot i: the number of present slots with a
// lower index. Used to insert a gap-filling frame in the right place.
int MainWindow::androidBoxPos(int i)
{
    int pos = 0;
    for (int k = 0; k < i; ++k)
        if (diskWidgets[k].frame) pos++;
    return pos;
}

// Tear down existing slot frames and rebuild the present ones from settings.
// Absent (removed) indices are skipped, so their slot numbers stay as gaps.
void MainWindow::androidRebuildSlots()
{
    for (int i = 0; i < MAX_DISKS; ++i)
        if (diskWidgets[i].frame) {
            m_slotBox->removeWidget(diskWidgets[i].frame);
            delete diskWidgets[i].frame;
            diskWidgets[i] = DiskWidgets();
        }
    m_numDisks = aspeqtSettings->numberOfDisks();
    if (m_numDisks < 1)         m_numDisks = DEFAULT_DISKS;
    if (m_numDisks > MAX_DISKS)  m_numDisks = MAX_DISKS;
    int base = m_loaderFrame ? 1 : 0;   // loader slot occupies box position 0
    int pos = 0;
    for (int i = 0; i < m_numDisks; ++i) {
        if (!aspeqtSettings->slotPresent(i)) continue;   // removed slot -> gap
        buildSlotFrame(i);
        m_slotBox->insertWidget(base + pos, diskWidgets[i].frame);
        pos++;
    }
    androidUpdateAddRow();
}

// Hide the "+" row once every hardware slot index is in use.
void MainWindow::androidUpdateAddRow()
{
    bool room = false;
    for (int i = 0; i < MAX_DISKS; ++i)
        if (!diskWidgets[i].frame) { room = true; break; }
    if (m_addSlotRow) m_addSlotRow->setVisible(room);
}

// "+" -> fill the lowest number gap, or append a new slot at the end.
void MainWindow::androidAddSlot()
{
    int i = -1;
    for (int k = 0; k < MAX_DISKS; ++k)
        if (!diskWidgets[k].frame) { i = k; break; }
    if (i < 0) return;                       // all slots present
    buildSlotFrame(i);
    m_slotBox->insertWidget((m_loaderFrame ? 1 : 0) + androidBoxPos(i), diskWidgets[i].frame);
    if (i >= m_numDisks) m_numDisks = i + 1;
    aspeqtSettings->setNumberOfDisks(m_numDisks);
    aspeqtSettings->setSlotPresent(i, true);
    deviceStatusChanged(i + 0x31);           // paint it as empty
    androidUpdateAddRow();
    androidRelayout();
}

// 2nd eject on an empty slot: drop this specific slot, leaving a number gap so
// the other slots keep their device numbers (important for DOS).
void MainWindow::androidRemoveSlot(int i)
{
    if (i < 0 || i >= MAX_DISKS || !diskWidgets[i].frame) return;
    int present = 0;
    for (int k = 0; k < MAX_DISKS; ++k) if (diskWidgets[k].frame) present++;
    if (present <= 1) return;                // keep at least one slot

    m_slotBox->removeWidget(diskWidgets[i].frame);
    delete diskWidgets[i].frame;
    diskWidgets[i] = DiskWidgets();
    aspeqtSettings->setSlotPresent(i, false);
    // Shrink the persisted range to the highest still-present slot.
    int hi = 0;
    for (int k = 0; k < MAX_DISKS; ++k) if (diskWidgets[k].frame) hi = k + 1;
    m_numDisks = hi;
    aspeqtSettings->setNumberOfDisks(m_numDisks);
    androidUpdateAddRow();
    androidRelayout();
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
void MainWindow::androidBuildLoaderSlot()
{
    QFrame *f = new QFrame(m_slotContainer);
    f->setObjectName("loaderFrame");
    f->setFrameShape(QFrame::NoFrame);
    f->setStyleSheet("QFrame#loaderFrame { background:#F7F7F7; border:1px solid #B0B0B0; border-radius:6px; }");
    m_loaderFrame = f;

    m_loaderBadge = new QLabel(f);
    m_loaderBadge->setObjectName("loaderBadge");
    m_loaderBadge->setAlignment(Qt::AlignCenter);
    m_loaderBadge->setFixedSize(30, 30);
    m_loaderBadge->setTextFormat(Qt::RichText);
    m_loaderBadge->setText(QStringLiteral("cas<br>xex"));
    m_loaderBadge->setStyleSheet("QLabel#loaderBadge { background:#E0A030; color:white;"
                                 " border-radius:8px; font-weight:bold; font-size:10px; }");

    m_loaderFileLbl = new QLabel(f);
    m_loaderFileLbl->setObjectName("loaderFileLbl");
    m_loaderTypeLbl = new QLabel(f);
    m_loaderTypeLbl->setObjectName("loaderTypeLbl");

    auto mkBtn = [&](const QString &icon, const QString &tip) {
        QToolButton *b = new QToolButton(f);
        b->setIcon(QIcon(icon));
        b->setToolTip(tip);
        return b;
    };
    m_loaderLoadBtn  = mkBtn(":/icons/tango-icons/categories/applications-system.svg", tr("Load executable or cassette"));
    m_loaderPlayBtn  = mkBtn(":/icons/tango-icons/actions/media-playback-start.svg", tr("Start cassette playback"));
    m_loaderRetryBtn = mkBtn(":/icons/tango-icons/actions/view-refresh.svg", tr("Retry"));
    m_loaderEjectBtn = mkBtn(":/icons/tango-icons/actions/media-eject.svg", tr("Eject"));
    // Invisible placeholders so the three loader icons line up with the first
    // three icons of the (five-icon) disk slots.
    m_loaderSpacer1 = new QWidget(f);
    m_loaderSpacer2 = new QWidget(f);

    connect(m_loaderLoadBtn,  &QToolButton::clicked, this, [this]{ loaderLoad(); });
    connect(m_loaderPlayBtn,  &QToolButton::clicked, this, [this]{ loaderPlayCas(); });
    connect(m_loaderRetryBtn, &QToolButton::clicked, this, [this]{ loaderRetry(); });
    connect(m_loaderEjectBtn, &QToolButton::clicked, this, [this]{ loaderEject(); });

    QHBoxLayout *outer = new QHBoxLayout(f);
    outer->setContentsMargins(8, 3, 8, 3);
    outer->setSpacing(8);
    outer->addWidget(m_loaderBadge, 0, Qt::AlignVCenter);

    QVBoxLayout *col = new QVBoxLayout();
    col->setSpacing(2);
    QHBoxLayout *btnRow = new QHBoxLayout();
    btnRow->setSpacing(6);
    btnRow->addStretch();
    btnRow->addWidget(m_loaderLoadBtn,  0, Qt::AlignVCenter);
    btnRow->addWidget(m_loaderPlayBtn,  0, Qt::AlignVCenter);
    btnRow->addWidget(m_loaderRetryBtn, 0, Qt::AlignVCenter);
    btnRow->addWidget(m_loaderSpacer1,  0, Qt::AlignVCenter);
    btnRow->addWidget(m_loaderSpacer2,  0, Qt::AlignVCenter);
    btnRow->addStretch();
    btnRow->addSpacing(6);
    btnRow->addWidget(m_loaderEjectBtn, 0, Qt::AlignVCenter);

    // Name + type row, identical to the disk slots (name bold left, type right).
    if (aspeqtSettings->useLargeFont()) {
        m_loaderFileLbl->setFont(QFont("Arial Black", 16, QFont::Normal));
        m_loaderTypeLbl->setFont(QFont("Arial Black", 14, QFont::Normal));
    } else {
        m_loaderFileLbl->setFont(QFont("MS Shell Dlg 2", 10, QFont::Normal));
        m_loaderTypeLbl->setFont(QFont("MS Shell Dlg 2", 10, QFont::Normal));
    }
    QHBoxLayout *lblRow = new QHBoxLayout();
    lblRow->setSpacing(8);
    m_loaderFileLbl->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    QFont ff = m_loaderFileLbl->font();
    ff.setBold(true);
    m_loaderFileLbl->setFont(ff);
    lblRow->addWidget(m_loaderFileLbl, 0, Qt::AlignBottom);
    lblRow->addStretch();
    m_loaderTypeLbl->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    QFont tf = m_loaderTypeLbl->font();
    tf.setPointSize(qMax(1, tf.pointSize() - 1));
    m_loaderTypeLbl->setFont(tf);
    m_loaderTypeLbl->setStyleSheet("color:#8A8A8A;");
    lblRow->addWidget(m_loaderTypeLbl, 0, Qt::AlignBottom);
    lblRow->addSpacing(6);

    col->addLayout(btnRow);
    col->addLayout(lblRow);
    outer->addLayout(col);

    m_slotBox->insertWidget(0, f);
    loaderEject();   // initialise: placeholder text, buttons disabled, no fill
}

// Fill the whole loader slot left-to-right as a progress bar. frac<=0 or >=1
// restores the plain background (so the fill disappears once loading is done).
void MainWindow::loaderSetFill(double frac)
{
    if (!m_loaderFrame) return;
    if (frac <= 0.0 || frac >= 1.0) {
        // Idle: blue accent when a file is loaded, plain otherwise.
        if (m_loaderKind != 0)
            m_loaderFrame->setStyleSheet("QFrame#loaderFrame { background:#EAF3FB;"
                " border:1px solid #6FA8DC; border-radius:6px; }");
        else
            m_loaderFrame->setStyleSheet("QFrame#loaderFrame { background:#F7F7F7;"
                " border:1px solid #B0B0B0; border-radius:6px; }");
        return;
    }
    double p2 = qMin(frac + 0.0005, 1.0);
    m_loaderFrame->setStyleSheet(QString(
        "QFrame#loaderFrame { border:1px solid #B0B0B0; border-radius:6px;"
        " background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        " stop:0 #CFE6FF, stop:%1 #CFE6FF, stop:%2 #F7F7F7, stop:1 #F7F7F7); }")
        .arg(frac, 0, 'f', 4).arg(p2, 0, 'f', 4));
}

// Enable/disable the loader buttons for the current state.
void MainWindow::loaderUpdateButtons()
{
    if (!m_loaderPlayBtn) return;
    bool casReady = (m_loaderKind == 2) && m_casWorker && !m_casWorker->isRunning();
    m_loaderPlayBtn->setEnabled(casReady);
    m_loaderRetryBtn->setEnabled(!m_loaderFile.isEmpty());
    m_loaderEjectBtn->setEnabled(m_loaderKind != 0);
}

// Load button: pick an XEX/CAS file and dispatch by type.
void MainWindow::loaderLoad()
{
    QString url = androidOpenUrl(tr("Load executable or cassette"),
        tr("Atari programs (*.xex *.com *.exe *.cas);;All files (*)"));
    if (url.isEmpty())
        return;
    // Always copy a content:// pick to a real temp file: QFile on a SAF stream
    // can pass a one-shot open() probe yet fail the repeated sequential reads /
    // atEnd() that CAS parsing and the boot loader need (seen as "Unknown
    // error" on some files but not others, depending on their SAF location).
    QString path = androidLocalCopy(url);
    if (path.isEmpty()) {
        qWarning() << "!i" << tr("Failed to load '%1'.").arg(friendlyName(url));
        return;
    }
    FileTypes::FileType t = FileTypes::getFileType(path);
    if (t == FileTypes::Cas || t == FileTypes::CasGz)
        loaderLoadCas(path);
    else if (t == FileTypes::Xex || t == FileTypes::XexGz)
        loaderLoadXex(path);
    else
        QMessageBox::information(this, tr("Unsupported file"),
            tr("Pick an Atari executable (.xex/.com/.exe) or a cassette image (.cas)."));
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
    m_loaderFileLbl->setStyleSheet("color: rgb(32,32,32); font-weight:bold;");
    m_loaderFileLbl->setText(friendlyName(path));
    m_loaderTypeLbl->setText(tr("Executable (%1k)").arg((QFileInfo(path).size() + 512) / 1024));
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
    m_loaderFileLbl->setStyleSheet("color: rgb(32,32,32); font-weight:bold;");
    m_loaderFileLbl->setText(friendlyName(path));
    {
        int minutes = m_casTotal / 60000;
        int seconds = (m_casTotal - minutes * 60000) / 1000;
        m_loaderTypeLbl->setText(tr("Cassette (%1:%2)").arg(minutes).arg(seconds, 2, 10, QChar('0')));
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
    if (m_loaderFileLbl) {
        m_loaderFileLbl->setText(tr("Load a cas/com/xex file."));
        m_loaderFileLbl->setStyleSheet("color:#B0B0B0; font-style:italic; font-weight:normal; font-size:12px;");
    }
    if (m_loaderTypeLbl) m_loaderTypeLbl->clear();
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

void MainWindow::androidRelayout()
{
    // setContentsMargins()/changeSize() below trigger a relayout (another
    // resizeEvent); guard against re-entering while we're mid-adjust.
    static bool busy = false;
    if (busy) return;
    busy = true;

    // --- system-bar insets -> window margins (edge-to-edge safe area) ---------
    // targetSdk 35+ forces the surface full-screen with the status/navigation
    // bars drawn as overlays. Inset the window so nothing hides under them.
    long packed = QJniObject::callStaticMethod<jlong>("net/greblus/SerialActivity", "systemBarInsets");
    qreal dpr = devicePixelRatioF();
    if (dpr < 1.0) dpr = 1.0;
    int il = (int)((packed >> 48) & 0xffff);
    int it = (int)((packed >> 32) & 0xffff);
    int ir = (int)((packed >> 16) & 0xffff);
    int ib = (int)( packed        & 0xffff);
    int ml = qRound(il / dpr);
    int mt = qRound(it / dpr);   // status bar + ActionBar height (from Java)
    int mr = qRound(ir / dpr);
    int mb = qRound(ib / dpr);
    // A small aesthetic gap below the ActionBar; the sides/bottom clear the nav
    // bar and any display cutout.
    const int topPad = 8;
    setContentsMargins(ml, mt, mr, mb);

    // Insets may not be published yet on the very first layout / just after a
    // rotation; retry shortly so the window still ends up correctly inset.
    if (packed == 0)
        QTimer::singleShot(200, this, [this]{ androidRelayout(); });

    // --- adaptive vertical budget --------------------------------------------
    // Lay out as: top pad, 6 drive rows, log (fills the rest), status bar. Size
    // the rows from the available height so the log always keeps a few lines,
    // shrinking the rows in landscape rather than starving the log.
    QWidget *central = ui->centralWidget;
    // Derive the height available to the central widget directly from the window
    // (minus the inset margins and the status bar) rather than central->height():
    // when the grid's minimum overflows, central->height() reports the *overflowed*
    // size and the status bar gets pushed off-screen.
    int chrome = qMax(statusBar()->height(), statusBar()->sizeHint().height());
    // Use the screen height, not window height(): after a live rotation the
    // fullscreen window briefly reports a bogus height (larger than the screen),
    // which would over-inflate the budget and make the rows too tall.
    QScreen *scr = screen();
    int winH   = scr ? scr->size().height() : height();
    int avail  = winH - contentsMargins().top() - contentsMargins().bottom() - chrome;
    QFontMetrics fm(ui->textEdit->font());
    int minLog  = fm.lineSpacing() * 2 + 10;          // keep ~2 log lines
    int gaps    = ui->gridLayout->spacing() * 8;      // 9 rows -> 8 gaps
    int margins = 6;                                  // grid top+bottom margins
    int forRows = avail - topPad - minLog - gaps - margins;
    // Floor low enough that the log's reserved height survives in cramped
    // landscape (rows shrink instead of starving the log); cap so portrait rows
    // stay only a little larger than landscape rather than ballooning.
    int rowH    = qBound(30, forRows / 6, 46);

    // The frame is the slot "box": give it an equal top/bottom pad and size the
    // (square) buttons to what's left, so the icons sit centred inside the panel
    // instead of poking out of the bottom edge.
    int pad    = 2;                                   // vertical pad inside the box
    int btnH   = rowH - 2 * pad - 2;                  // -2 for the frame border
    int iconPx = btnH - 3;                            // less padding -> larger icons
    // Two-line slot box: pad + button row + gap + description line + pad + border.
    int labelH = 0;
    if (QLabel *l0 = findChild<QLabel *>("labelFileName_1"))
        labelH = QFontMetrics(l0->font()).height();
    int frameH = 3 + btnH + 2 + labelH + 3 + 2;
    // Size every active slot frame plus the trailing "+" row.
    for (int i = 0; i < m_numDisks; ++i)
        if (diskWidgets[i].frame) {
            diskWidgets[i].frame->setMinimumHeight(frameH);
            diskWidgets[i].frame->setMaximumHeight(frameH);
        }
    if (m_loaderFrame) {
        m_loaderFrame->setMinimumHeight(frameH);
        m_loaderFrame->setMaximumHeight(frameH);
    }
    // The "+" row is a compact bar, not a full-height slot.
    if (m_addSlotRow) {
        int addH = btnH + 8;
        m_addSlotRow->setMinimumHeight(addH);
        m_addSlotRow->setMaximumHeight(addH);
    }
    // Size the scroll area to its content so the log expands up to just under
    // the "+" row (hiding the grey fill), but never taller than the six-row
    // height it has now — beyond that the slots scroll and the log keeps its
    // current minimum height.
    if (m_slotScroll) {
        int spacing = m_slotBox ? m_slotBox->spacing() : 3;
        int addH = btnH + 8;
        int loaderH = m_loaderFrame ? frameH : 0;
        int loaderN = m_loaderFrame ? 1 : 0;
        int present = 0;
        for (int k = 0; k < MAX_DISKS; ++k) if (diskWidgets[k].frame) present++;
        bool addVisible = m_addSlotRow && m_addSlotRow->isVisible();
        int items = loaderN + present + (addVisible ? 1 : 0);
        int content = loaderH + present * frameH + (addVisible ? addH : 0)
                    + (items > 0 ? (items - 1) * spacing : 0);
        // Cap so the default set (loader + DEFAULT_DISKS slots + "+") fits; more
        // slots than that scroll while the log keeps its height.
        int capItems = loaderN + DEFAULT_DISKS + 1;
        int cap = loaderH + DEFAULT_DISKS * frameH + addH + (capItems - 1) * spacing;
        int h = qMin(content, cap);
        m_slotScroll->setMinimumHeight(h);
        m_slotScroll->setMaximumHeight(h);
        // Hint once when the slots first stop fitting (must be scrolled to see
        // them all); reset when they fit again so it can fire next time.
        bool over = content > cap;
        if (over && !m_slotsOverflowed)
            qWarning() << "!i" << tr("Scroll the slot list to see all of them.");
        m_slotsOverflowed = over;
        // Pin the content to the viewport width so there is no horizontal scroll
        // range (keeps scrolling vertical-only).
        int vpW = m_slotScroll->maximumViewportSize().width();
        if (vpW > 0 && m_slotContainer)
            m_slotContainer->setMaximumWidth(vpW);
    }
    foreach (QToolButton *btn, central->findChildren<QToolButton *>()) {
        btn->setMinimumSize(btnH, btnH);
        btn->setMaximumSize(btnH, btnH);
        // The disk-viewer (edit) icon has more internal padding than the others,
        // so bump its icon size a little to match the visual weight.
        int px = btn->objectName().startsWith("buttonEditDisk") ? iconPx + 5 : iconPx;
        btn->setIconSize(QSize(px, px));
    }
    // Loader placeholders take a button's footprint so its 3 icons line up with
    // the disk slots' first 3 icons.
    if (m_loaderSpacer1) m_loaderSpacer1->setFixedSize(btnH, btnH);
    if (m_loaderSpacer2) m_loaderSpacer2->setFixedSize(btnH, btnH);
    ui->verticalSpacer_2->changeSize(0, topPad, QSizePolicy::Fixed, QSizePolicy::Fixed);
    ui->verticalSpacer->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
    ui->gridLayout->invalidate();
    ui->gridLayout->activate();    // apply the new geometry synchronously
    // After a live rotation QAbstractScrollArea leaves the log's viewport stuck
    // at the previous orientation's width (frame is full width, but the white
    // viewport background only fills half). layoutChildren() won't fix it, so
    // resize the viewport widget directly to fill the textEdit's frame.
    {
        QRect fr = ui->textEdit->contentsRect();
        ui->textEdit->viewport()->setGeometry(fr);
        ui->textEdit->viewport()->update();
    }
    ui->centralWidget->update();   // repaint vacated regions after a resize

    busy = false;
}
#endif

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
#ifdef Q_OS_ANDROID
    // A tap anywhere on the "+" row adds a slot.
    if (obj == m_addSlotRow) {
        if (event->type() == QEvent::MouseButtonRelease) {
            androidAddSlot();
            return true;
        }
        return false;
    }
#endif
    if (event->type() == QEvent::MouseButtonDblClick) {
        on_actionLogWindow_triggered();
        return false;
    }
    return true;
}
void MainWindow::on_actionLogWindow_triggered()
{
    if (g_logOpen == false) {
        g_logOpen = true;
        QDialog *ldd = new LogDisplayDialog(this);
        int x, y, w, h;
        x = geometry().x();
        y = geometry().y();
        w = geometry().width();
        h = geometry().height();
        if (!g_miniMode) {
            ldd->setGeometry(x+w/1.9, y+30, ldd->geometry().width(), geometry().height());
        } else {
            ldd->setGeometry(x+20, y+60, w, h*2);
        }
        connect(this, SIGNAL(sendLogText(QString)), ldd, SLOT(getLogText(QString)));
        connect(this, SIGNAL(sendLogTextChange(QString)), ldd, SLOT(getLogTextChange(QString)));
        emit sendLogText(ui->textEdit->toHtml());
        ldd->show();
    }
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
    qWarning() << "!i" << tr("Emulation stopped.");
}

void MainWindow::sioStatusChanged(QString status)
{
    speedLabel->setText(status);
    speedLabel->show();
}

void MainWindow::deviceStatusChanged(int deviceNo)
{
    if (deviceNo >= 0x31 && deviceNo <= 0x31 + MAX_DISKS - 1) { //
        // Skip slots that were removed (their widgets no longer exist).
        if (!diskWidgets[deviceNo - 0x31].frame)
            return;
        SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(deviceNo));
        if (img) {

            // Show file name without the path and set toolTip & statusTip to show the path separately //
            QString filenamelabel;
            int i;
            if (img->description() == tr("Folder image")) {
                i = img->originalFileName().lastIndexOf("\\");
            } else {
                i = img->originalFileName().lastIndexOf("/");
            }
               if (i == -1) {
                   i = img->originalFileName().lastIndexOf("/");
               }
            if ((i != -1) || (img->originalFileName().mid(0, 14) == "Untitled image")) {
                filenamelabel = img->originalFileName().right(img->originalFileName().size() - ++i);
            } else {
                filenamelabel = "!!!!!!!!.!!!";
                }
#ifdef Q_OS_ANDROID
            // content:// URIs have no real path/name; resolve a display name.
            if (img->originalFileName().startsWith("content:")) {
                filenamelabel = friendlyName(img->originalFileName());
            }
#endif

            diskWidgets[deviceNo - 0x31].fileNameLabel->setToolTip(img->originalFileName().left(i - 1));
            diskWidgets[deviceNo - 0x31].fileNameLabel->setStatusTip(img->originalFileName());
            diskWidgets[deviceNo - 0x31].imagePropertiesLabel->setToolTip(img->description());

            diskWidgets[deviceNo - 0x31].fileNameLabel->setText(filenamelabel);
            diskWidgets[deviceNo - 0x31].imagePropertiesLabel->setText(img->description());
            diskWidgets[deviceNo - 0x31].ejectAction->setEnabled(true);
#ifdef Q_OS_ANDROID
            // Mounted: show the eject icon (empty slots show a trash icon).
            diskWidgets[deviceNo - 0x31].ejectAction->setIcon(QIcon(":/icons/tango-icons/actions/media-eject.svg"));
            diskWidgets[deviceNo - 0x31].ejectAction->setToolTip(tr("Eject"));
#endif
            diskWidgets[deviceNo - 0x31].editAction->setChecked(img->editDialog() != 0);
#ifdef Q_OS_ANDROID
            // Mounted slot: subtle accent box so it stands out from empty ones.
            diskWidgets[deviceNo - 0x31].frame->setStyleSheet(
                QString("QFrame#%1 { background:#EAF3FB; border:1px solid #6FA8DC; border-radius:6px; }")
                    .arg(diskWidgets[deviceNo - 0x31].frame->objectName()));
#endif
            if (img->description() == tr("Folder image")) {
                diskWidgets[deviceNo - 0x31].fileNameLabel->setStyleSheet("color: rgb(54, 168, 164); font-weight: bold");
                diskWidgets[deviceNo - 0x31].editAction->setEnabled(true);              //
                diskWidgets[deviceNo - 0x31].saveAsAction->setEnabled(false);
                diskWidgets[deviceNo - 0x31].saveAction->setEnabled(false);
                diskWidgets[deviceNo - 0x31].autoSaveAction->setEnabled(false);         //
                diskWidgets[deviceNo - 0x31].revertAction->setEnabled(false);
#ifdef Q_OS_ANDROID
                // For a folder the "save" button becomes "install high-speed DOS".
                diskWidgets[deviceNo - 0x31].saveAction->setEnabled(true);
                diskWidgets[deviceNo - 0x31].saveAction->setIcon(dosDriveIcon());
                diskWidgets[deviceNo - 0x31].saveAction->setToolTip(tr("Install high-speed DOS (MyPicoDOS) into this folder"));
#endif
                if(deviceNo - 0x31 == 0)
                    diskWidgets[deviceNo - 0x31].bootOptionAction->setEnabled(true);   //
            } else {
#ifdef Q_OS_ANDROID
                // Restore the normal "save disk" icon for real disk images.
                diskWidgets[deviceNo - 0x31].saveAction->setIcon(QIcon(":/icons/tango-icons/devices/media-floppy.svg"));
                diskWidgets[deviceNo - 0x31].saveAction->setToolTip(tr("Save disk"));
#endif
                diskWidgets[deviceNo - 0x31].fileNameLabel->setStyleSheet("color: rgb(32, 32, 32); font-weight: bold");  //
                diskWidgets[deviceNo - 0x31].editAction->setEnabled(true);
                diskWidgets[deviceNo - 0x31].saveAsAction->setEnabled(true);
                diskWidgets[deviceNo - 0x31].autoSaveAction->setEnabled(true);          //
                if(deviceNo - 0x31 == 0)
                    diskWidgets[deviceNo - 0x31].bootOptionAction->setEnabled(false);   //

                if (img->isModified()) {
                    if (!diskWidgets[deviceNo - 0x31].autoSaveAction->isChecked()) {    //
                        diskWidgets[deviceNo - 0x31].saveAction->setEnabled(true);
                        diskWidgets[deviceNo - 0x31].revertAction->setEnabled(true);
                    // Image is modified and autosave is checked, so save the image (no need to lock it)  //
                    } else {
                        bool saved;
                        saved = img->save();
                        if (!saved) {
                            if (QMessageBox::question(this, tr("Save failed"), tr("'%1' cannot be saved, do you want to save the image with another name?")
                                .arg(img->originalFileName()), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
                                saveDiskAs(deviceNo);
                            }
                        } else {
                            diskWidgets[deviceNo - 0x31].saveAction->setEnabled(false);
                            diskWidgets[deviceNo - 0x31].revertAction->setEnabled(false);
                        }
                    }
                } else {
                    diskWidgets[deviceNo - 0x31].saveAction->setEnabled(false);
                    diskWidgets[deviceNo - 0x31].revertAction->setEnabled(false);
                }
            }
        } else {
#ifdef Q_OS_ANDROID
            // Empty slot: muted outline, no fill.
            diskWidgets[deviceNo - 0x31].frame->setStyleSheet(
                QString("QFrame#%1 { background:#F7F7F7; border:1px solid #B0B0B0; border-radius:6px; }")
                    .arg(diskWidgets[deviceNo - 0x31].frame->objectName()));
            // The eject button doubles as "remove slot" when empty: trash icon.
            diskWidgets[deviceNo - 0x31].ejectAction->setIcon(removeSlotIcon());
            diskWidgets[deviceNo - 0x31].ejectAction->setToolTip(tr("Remove slot"));
            diskWidgets[deviceNo - 0x31].ejectAction->setEnabled(true);
#else
            diskWidgets[deviceNo - 0x31].ejectAction->setEnabled(false);
#endif
            diskWidgets[deviceNo - 0x31].saveAction->setEnabled(false);
#ifdef Q_OS_ANDROID
            diskWidgets[deviceNo - 0x31].saveAction->setIcon(QIcon(":/icons/tango-icons/devices/media-floppy.svg"));
            diskWidgets[deviceNo - 0x31].saveAction->setToolTip(tr("Save disk"));
            // Empty slot: a muted call-to-action where the file name would be.
            diskWidgets[deviceNo - 0x31].fileNameLabel->setText(tr("Mount a disk image or folder."));
            diskWidgets[deviceNo - 0x31].fileNameLabel->setStyleSheet(
                "color:#B0B0B0; font-style:italic; font-weight:normal; font-size:12px;");
#else
            diskWidgets[deviceNo - 0x31].fileNameLabel->clear();
#endif
            diskWidgets[deviceNo - 0x31].imagePropertiesLabel->clear();
            diskWidgets[deviceNo - 0x31].revertAction->setEnabled(false);
            diskWidgets[deviceNo - 0x31].saveAsAction->setEnabled(false);
            diskWidgets[deviceNo - 0x31].editAction->setEnabled(false);
            diskWidgets[deviceNo - 0x31].editAction->setChecked(false);
            diskWidgets[deviceNo - 0x31].autoSaveAction->setEnabled(false);             //
            diskWidgets[deviceNo - 0x31].autoSaveAction->setChecked(false);             //
            if(deviceNo - 0x31 == 0)
                diskWidgets[deviceNo - 0x31].bootOptionAction->setEnabled(false);             //

        }
    }
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

void MainWindow::on_actionOptions_triggered()
{
    bool restart;
    restart = ui->actionStartEmulation->isChecked();
    if (restart) {
        ui->actionStartEmulation->trigger();
        sio->wait();
        qApp->processEvents();
    }
    OptionsDialog optionsDialog(this);
    optionsDialog.exec() ;

// Change drive slot description fonts
    changeFonts();

// load translators and retranslate
    loadTranslators();

// retranslate Designer Form
    ui->retranslateUi(this);

    for (int i = 0x31; i <= 0x36; i++) {    //
        deviceStatusChanged(i);
    }

    ui->actionStartEmulation->trigger();
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

void MainWindow::on_actionAbout_triggered()
{
    AboutDialog aboutDialog(this, VERSION);
    aboutDialog.exec();
}
//
void MainWindow::on_actionDocumentation_triggered()
{
    docDisplayWindow->show();
}

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
    // Folder image mounted from a SAF tree: write the temp working copy back to
    // the picked folder, then discard the temp dir.
    if (m_folderTree.contains(no)) {
        androidCopyDirToTree(m_folderTemp.value(no), m_folderTree.value(no));
        QDir(QFileInfo(m_folderTemp.value(no)).absolutePath()).removeRecursively();
        m_folderTree.remove(no);
        m_folderTemp.remove(no);
    }
#endif
    diskWidgets[no].ejectAction->setEnabled(false);
    QString fileName = diskWidgets[no].fileNameLabel->text();
    diskWidgets[no].fileNameLabel->clear();
    diskWidgets[no].writeProtectAction->setChecked(false);
    diskWidgets[no].writeProtectAction->setEnabled(false);
    diskWidgets[no].editAction->setEnabled(false);

    aspeqtSettings->unmountImage(no);
    updateRecentFileActions();
    deviceStatusChanged(no + 0x31);
    qDebug() << "!n" << tr("Unmounted disk %1").arg(no + 1);
    return true;
}

int MainWindow::containingDiskSlot(const QPoint &point)
{
    // Map through global coordinates so this works no matter how the slots are
    // nested (e.g. inside a scroll area). The old version compared against
    // frame->geometry() offset by the central widget, which ignored the scroll
    // position — so once the slot list was scrolled (slots 6+), the hit test
    // landed on the wrong slot.
    const QPoint global = mapToGlobal(point);
    for (int i = 0; i < m_numDisks; i++) {
        if (!diskWidgets[i].frame) continue;   // skip removed slots (gaps)
        QRect rect(diskWidgets[i].frame->mapToGlobal(QPoint(0, 0)),
                   diskWidgets[i].frame->size());
        if (rect.contains(global)) {
            return i;
        }
    }
    return -1;
}

int MainWindow::firstEmptyDiskSlot(int startFrom, bool createOne)
{
    int i;
    for (i = startFrom; i < m_numDisks; i++) {  //
        // Skip removed slots (gaps): they have no widgets, so nothing may mount
        // there.
        if (!diskWidgets[i].frame) {
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
            while (i >= 0 && !diskWidgets[i].frame) i--;
        } else {
            i = -1;
        }
    }
    emit newSlot(i);        //
    return i;
}

void MainWindow::bootExe(const QString &fileName)
{
    SioDevice *old = sio->getDevice(0x31);
    AutoBoot loader(sio, old);    
    AutoBootDialog dlg(this);
    if (!loader.open(fileName, aspeqtSettings->useHighSpeedExeLoader())) {
        return;
    }

    sio->uninstallDevice(0x31);
    sio->installDevice(0x31, &loader);
    connect(&loader, SIGNAL(booterStarted()), &dlg, SLOT(booterStarted()));
    connect(&loader, SIGNAL(booterLoaded()), &dlg, SLOT(booterLoaded()));
    connect(&loader, SIGNAL(blockRead(int, int)), &dlg, SLOT(blockRead(int, int)));
    connect(&loader, SIGNAL(loaderDone()), &dlg, SLOT(loaderDone()));
    connect(&dlg, SIGNAL(keepOpen()), this, SLOT(keepBootExeOpen()));

    dlg.exec();

    sio->uninstallDevice(0x31);
    if (old) {
        sio->installDevice(0x31, old);
        SimpleDiskImage *d = qobject_cast <SimpleDiskImage*> (old);
        d = qobject_cast <SimpleDiskImage*> (sio->getDevice(0x31));
    }
    if(!g_exefileName.isEmpty()) bootExe(g_exefileName);
}
// Make boot executable dialog persistant until it's manually closed //
void MainWindow::keepBootExeOpen()
{
    bootExe(g_exefileName);
}

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
            m_folderTemp[no] = fileName;
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

        diskWidgets[no].ejectAction->setEnabled(true);
        diskWidgets[no].editAction->setEnabled(true);
        diskWidgets[no].writeProtectAction->setChecked(disk->isReadOnly());
        diskWidgets[no].writeProtectAction->setEnabled(!disk->isUnmodifiable());

        diskWidgets[no].fileNameLabel->setText(fileName);

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
QString MainWindow::androidOpenUrl(const QString &caption, const QString &filter)
{
    QUrl u = QFileDialog::getOpenFileUrl(this, caption, QUrl(), filter);
    return u.isEmpty() ? QString() : androidContentUri(u);
}

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

void MainWindow::androidInstallDos(int no)
{
    QString folder = m_folderTemp.value(no);
    QString tree   = m_folderTree.value(no);
    if (folder.isEmpty() || !QDir(folder).exists()) {
        QMessageBox::warning(this, tr("Install DOS"), tr("This slot does not hold a mounted folder."));
        return;
    }
    if (QMessageBox::question(this, tr("Install DOS"),
            tr("Copy high-speed MyPicoDOS ($boot.bin + picodos.sys) into this folder? "
               "The Atari will then be able to boot DOS from it."),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;

    // Stage the two files so we can push exactly them to the real SAF folder
    // (rather than re-uploading the whole mounted folder).
    QString stage = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/dosinstall";
    QDir(stage).removeRecursively();
    QDir().mkpath(stage);

    auto put = [&](const QString &src, const QString &dstName) -> bool {
        const QFileDevice::Permissions rw = QFileDevice::ReadOwner | QFileDevice::WriteOwner
                                          | QFileDevice::ReadGroup | QFileDevice::ReadOther;
        QString a = folder + "/" + dstName;   // into the mounted (temp) folder
        QFile::remove(a);
        if (!QFile::copy(src, a))
            return false;
        QFile::setPermissions(a, rw);
        QString b = stage + "/" + dstName;    // into the push stage
        QFile::copy(src, b);
        QFile::setPermissions(b, rw);
        return true;
    };

    bool ok = put(":/dos/boot.bin", "$boot.bin") && put(":/dos/picodos.sys", "picodos.sys");
    // Write the files to the real folder now (not only on eject).
    if (ok && !tree.isEmpty())
        androidCopyDirToTree(stage, tree);
    QDir(stage).removeRecursively();

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

void MainWindow::mountDiskImage(int no)
{
    QString dir;
// Always mount from "last image dir" //
//    if (diskWidgets[no].fileNameLabel->text().isEmpty()) {
        dir = aspeqtSettings->lastDiskImageDir();
//    } else {
//        dir = QFileInfo(diskWidgets[no].fileNameLabel->text()).absolutePath();
//    }
#ifdef Q_OS_ANDROID
    // The SAF picker cannot filter by the Atari extensions (no MIME types), so
    // it shows every file. Validate the picked file's real type and reject the
    // ones that belong to other actions.
    QString fileName = androidOpenUrl(tr("Open a disk image"),
                                      tr("All Atari disk images (*.atr *.xfd *.pro);;All files (*)"));
    if (fileName.isEmpty()) {
        return;
    }
    fileName = androidReadablePath(fileName, no);
    {
        FileTypes::FileType t = FileTypes::getFileType(fileName);
        if (t == FileTypes::Xex || t == FileTypes::XexGz) {
            QMessageBox::information(this, tr("Not a disk image"),
                tr("This is an Atari executable, not a disk image.\nUse \"File / Boot Atari executable\" to run it."));
            return;
        }
        if (t == FileTypes::Cas || t == FileTypes::CasGz) {
            QMessageBox::information(this, tr("Not a disk image"),
                tr("This is a cassette image, not a disk image.\nUse \"File / Play cassette image\" to run it."));
            return;
        }
    }
#else
        QString fileName = QFileDialog::getOpenFileName(this,
                                                        tr("Open a disk image"),
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
    aspeqtSettings->setLastDiskImageDir(QFileInfo(fileName).absolutePath());
#endif

    mountFileWithDefaultProtection(no, fileName);
}

void MainWindow::mountFolderImage(int no)
{
    QString dir;
// Always mount from "last folder dir" //
    dir = aspeqtSettings->lastFolderImageDir();
#ifdef Q_OS_ANDROID
    // Pick a folder via SAF (ACTION_OPEN_DOCUMENT_TREE), copy its files into a
    // local temp dir and mount that; changes are written back on eject.
    QUrl treeUrl = QFileDialog::getExistingDirectoryUrl(this, tr("Open a folder image"), QUrl());
    QString tree = treeUrl.isEmpty() ? QString() : androidContentUri(treeUrl);
    if (tree.isEmpty()) {
        return;
    }
    androidTakePersistable(tree, true);
    QString folderName = androidTreeName(tree);
    if (folderName.isEmpty()) folderName = QStringLiteral("folder");
    QString base = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                 + "/foldermount/" + QString::number(no);
    QDir(base).removeRecursively();
    QString fileName = base + "/" + folderName;
    if (!QDir().mkpath(fileName)) {
        return;
    }
    if (androidCopyTreeToDir(tree, fileName) < 0) {
        QMessageBox::warning(this, tr("Folder image"), tr("Could not read the selected folder."));
        return;
    }
    m_folderTree[no] = tree;
    m_folderTemp[no] = fileName;
#else
    QString fileName = QFileDialog::getExistingDirectory(this, tr("Open a folder image"), dir);
    fileName = QDir::fromNativeSeparators(fileName);
    if (fileName.isEmpty()) {
        return;
    }
    aspeqtSettings->setLastFolderImageDir(fileName);
#endif
    mountFileWithDefaultProtection(no, fileName);
}

void MainWindow::toggleWriteProtection(int no)
{
    SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + 0x31));
    if (diskWidgets[no].writeProtectAction->isChecked()) {
        img->setReadOnly(true);
    } else {
        img->setReadOnly(false);
    }
    aspeqtSettings->setMountedImageSetting(no, diskWidgets[no].fileNameLabel->text(), diskWidgets[no].writeProtectAction->isChecked());
}

void MainWindow::openEditor(int no)
{
    SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + 0x31));
    if (img->editDialog()) {
        img->editDialog()->close();
    } else {
        DiskEditDialog *dlg = new DiskEditDialog();
        dlg->go(img);
        dlg->show();
    }
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
    switch (no)
    {
        case 0 :
        {
            if(ui->autoSave_1->isEnabled()) ui->autoSave_1->click();

        }
        break;
        case 1 :
        {
            if(ui->autoSave_2->isEnabled()) ui->autoSave_2->click();

        }
        break;
        case 2 :
        {
            if(ui->autoSave_3->isEnabled()) ui->autoSave_3->click();

        }
        break;
        case 3 :
        {
            if(ui->autoSave_4->isEnabled()) ui->autoSave_4->click();

        }
        break;
        case 4 :
        {
            if(ui->autoSave_5->isEnabled()) ui->autoSave_5->click();

        }
        break;
        case 5 :
        {
            if(ui->autoSave_6->isEnabled()) ui->autoSave_6->click();

        }
        break;
    }
}

void MainWindow::autoSaveDisk(int no)
{
    SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + 0x31));

    if (img->isUnnamed()) {
        saveDiskAs(no);
        diskWidgets[no].saveAction->setEnabled(false);
        diskWidgets[no].revertAction->setEnabled(false);
        return;
    }
    switch (no) {
        case 0 :
            if (ui->autoSave_1->isChecked()) {
                qDebug() << "!n" << tr("[Disk 1] Auto-commit ON.");
            } else {
                qDebug() << "!n" << tr("[Disk 1] Auto-commit OFF.");
              }
            break;

        case 1 :
            if (ui->autoSave_2->isChecked()) {
                qDebug() << "!n" << tr("[Disk 2] Auto-commit ON.");
            } else {
                qDebug() << "!n" << tr("[Disk 2] Auto-commit OFF.");
              }
            break;

        case 2 :
            if (ui->autoSave_3->isChecked()) {
                qDebug() << "!n" << tr("[Disk 3] Auto-commit ON.");
            } else {
                qDebug() << "!n" << tr("[Disk 3] Auto-commit OFF.");
              }
            break;

        case 3 :
            if (ui->autoSave_4->isChecked()) {
                qDebug() << "!n" << tr("[Disk 4] Auto-commit ON.");
            } else {
                qDebug() << "!n" << tr("[Disk 4] Auto-commit OFF.");
              }
            break;

        case 4 :
            if (ui->autoSave_5->isChecked()) {
                qDebug() << "!n" << tr("[Disk 5] Auto-commit ON.");
            } else {
                qDebug() << "!n" << tr("[Disk 5] Auto-commit OFF.");
              }
            break;

        case 5 :
            if (ui->autoSave_6->isChecked()) {
                qDebug() << "!n" << tr("[Disk 6] Auto-commit ON.");
            } else {
                qDebug() << "!n" << tr("[Disk 6] Auto-commit OFF.");
              }
            break;
    }

    bool saved;

    img->lock();
    saved = img->save();
    img->unlock();
    if (!saved) {
        if (QMessageBox::question(this, tr("Save failed"), tr("'%1' cannot be saved, do you want to save the image with another name?")
            .arg(img->originalFileName()), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
            saveDiskAs(no);
        }
    } else {
            diskWidgets[no].saveAction->setEnabled(false);
            diskWidgets[no].revertAction->setEnabled(false);

    }
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

void MainWindow::on_actionMountDisk_1_triggered() {mountDiskImage(0);}
void MainWindow::on_actionMountDisk_2_triggered() {mountDiskImage(1);}
void MainWindow::on_actionMountDisk_3_triggered() {mountDiskImage(2);}
void MainWindow::on_actionMountDisk_4_triggered() {mountDiskImage(3);}
void MainWindow::on_actionMountDisk_5_triggered() {mountDiskImage(4);}
void MainWindow::on_actionMountDisk_6_triggered() {mountDiskImage(5);}

void MainWindow::on_actionMountFolder_1_triggered() {mountFolderImage(0);}
void MainWindow::on_actionMountFolder_2_triggered() {mountFolderImage(1);}
void MainWindow::on_actionMountFolder_3_triggered() {mountFolderImage(2);}
void MainWindow::on_actionMountFolder_4_triggered() {mountFolderImage(3);}
void MainWindow::on_actionMountFolder_5_triggered() {mountFolderImage(4);}
void MainWindow::on_actionMountFolder_6_triggered() {mountFolderImage(5);}

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

void MainWindow::on_actionEditDisk_1_triggered() {openEditor(0);}
void MainWindow::on_actionEditDisk_2_triggered() {openEditor(1);}
void MainWindow::on_actionEditDisk_3_triggered() {openEditor(2);}
void MainWindow::on_actionEditDisk_4_triggered() {openEditor(3);}
void MainWindow::on_actionEditDisk_5_triggered() {openEditor(4);}
void MainWindow::on_actionEditDisk_6_triggered() {openEditor(5);}

void MainWindow::on_actionSave_1_triggered() {saveDisk(0);}
void MainWindow::on_actionSave_2_triggered() {saveDisk(1);}
void MainWindow::on_actionSave_3_triggered() {saveDisk(2);}
void MainWindow::on_actionSave_4_triggered() {saveDisk(3);}
void MainWindow::on_actionSave_5_triggered() {saveDisk(4);}
void MainWindow::on_actionSave_6_triggered() {saveDisk(5);}

//
void MainWindow::on_actionAutoSave_1_triggered() {autoSaveDisk(0);}
void MainWindow::on_actionAutoSave_2_triggered() {autoSaveDisk(1);}
void MainWindow::on_actionAutoSave_3_triggered() {autoSaveDisk(2);}
void MainWindow::on_actionAutoSave_4_triggered() {autoSaveDisk(3);}
void MainWindow::on_actionAutoSave_5_triggered() {autoSaveDisk(4);}
void MainWindow::on_actionAutoSave_6_triggered() {autoSaveDisk(5);}

void MainWindow::on_actionSaveAs_1_triggered() {saveDiskAs(0);}
void MainWindow::on_actionSaveAs_2_triggered() {saveDiskAs(1);}
void MainWindow::on_actionSaveAs_3_triggered() {saveDiskAs(2);}
void MainWindow::on_actionSaveAs_4_triggered() {saveDiskAs(3);}
void MainWindow::on_actionSaveAs_5_triggered() {saveDiskAs(4);}
void MainWindow::on_actionSaveAs_6_triggered() {saveDiskAs(5);}

void MainWindow::on_actionRevert_1_triggered() {revertDisk(0);}
void MainWindow::on_actionRevert_2_triggered() {revertDisk(1);}
void MainWindow::on_actionRevert_3_triggered() {revertDisk(2);}
void MainWindow::on_actionRevert_4_triggered() {revertDisk(3);}
void MainWindow::on_actionRevert_5_triggered() {revertDisk(4);}
void MainWindow::on_actionRevert_6_triggered() {revertDisk(5);}

void MainWindow::on_actionEjectAll_triggered()
{
    QMessageBox::StandardButton answer = QMessageBox::No;

    int toBeSaved = 0;

    for (int i = 0; i < m_numDisks; i++) {  //
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
void MainWindow::on_actionMountDisk_triggered()
{
    mountDiskImage(firstEmptyDiskSlot(0, true));
}

void MainWindow::on_actionMountFolder_triggered()
{
    mountFolderImage(firstEmptyDiskSlot(0, true));
}

void MainWindow::on_actionNewImage_triggered()
{
    CreateImageDialog dlg(this);
    if (!dlg.exec()) {
        return;
    };

    SimpleDiskImage *disk = new SimpleDiskImage(sio);
    connect(disk, SIGNAL(statusChanged(int)), this, SLOT(deviceStatusChanged(int)), Qt::QueuedConnection);

    if (!disk->create(++untitledName)) {
        delete disk;
        return;
    }

    DiskGeometry g;
    uint size = dlg.sectorCount() * dlg.sectorSize();
    if (dlg.sectorSize() == 256) {
        if (dlg.sectorCount() >= 3) {
            size -= 384;
        } else {
            size -= dlg.sectorCount() * 128;
        }
    }
    g.initialize(size, dlg.sectorSize());

    if (!disk->format(g)) {
        delete disk;
        return;
    }

    int no = firstEmptyDiskSlot(0, true);

    if (!ejectImage(no)) {
        delete disk;
        return;
    }

    sio->installDevice(0x31 + no, disk);
    deviceStatusChanged(0x31 + no);
    qDebug() << "!n" << tr("[%1] Mounted '%2' as '%3'.")
            .arg(disk->deviceName())
            .arg(friendlyName(disk->originalFileName()))
            .arg(disk->description());
}

void MainWindow::on_actionOpenSession_triggered()
{
    QString dir = aspeqtSettings->lastSessionDir();
    QString fileName;   // path QSettings can read
#ifdef Q_OS_ANDROID
    // QSettings can't read a content:// URI, so copy the picked session into a
    // local temp file and load QSettings from there. Keep tmp alive till the end.
    QTemporaryFile tmp;
    QString url = androidOpenUrl(tr("Open session"),
                                 tr("AspeQt sessions (*.aspeqt);;All files (*)"));
    if (url.isEmpty()) {
        return;
    }
    if (!tmp.open()) {
        return;
    }
    {
        QFile in(url);
        if (!in.open(QIODevice::ReadOnly)) {
            return;
        }
        tmp.write(in.readAll());
    }
    tmp.flush();
    tmp.close();
    fileName = tmp.fileName();
    g_sessionFile = friendlyName(url);
    g_sessionFilePath = QString();
#else
    fileName = QFileDialog::getOpenFileName(this, tr("Open session"),
                                 dir,
                                 tr(
                                         "AspeQt sessions (*.aspeqt);;"
                                         "All files (*)"));
    if (fileName.isEmpty()) {
        return;
    }
    aspeqtSettings->setLastSessionDir(QFileInfo(fileName).absolutePath());
    g_sessionFile = QFileInfo(fileName).fileName();
    g_sessionFilePath = QFileInfo(fileName).absolutePath();
#endif
// First eject existing images, then mount session images and restore mainwindow position and size //
    MainWindow::on_actionEjectAll_triggered();

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
void MainWindow::on_actionSaveSession_triggered()
{
    QString dir = aspeqtSettings->lastSessionDir();
#ifdef Q_OS_ANDROID
    QString url = androidSaveUrl(tr("Save session as"),
                                 tr("AspeQt sessions (*.aspeqt);;All files (*)"));
    if (url.isEmpty()) {
        return;
    }
#else
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save session as"),
                                 dir,
                                 tr(
                                         "AspeQt sessions (*.aspeqt);;"
                                         "All files (*)"));
    if (fileName.isEmpty()) {
        return;
    }
    aspeqtSettings->setLastSessionDir(QFileInfo(fileName).absolutePath());
#endif

// Save mainwindow position and size to session file //
    if (aspeqtSettings->saveWindowsPos()) {
        aspeqtSettings->setLastHorizontalPos(geometry().x());
        aspeqtSettings->setLastVerticalPos(geometry().y());
        aspeqtSettings->setLastWidth(geometry().width());
        aspeqtSettings->setLastHeight(geometry().height());
    }
#ifdef Q_OS_ANDROID
    // QSettings needs a real path: write to a temp file, then copy the bytes to
    // the SAF content:// target.
    QTemporaryFile tmp;
    if (!tmp.open()) {
        return;
    }
    QString tmpPath = tmp.fileName();
    tmp.close();
    aspeqtSettings->saveSessionToFile(tmpPath);
    QFile in(tmpPath), out(url);
    if (in.open(QIODevice::ReadOnly) && out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        out.write(in.readAll());
    }
#else
    aspeqtSettings->saveSessionToFile(fileName);
#endif
}

void MainWindow::on_actionBootExe_triggered()
{
    QString dir = aspeqtSettings->lastExeDir();
    #ifdef Q_OS_ANDROID
    g_exefileName = androidOpenUrl(tr("Open executable"),
                                   tr("Atari executables (*.xex *.com *.exe);;All files (*)"));
    if (g_exefileName.isEmpty()) {
        return;
    }
    // QFile (used by the boot loader) can't reliably read some SAF content://
    // URIs; always copy to a temp file first.
    g_exefileName = androidLocalCopy(g_exefileName);
    if (g_exefileName.isEmpty())
        return;
    {
        FileTypes::FileType t = FileTypes::getFileType(g_exefileName);
        if (t != FileTypes::Xex && t != FileTypes::XexGz) {
            QMessageBox::information(this, tr("Not an executable"),
                tr("This is not an Atari executable.\nExecutables start with $FFFF; pick a .xex/.com/.exe file."));
            g_exefileName.clear();
            return;
        }
    }
    #else
    g_exefileName = QFileDialog::getOpenFileName(this, tr("Open executable"),
                                 dir,
                                 tr(
                                         "Atari executables (*.xex *.com *.exe);;"
                                         "All files (*)"));
    if (g_exefileName.isEmpty()) {
        return;
    }
    aspeqtSettings->setLastExeDir(QFileInfo(g_exefileName).absolutePath());
    #endif
    bootExe(g_exefileName);
}

void MainWindow::on_actionShowPrinterTextOutput_triggered()
{
    if (ui->actionShowPrinterTextOutput->isChecked()) {
        textPrinterWindow->setGeometry(aspeqtSettings->lastPrtHorizontalPos() ,aspeqtSettings->lastPrtVerticalPos(),aspeqtSettings->lastPrtWidth(),aspeqtSettings->lastPrtHeight());
        textPrinterWindow->show();
    } else {
        textPrinterWindow->hide();
    }
}

void MainWindow::textPrinterWindowClosed()
{
    ui->actionShowPrinterTextOutput->setChecked(false);
}

void MainWindow::on_actionPlaybackCassette_triggered()
{
    QString dir = aspeqtSettings->lastCasDir();
    QString fileName = NULL;
    #ifdef Q_OS_ANDROID
    fileName = androidOpenUrl(tr("Open a cassette image"),
                              tr("CAS images (*.cas);;All files (*)"));
    if (fileName.isEmpty()) {
        return;
    }
    // QFile (used by CassetteWorker) can't reliably read some SAF content://
    // URIs; always copy to a temp file first.
    fileName = androidLocalCopy(fileName);
    if (fileName.isEmpty())
        return;
    {
        FileTypes::FileType t = FileTypes::getFileType(fileName);
        if (t != FileTypes::Cas && t != FileTypes::CasGz) {
            QMessageBox::information(this, tr("Not a cassette image"),
                tr("This is not a cassette image.\nPick a .cas file."));
            return;
        }
    }
    #else
    fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open a cassette image"),
                                                    aspeqtSettings->lastCasDir(),
                                                    tr(
                                                    "CAS images (*.cas);;"
                                                    "All files (*)"));
    if (fileName.isEmpty()) {
        return;
    }
    aspeqtSettings->setLastCasDir(QFileInfo(fileName).absolutePath());
    #endif

    bool restart;
    restart = ui->actionStartEmulation->isChecked();
    if (restart) {
        ui->actionStartEmulation->trigger();
        sio->wait();
        qApp->processEvents();
    }

    CassetteDialog *dlg = new CassetteDialog(this, fileName, friendlyName(fileName));
    dlg->exec();
    delete dlg;

    if (restart) {
        ui->actionStartEmulation->trigger();
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

void MainWindow::folderPath(int slot)
{
   emit takeFolderPath(diskWidgets[slot].fileNameLabel->statusTip());
}

void MainWindow::on_actionBootOption_triggered()
{
    BootOptionsDialog bod(this);
    connect(&bod, SIGNAL(giveFolderPath(int)), this, SLOT(folderPath(int)));
    connect(this, SIGNAL(takeFolderPath(QString)), &bod, SLOT(folderPath(QString)));
    bod.exec();
}
