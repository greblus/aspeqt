#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>
#include <QFileDialog>
#include <QMessageBox>
#include <QMap>
#include <QtDebug>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QProgressBar>
#include <QTranslator>
#include <QSystemTrayIcon>
#include <QTextEdit>

#include "optionsdialog.h"
#include "aboutdialog.h"
#include "createimagedialog.h"
#include "diskeditdialog.h"
#include "serialport.h"
#include "sioworker.h"
#include "textprinterwindow.h"
#include "docdisplaywindow.h"

#define g_numberOfDisks 6      // desktop: fixed number of drive slots

#ifdef Q_OS_ANDROID
#define MAX_DISKS 15           // SIO disk device numbers 0x31..0x3F
#define DEFAULT_DISKS 5
#else
#define MAX_DISKS g_numberOfDisks
#endif

namespace Ui
{
    class MainWindow;
}
class AutoBoot;
class AtariFileSystem;
class SimpleDiskImage;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
    QString g_sessionFile;
    QString g_sessionFilePath;
    QString g_mainWindowTitle;

public slots:
    void show();
    int firstEmptyDiskSlot(int startFrom = 0, bool createOne = true);       //
    void mountFileWithDefaultProtection(int no, const QString &fileName);   //
    void autoCommit(int no);                                                //
    void folderPath(int slot);                                              //

private:
    int untitledName;
    Ui::MainWindow *ui;
    SioWorker *sio;
    bool shownFirstTime;
    int m_numDisks;                        // active number of drive slots
    bool m_emulationRunning = false;       // SIO worker running (mirrored to QML)

    // Per-slot runtime state, moved out of the widgets (DiskWidgets) so the
    // engine no longer needs them. Presence used to be "diskWidgets[i].frame".
    bool m_slotPresent[MAX_DISKS] = {};
    bool m_autoCommit[MAX_DISKS] = {};
    bool m_writeProtect[MAX_DISKS] = {};

    // QML disk viewer state (entries are re-fetched per call, not stored)
    AtariFileSystem *m_dvFs = nullptr;
    SimpleDiskImage *m_dvDisk = nullptr;
    int m_dvFsType = 0;                    // current filesystem type (0..5)
    QList<quint16> m_dvDirs;               // directory sector stack
    QStringList m_dvPaths;                 // directory name stack
    QLabel *speedLabel, *onOffLabel, *prtOnOffLabel, *netLabel, *clearMessagesLabel;  //
    TextPrinterWindow *textPrinterWindow;
    DocDisplayWindow *docDisplayWindow;    //
    QTranslator aspeqt_translator, aspeqt_qt_translator;
    QSystemTrayIcon trayIcon;
    Qt::WindowFlags oldWindowFlags;
    Qt::WindowStates oldWindowStates;
    QString lastMessage;
    int lastMessageRepeat;
    
    void setSession();  //
    void updateRecentFileActions();
    void bootExe(const QString &fileName);
    void mountFile(int no, const QString &fileName, bool prot);
    void mountDiskImage(int no);
    void mountFolderImage(int no);
    // Short, human-readable name for logs/labels: the file's base name, or the
    // ContentResolver display name for a content:// URI.
    QString friendlyName(const QString &name);
