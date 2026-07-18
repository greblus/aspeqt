#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QObject>
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

#include "diskeditdialog.h"
#include "serialport.h"
#include "sioworker.h"
#include "textprinterwindow.h"

#define g_numberOfDisks 6      // desktop: fixed number of drive slots

#ifdef Q_OS_ANDROID
#define MAX_DISKS 15           // SIO disk device numbers 0x31..0x3F
#define DEFAULT_DISKS 5
#else
#define MAX_DISKS g_numberOfDisks
#endif

class AutoBoot;
class AtariFileSystem;
class SimpleDiskImage;

class MainWindow : public QObject
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
    QString g_sessionFile;
    QString g_sessionFilePath;
    QString g_mainWindowTitle;

public slots:
    int firstEmptyDiskSlot(int startFrom = 0, bool createOne = true);       //
    void mountFileWithDefaultProtection(int no, const QString &fileName);   //
    void autoCommit(int no);                                                //

private:
    int untitledName;
    SioWorker *sio;
    bool shownFirstTime;
    int m_numDisks;                        // active number of drive slots
    bool m_emulationRunning = false;       // SIO worker running (mirrored to QML)
    QString m_sioStatus;                   // connection speed shown in the status bar

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
    TextPrinterWindow *textPrinterWindow;
    QTranslator aspeqt_translator, aspeqt_qt_translator;
    
    void setSession();  //
    void mountFile(int no, const QString &fileName, bool prot);
    // Short, human-readable name for logs/labels: the file's base name, or the
    // ContentResolver display name for a content:// URI.
    QString friendlyName(const QString &name);
#ifdef Q_OS_ANDROID
    // Storage Access Framework pickers: return a content:// URI string (empty
    // if cancelled). QFile opens these directly, so no storage permission is
    // needed. Replaces the old filesystem-browsing Java dialog.
    // Human-readable name of a content:// URI (via ContentResolver), for labels.
    QString androidDisplayName(const QString &uri);
    // Persist access to a content:// URI so it stays usable after a restart.
    void androidTakePersistable(const QString &uri, bool write);
    // Folder images: a SAF tree can't be read as a path, so it is copied to a
    // local temp dir for mounting and copied back on eject.
    int androidCopyTreeToDir(const QString &tree, const QString &dest);
    int androidCopyDirToTree(const QString &src, const QString &tree);
    int androidCopyUriToFile(const QString &uri, const QString &dest);
    // Copy the bundled high-speed MyPicoDOS ($boot.bin + picodos.sys) into a
    // mounted folder so the Atari can boot DOS from it.
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
    QMessageBox::StandardButton saveImageWhenClosing(int no, QMessageBox::StandardButton previousAnswer, int number);
    void loadTranslators();
    void autoSaveDisk(int no);                                              //

protected:
#ifdef Q_OS_ANDROID
#endif

signals:
    void logMessage(int type, const QString &msg);
    void newSlot (int slot);
    void fileMounted(bool mounted);

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

    void qmlEjectPressed(int i);
    void qmlToggleWriteProtect(int i);
    int qmlAddSlot();    // returns the hardware index of the added slot (-1 none)
    void qmlSwapSlots(int source, int slot);   // drag-reorder: swap two drives
    void qmlLoaderPlay();
    void qmlLoaderRetry();
    void qmlLoaderEject();
    void qmlToggleSio();
    void qmlTogglePrinter();
    void qmlClearLog();
    // menu items (mirror the QtWidgets menu bar)
    void qmlCreateDisk(int sectorCount, int sectorSize);
    void qmlEjectAll();
    QString qmlPrinterText();
    QString qmlPrinterTextAtascii();
    void    qmlPrinterClear();
    void    qmlPrinterSave();
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
    bool         qmlDiskSetFsType(int index);
    // Called from JNI when a SAF pick finishes (empty uri = cancelled).
    Q_INVOKABLE void documentPicked(int reqId, const QString &uri);
    Q_INVOKABLE void pickDocument(int reqId, const QString &mimeType);
    Q_INVOKABLE void createDocument(int reqId, const QString &mimeType, const QString &suggestedName);
    Q_INVOKABLE void pickFolder(int reqId);

    QString      qmlStartDir(const QString &kind);
    void         qmlMountDiskPath(int no, const QString &url);
    void         qmlMountFolderPath(int no, const QString &url);
    void         qmlLoaderLoadPath(const QString &url);
    void         qmlOpenSessionPath(const QString &url);
    void         qmlSaveSessionPath(const QString &url);
    // Saving cannot ask the user from engine code any more (a modal dialog here
    // blocks the SIO path in a nested event loop), so it reports back instead
    // and QML drives the file picker / message.
    enum SaveResult { SaveOk = 0, SaveNeedsName = 1, SaveFailed = 2 };
    int          qmlSaveDisk(int no);
    int          qmlToggleAutoCommitDisk(int no);
    bool         qmlSaveAsPath(int no, const QString &url);
    void         qmlInstallDos(int no);
    void         qmlToast(const QString &text);
    bool         shutdown();   // false = user cancelled quitting
    void         qmlDiskEnter(int row);
    void         qmlDiskParent();
    void         qmlDiskSetTextConversion(bool on);
    bool         qmlDiskExtractPath(const QVariantList &rows, const QString &url);
    bool         qmlDiskDelete(const QVariantList &rows);
    bool         qmlDiskAddFilesPath(const QString &url);
signals:
    void qmlChanged();
    void qmlLoaderProgress();   // frequent, loader-only (progress fill)
    void qmlPrinterTextChanged();
    void qmlDocumentPicked(int reqId, const QString &uri);
private:
#endif

private slots:
    void on_actionQuit_triggered();

    void sioFinished();
    void sioStarted();
    void sioStatusChanged(QString status);
    void deviceStatusChanged(int deviceNo);
};

#endif // MAINWINDOW_H
