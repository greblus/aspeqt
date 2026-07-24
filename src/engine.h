#ifndef ENGINE_H
#define ENGINE_H

#include <QObject>
#include <QMap>
#include <QtDebug>
#include <QTimer>
#include <QTranslator>

#include "serialport.h"
#include "sioworker.h"
#include "printeroutput.h"
#include "rdevice.h"

#define MAX_DISKS 15           // SIO disk device numbers 0x31..0x3F
#define DEFAULT_DISKS 5        // slots a fresh session starts with

class AutoBoot;
class AtariFileSystem;
class SimpleDiskImage;

class Engine : public QObject
{
    Q_OBJECT

public:
    Engine(QObject *parent = nullptr);
    ~Engine();
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
    PrinterOutput *printerOutput = nullptr;
    RDevice *m_rDevice = nullptr;
    QTranslator aspeqt_translator, aspeqt_qt_translator;
    
    void setSession();  //
    void mountFile(int no, const QString &fileName, bool prot);
    // Short, human-readable name for logs/labels: the file's base name, or the
    // ContentResolver display name for a content:// URI.
    QString friendlyName(const QString &name);
    // SAF helpers that platform-independent code calls; outside Android they
    // fall through (a plain path is already readable, a name is already a name).
    QString androidDisplayName(const QString &uri);
    QString androidLocalCopy(const QString &uri);
    int     androidCopyDirToTree(const QString &src, const QString &tree);
#ifdef Q_OS_ANDROID
    // Storage Access Framework pickers: return a content:// URI string (empty
    // if cancelled). QFile opens these directly, so no storage permission is
    // needed. Replaces the old filesystem-browsing Java dialog.
    // Human-readable name of a content:// URI (via ContentResolver), for labels.
    // Persist access to a content:// URI so it stays usable after a restart.
    void androidTakePersistable(const QString &uri, bool write);
    // Folder images: a SAF tree can't be read as a path, so it is copied to a
    // local temp dir for mounting and copied back on eject.
    int androidCopyTreeToDir(const QString &tree, const QString &dest);

    int androidCopyUriToFile(const QString &uri, const QString &dest);
    // Copy the bundled high-speed MyPicoDOS ($boot.bin + picodos.sys) into a
    // mounted folder so the Atari can boot DOS from it.
    // Find-or-create a document by name in a SAF tree; returns its content:// URI.
    QString androidChildOrCreate(const QString &tree, const QString &name);
    // Return a path QFile can read: the content:// URI itself when Qt can open
    // it, otherwise a temp copy made via ContentResolver (Qt's QFile fails on
    // some SAF URIs, e.g. files in sub-folders). Keeps the real file name.
    QString androidReadablePath(const QString &uri, int slot);

    QMap<int, QString> m_folderTree;   // slot -> tree content:// URI
    QMap<int, QString> m_folderTemp;   // slot -> local temp working dir
    QMap<int, QString> m_netTempPath;  // slot -> cache file backing a network mount
#endif

    // --- dynamic drive slots ------------------------------------------------
    // Slot presence lives in m_slotPresent[]; add fills the lowest gap (or
    // appends), remove drops one leaving a numbering gap. Persists in the session.
    void rebuildSlots();               // (re)populate present slots from settings

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
    void loaderUpdateButtons();
    void loaderSetFill(double frac);   // fill the whole slot as a progress bar
    void loaderCasStatus(int remainingTime);
    void loaderCasTick();
    void loaderCasFinished();
    void loaderBlockRead(int current, int all);
    void loaderBooterDone();
    void addSlotAt();                  // "+" -> fill the lowest gap / append
    void removeSlot(int i);            // 2nd eject on empty -> drop this slot
    void ejectPressedAt(int i);        // eject if mounted, else remove the slot
    void ejectImage(int no);
    void toggleWriteProtection(int no);
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