#ifdef Q_OS_ANDROID
    // Storage Access Framework pickers: return a content:// URI string (empty
    // if cancelled). QFile opens these directly, so no storage permission is
    // needed. Replaces the old filesystem-browsing Java dialog.
    QString androidOpenUrl(const QString &caption, const QString &filter);
    QString androidSaveUrl(const QString &caption, const QString &filter);
    // Human-readable name of a content:// URI (via ContentResolver), for labels.
    QString androidDisplayName(const QString &uri);
    // Persist access to a content:// URI so it stays usable after a restart.
    void androidTakePersistable(const QString &uri, bool write);
    // Folder images: a SAF tree can't be read as a path, so it is copied to a
    // local temp dir for mounting and copied back on eject.
    QString androidTreeName(const QString &tree);
    int androidCopyTreeToDir(const QString &tree, const QString &dest);
    int androidCopyDirToTree(const QString &src, const QString &tree);
    int androidCopyUriToFile(const QString &uri, const QString &dest);
    // Copy the bundled high-speed MyPicoDOS ($boot.bin + picodos.sys) into a
    // mounted folder so the Atari can boot DOS from it.
    void androidInstallDos(int no);
    // Find-or-create a document by name in a SAF tree; returns its content:// URI.
    QString androidChildOrCreate(const QString &tree, const QString &name);
    // Return a path QFile can read: the content:// URI itself when Qt can open
    // it, otherwise a temp copy made via ContentResolver (Qt's QFile fails on
    // some SAF URIs, e.g. files in sub-folders). Keeps the real file name.
    QString androidReadablePath(const QString &uri, int slot);
    // Always copy a content:// URI to a local temp file (returns the path, or
    // empty on failure). For read-only loads (CAS/executable) where QFile's SAF
    // stream can pass open() but fail the repeated reads/seeks/atEnd() they need.
    QString androidLocalCopy(const QString &uri);
    QMap<int, QString> m_folderTree;   // slot -> tree content:// URI
    QMap<int, QString> m_folderTemp;   // slot -> local temp working dir

    // --- dynamic drive slots (Android) --------------------------------------
    // Slot presence lives in m_slotPresent[]; add fills the lowest gap (or
    // appends), remove drops one leaving a numbering gap. Persists in the session.
    void androidRebuildSlots();        // (re)populate present slots from settings

    // --- top loader slot: inline XEX autoboot / CAS cassette player ----------
    QString       m_loaderFile;             // current local (temp) file path
    int           m_loaderKind = 0;         // 0 none, 1 xex, 2 cas
    double        m_loaderFill = 0.0;       // last progress-fill fraction (for QML)
    QString       m_loaderName;             // display name shown by the QML loader
    QString       m_loaderTypeText;         // e.g. "Executable (24k)" / "Cassette (1:23)"
    CassetteWorker *m_casWorker = nullptr;
    QTimer       *m_casTimer = nullptr;
    int           m_casTotal = 0, m_casRemaining = 0;
    bool          m_casWasRunning = false;   // emulation paused for cassette play
    AutoBoot     *m_autoBoot = nullptr;
    SioDevice    *m_autoBootOld = nullptr;
    void loaderLoad();
    void loaderLoadXex(const QString &path);
    void loaderLoadCas(const QString &path);
    void loaderPlayCas();
    void loaderEject();
    void loaderRetry();
    void loaderUpdateButtons();
    void loaderSetFill(double frac);   // fill the whole slot as a progress bar
    void loaderCasStatus(int remainingTime);
    void loaderCasTick();
    void loaderCasFinished();
    void loaderBlockRead(int current, int all);
    void loaderBooterDone();
    void androidAddSlot();             // "+" -> fill the lowest gap / append
    void androidRemoveSlot(int i);     // 2nd eject on empty -> drop this slot
    void androidEjectPressed(int i);   // eject if mounted, else remove the slot
#endif
    bool ejectImage(int no, bool ask = true);
    void toggleWriteProtection(int no);
    void openEditor(int no);
    void saveDisk(int no);
    void saveDiskAs(int no);
    void revertDisk(int no);
    QMessageBox::StandardButton saveImageWhenClosing(int no, QMessageBox::StandardButton previousAnswer, int number);
    void loadTranslators();
    void autoSaveDisk(int no);                                              //

protected:
    void closeEvent(QCloseEvent *event);
    void hideEvent(QHideEvent *event);
    bool eventFilter(QObject *obj, QEvent *event);
#ifdef Q_OS_ANDROID
#endif

signals:
    void logMessage(int type, const QString &msg);
    void newSlot (int slot);
    void fileMounted(bool mounted);
    void takeFolderPath (QString fPath);
    void sendLogText (QString logText);
    void sendLogTextChange (QString logTextChange);

public:
    void doLogMessage(int type, const QString &msg);

#ifdef ASPEQT_QML
    // --- bridge for the QML UI (branch `qml`) -------------------------------
    // MainWindow runs headless (never shown) as the emulation engine; the QML
    // AppController drives it through these wrappers and mirrors its state via
    // the qml*() readers, refreshing whenever qmlChanged() fires.
public:
    QVariantList qmlDriveList();     // one map per present drive slot
    QVariantMap  qmlLoaderState();   // loader (XEX/CAS) slot
    QVariantMap  qmlStatus();        // { running, speed, printerOn }
    bool         qmlCanAddSlot();

    void qmlMountDisk(int i);
    void qmlMountFolder(int i);
    void qmlEjectPressed(int i);
    void qmlSave(int i);
    void qmlToggleAutoCommit(int i);
    void qmlEdit(int i);
    void qmlToggleWriteProtect(int i);
    int qmlAddSlot();    // returns the hardware index of the added slot (-1 none)
    void qmlSwapSlots(int source, int slot);   // drag-reorder: swap two drives
    void qmlBootOptions();
    void qmlLoaderLoad();
    void qmlLoaderPlay();
    void qmlLoaderRetry();
    void qmlLoaderEject();
    void qmlToggleSio();
    void qmlTogglePrinter();
    void qmlClearLog();
    // menu items (mirror the QtWidgets menu bar)
    void qmlNewImage();
    void qmlCreateDisk(int sectorCount, int sectorSize);
    void qmlMountDiskAny();
    void qmlMountFolderAny();
    void qmlEjectAll();
    void qmlShowPrinterOutput();
    QString qmlPrinterText();
    QString qmlPrinterTextAtascii();
    void    qmlPrinterClear();
    void    qmlPrinterSave();
    void qmlOpenSession();
    void qmlSaveSession();
    void qmlOptions();
    void qmlLogWindow();
    void qmlQuit();
    QStringList qmlRecentFiles();
    void qmlMountRecent(int index);
    // options window
    QVariantMap  qmlLoadOptions();
    void         qmlApplyOptions(const QVariantMap &o);
    QVariantList qmlLanguages();
    // disk viewer/editor
    bool         qmlDiskOpen(int hwIndex);
    void         qmlDiskClose();
    QVariantList qmlDiskEntries();
    QString      qmlDiskPath();
    bool         qmlDiskCanParent();
    bool         qmlDiskReadOnly();
    int          qmlDiskFsType();
    void         qmlDiskSetFsType(int index);
    void         qmlDiskEnter(int row);
    void         qmlDiskParent();
    void         qmlDiskSetTextConversion(bool on);
    bool         qmlDiskExtract(const QVariantList &rows);
    bool         qmlDiskDelete(const QVariantList &rows);
    bool         qmlDiskAddFiles();
