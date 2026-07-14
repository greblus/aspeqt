#ifndef FOLDERIMAGE_H
#define FOLDERIMAGE_H

#include <QDir>
#include <QVector>
#include <QMap>
#include "diskimage.h"

class ContentFile;

class AtariFile
{
public:
    bool exists;
    QString source;      // real path (desktop) or content:// child URI (Android)
    qint64 size;
    QString atariName;
    QString atariExt;
    QString longName;
    int lastSector;
    quint64 pos;
    int sectPass;
};

class FolderImage : public SimpleDiskImage
{
    Q_OBJECT

protected:
    QDir dir;
    QString m_tree;              // Android: the SAF tree content:// URI
    bool mReadOnly;
    void buildDirectory();
    AtariFile atariFiles[64];
    int atariFileNo;             //

    // One folder entry: display name, its openable source (path or child URI)
    // and byte size. buildDirectory() turns these into the 64 mirrored slots.
    struct Entry { QString name; QString source; qint64 size; };
    QVector<Entry> listFolder();
    // Openable source for a by-name helper file ($boot.bin, piconame.txt, ...).
    // create=true finds-or-creates it (Android SAF); on desktop it is just a path.
    QString nameSource(const QString &name, bool create);
    QMap<QString, QString> m_nameCache;   // Android: name -> resolved source URI
    // Cached read handle for the file currently being served sector by sector,
    // so we don't reopen it (an fd/ContentResolver round-trip) every 125 bytes.
    ContentFile *m_openFile = nullptr;
    int m_openFileNo = -1;
    ContentFile *dataFile(int fileNo);

public:
    FolderImage(SioWorker *worker): SimpleDiskImage(worker) {}
    ~FolderImage();

    void close();
    bool open(const QString &fileName, FileTypes::FileType /* type */);
    bool readSector(quint16 sector, QByteArray &data);
    bool writeSector(quint16 sector, const QByteArray &data);
    bool format(quint16 aSectorCount, quint16 aSectorSize);
    QString longName (QString &lastMountedFolder, QString &atariFileName);   // 
    QString description() const {return tr("Folder image");}
};
    extern FolderImage *folderImage;

#endif // FOLDERIMAGE_H