    // --- state read by the QML UI through AppController ---------------------
    // AppController drives the engine through the actions below and mirrors
    // its state via the readers, refreshing whenever stateChanged() fires.
public:
    QVariantList driveList();
    QVariantList modifiedDisks();     // one map per present drive slot
    // Network-browser images are mounted from a throwaway cache file; they are
    // kept out of the saved session and cleaned up on eject unless saved.
    void mountNetworkTemp(int no, const QString &path);
    bool isNetworkTempSlot(int no) const { return m_netTempPath.contains(no); }
    QString netTempName(int no) const;
    void clearNetworkTemp(int no);
    QVariantList networkTempDisks() const;
    QVariantMap  loaderState();   // loader (XEX/CAS) slot
    QVariantMap  status();        // { running, speed, printerOn }
    bool         canAddSlot();

    void ejectPressed(int i);
    void toggleWriteProtect(int i);
    int addSlot();    // returns the hardware index of the added slot (-1 none)
    void swapSlots(int source, int slot);   // drag-reorder: swap two drives
    void loaderPlay();
    void toggleSio();
    void togglePrinter();
    void clearLog();
    // menu items (mirror the QtWidgets menu bar)
    void createDisk(int sectorCount, int sectorSize);
    void ejectAll();
    QString printerText();
    QString printerTextAtascii();
    void    printerClear();
    bool    printerSavePath(const QString &url, bool asPdf);
    void quit();
    QStringList recentFiles();
    void mountRecent(int index);
    // options window
    QVariantMap  loadOptions();
    void         applyOptions(const QVariantMap &o);
    QVariantList languages();

    // BBS phonebook for the R: device. Entries are maps of
    // name/ip/port/protocol/login/password, read from and written to the XML
    // file at aspeqtSettings->phonebookPath().
    QVariantList phonebookEntries();
    bool         phonebookSave(const QVariantList &list);
    QString      phonebookPath();
    QString      phonebookUseBundled();
    QString      phonebookBundledPath();
    void         phonebookDial(const QVariantMap &entry);
    // disk viewer/editor
    bool         diskOpen(int hwIndex);
    void         diskClose();
    QVariantList diskEntries();
    QString      diskPath();
    bool         diskCanParent();
    bool         diskReadOnly();
    int          diskFsType();
    bool         diskSetFsType(int index);
    // Called from JNI when a SAF pick finishes (empty uri = cancelled).
    Q_INVOKABLE void onDocumentPicked(int reqId, const QString &uri);
    Q_INVOKABLE void pickDocument(int reqId, const QString &mimeType);
    Q_INVOKABLE void createDocument(int reqId, const QString &mimeType, const QString &suggestedName);
    Q_INVOKABLE void pickFolder(int reqId);

    QString      startDir(const QString &kind);
    void         mountDiskPath(int no, const QString &url);
    void         mountFolderPath(int no, const QString &url);
    void         loaderLoadPath(const QString &url);
    void         openSessionPath(const QString &url);
    void         saveSessionPath(const QString &url);
    // Saving cannot ask the user from engine code any more (a modal dialog here
    // blocks the SIO path in a nested event loop), so it reports back instead
    // and QML drives the file picker / message.
    enum SaveResult { SaveOk = 0, SaveNeedsName = 1, SaveFailed = 2 };
    int          saveDisk(int no);
    int          toggleAutoCommitDisk(int no);
    bool         saveAsPath(int no, const QString &url);
    void         installDos(int no);
    QByteArray   readBundled(const QString &resource);
#ifdef Q_OS_ANDROID
    bool         writeIntoTree(const QString &tree, const QString &name, const QByteArray &bytes);
#else
    bool         writeIntoDir(const QString &dir, const QString &name, const QByteArray &bytes);
#endif
    void         toast(const QString &text);
    void         loaderRetry();
    void         loaderEject();
    void         shutdown();
    void         diskEnter(int row);
    void         diskParent();
    void         diskSetTextConversion(bool on);
    bool         diskExtractPath(const QVariantList &rows, const QString &url);
    bool         diskDelete(const QVariantList &rows);
    bool         diskAddFilesPath(const QString &url);
signals:
    void stateChanged();
    void loaderProgress();   // frequent, loader-only (progress fill)
    void printerTextChanged();
    void documentPicked(int reqId, const QString &uri);
private:
#endif

private slots:
    void on_actionQuit_triggered();

    void sioFinished();
    void sioStarted();
    void sioStatusChanged(QString status);
    void deviceStatusChanged(int deviceNo);
};