signals:
    void qmlChanged();
    void qmlLoaderProgress();   // frequent, loader-only (progress fill)
    void qmlPrinterTextChanged();
private:
#endif

private slots:
    void on_actionPlaybackCassette_triggered();
    void on_actionShowPrinterTextOutput_triggered();
    void on_actionBootExe_triggered();
    void on_actionSaveSession_triggered();
    void on_actionOpenSession_triggered();
    void on_actionNewImage_triggered();
    void on_actionMountFolder_triggered();
    void on_actionMountDisk_triggered();
    void on_actionEjectAll_triggered();
    void on_actionOptions_triggered();
    void on_actionStartEmulation_triggered();
    void on_actionPrinterEmulation_triggered();
    void on_actionQuit_triggered();
    void on_actionAbout_triggered();
    void on_actionDocumentation_triggered();
    void on_actionMountDisk_1_triggered();
    void on_actionMountDisk_2_triggered();
    void on_actionMountDisk_3_triggered();
    void on_actionMountDisk_4_triggered();
    void on_actionMountDisk_5_triggered();
    void on_actionMountDisk_6_triggered();

    void on_actionMountFolder_1_triggered();
    void on_actionMountFolder_2_triggered();
    void on_actionMountFolder_3_triggered();
    void on_actionMountFolder_4_triggered();
    void on_actionMountFolder_5_triggered();
    void on_actionMountFolder_6_triggered();

    void on_actionEject_1_triggered();
    void on_actionEject_2_triggered();
    void on_actionEject_3_triggered();
    void on_actionEject_4_triggered();
    void on_actionEject_5_triggered();
    void on_actionEject_6_triggered();

    void on_actionWriteProtect_1_triggered();
    void on_actionWriteProtect_2_triggered();
    void on_actionWriteProtect_3_triggered();
    void on_actionWriteProtect_4_triggered();
    void on_actionWriteProtect_5_triggered();
    void on_actionWriteProtect_6_triggered();

    void on_actionMountRecent_0_triggered();
    void on_actionMountRecent_1_triggered();
    void on_actionMountRecent_2_triggered();
    void on_actionMountRecent_3_triggered();
    void on_actionMountRecent_4_triggered();
    void on_actionMountRecent_5_triggered();
    void on_actionMountRecent_6_triggered();
    void on_actionMountRecent_7_triggered();
    void on_actionMountRecent_8_triggered();
    void on_actionMountRecent_9_triggered();

    void on_actionEditDisk_1_triggered();
    void on_actionEditDisk_2_triggered();
    void on_actionEditDisk_3_triggered();
    void on_actionEditDisk_4_triggered();
    void on_actionEditDisk_5_triggered();
    void on_actionEditDisk_6_triggered();

    void on_actionSave_1_triggered();
    void on_actionSave_2_triggered();
    void on_actionSave_3_triggered();
    void on_actionSave_4_triggered();
    void on_actionSave_5_triggered();
    void on_actionSave_6_triggered();

    void on_actionAutoSave_1_triggered();
    void on_actionAutoSave_2_triggered();
    void on_actionAutoSave_3_triggered();
    void on_actionAutoSave_4_triggered();
    void on_actionAutoSave_5_triggered();
    void on_actionAutoSave_6_triggered();

    void on_actionSaveAs_1_triggered();
    void on_actionSaveAs_2_triggered();
    void on_actionSaveAs_3_triggered();
    void on_actionSaveAs_4_triggered();
    void on_actionSaveAs_5_triggered();
    void on_actionSaveAs_6_triggered();

    void on_actionRevert_1_triggered();
    void on_actionRevert_2_triggered();
    void on_actionRevert_3_triggered();
    void on_actionRevert_4_triggered();
    void on_actionRevert_5_triggered();
    void on_actionRevert_6_triggered();

    void on_actionBootOption_triggered();
    void on_actionToggleMiniMode_triggered();
    void on_actionToggleShade_triggered();
    void on_actionLogWindow_triggered();
    void sioFinished();
    void sioStarted();
    void sioStatusChanged(QString status);
    void textPrinterWindowClosed();
    void deviceStatusChanged(int deviceNo);
    void uiMessage(int t, const QString message);
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void keepBootExeOpen();
    void saveWindowGeometry();
    void saveMiniWindowGeometry();
    void logChanged(QString text);
    void changeFonts();
};

#endif // MAINWINDOW_H
