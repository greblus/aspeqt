#include "folderimage.h"
#include "aspeqtsettings.h"
#include "miscutils.h"

#include <QFileInfoList>
#include <QtDebug>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <algorithm>
#ifdef Q_OS_ANDROID
#include <QJniObject>
#endif

#ifdef Q_OS_ANDROID
// content:// tree helpers, implemented in net/greblus/SerialActivity.
static QString androidTreeCall(const char *method, const QString &tree, const QString &arg = QString())
{
    QJniObject jt = QJniObject::fromString(tree);
    if (arg.isNull()) {
        QJniObject r = QJniObject::callStaticObjectMethod(
            "net/greblus/SerialActivity", method,
            "(Ljava/lang/String;)Ljava/lang/String;", jt.object<jstring>());
        return r.isValid() ? r.toString() : QString();
    }
    QJniObject ja = QJniObject::fromString(arg);
    QJniObject r = QJniObject::callStaticObjectMethod(
        "net/greblus/SerialActivity", method,
        "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;",
        jt.object<jstring>(), ja.object<jstring>());
    return r.isValid() ? r.toString() : QString();
}
#endif


// CIRCULAR SECTORS USED FOR SERVING FILES FROM FOLDER IMAGES
// ==========================================================
// Circular sectors per file logic utilizes all sectors from 433 to 1023 for a total of 591 sectors.
// Sector number will cycle back to 433 once it hits 1023, and this cycle will repeat until the entire file is read.
// The same pool of sectors are used for every file in the Folder Image.
// First sector number of each file however is selected from a different pool of (369-432) so that they are unique for each file.
// This allows for dynamic calculation of the Atari file number within the code.
// Sector numbers (5, 6, 32-134) are reserved for SpartaDos boot process.

extern QString g_aspeQtAppPath;
extern bool g_disablePicoHiSpeed;

FolderImage::~FolderImage()
{
    close();
}

void FolderImage::close()
{
    for (int i = 0; i < 64; i++) {
        atariFiles[i].exists = false;
    }
    if (m_openFile) {
        m_openFile->close();
        delete m_openFile;
        m_openFile = nullptr;
    }
    m_openFileNo = -1;
    return;
}

// Enumerate the folder's flat file list: on Android from the SAF tree (name +
// child URI + size), on the desktop from the QDir. Sorted by name to match the
// old QDir::Name ordering.
QVector<FolderImage::Entry> FolderImage::listFolder()
{
    QVector<Entry> out;
#ifdef Q_OS_ANDROID
    if (m_tree.startsWith(QLatin1String("content:"))) {
        const QString listing = androidTreeCall("listTree", m_tree);
        const QStringList lines = listing.split('\n', Qt::SkipEmptyParts);
        for (const QString &ln : lines) {
            const QStringList f = ln.split('\t');
            if (f.size() < 3)
                continue;
            out.append({ f.at(0), f.at(1), f.at(2).toLongLong() });
        }
        std::sort(out.begin(), out.end(), [](const Entry &a, const Entry &b) {
            return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
        });
        return out;
    }
#endif
    const QFileInfoList infos = dir.entryInfoList(QDir::Files, QDir::Name);
    for (const QFileInfo &fi : infos)
        out.append({ fi.fileName(), fi.absoluteFilePath(), fi.size() });
    return out;
}

// Openable source for a helper file kept next to the mirrored files. On the
// desktop that is just a path; on Android it is the tree child URI (created when
// create=true, e.g. for the piconame.txt/x32.dos markers a DOS mount writes).
QString FolderImage::nameSource(const QString &name, bool create)
{
#ifdef Q_OS_ANDROID
    if (m_tree.startsWith(QLatin1String("content:"))) {
        auto it = m_nameCache.constFind(name);
        if (it != m_nameCache.constEnd() && !it.value().isEmpty())
            return it.value();
        QString src = androidTreeCall(create ? "ensureInTree" : "childInTree", m_tree, name);
        if (!src.isEmpty())
            m_nameCache.insert(name, src);
        return src;
    }
#else
    Q_UNUSED(create)
#endif
    return dir.path() + "/" + name;
}

// Cached read handle for the file served sector by sector (opened once per file).
ContentFile *FolderImage::dataFile(int fileNo)
{
    if (m_openFileNo == fileNo && m_openFile && m_openFile->isOpen())
        return m_openFile;
    if (m_openFile) {
        m_openFile->close();
        delete m_openFile;
        m_openFile = nullptr;
    }
    m_openFileNo = -1;
    if (fileNo < 0 || fileNo >= 64 || !atariFiles[fileNo].exists)
        return nullptr;
    m_openFile = new ContentFile(atariFiles[fileNo].source);
    if (!m_openFile->open(QFile::ReadOnly)) {
        delete m_openFile;
        m_openFile = nullptr;
        return nullptr;
    }
#ifdef Q_OS_ANDROID
    // A cloud provider (Google Drive) hands back a *non-seekable* descriptor.
    // Sector reads seek before every read, and that seek silently failed: as
    // long as the Atari read strictly forward the bytes happened to line up,
    // but the first retried sector returned the *next* 125 bytes instead of the
    // same ones, the file stream desynchronised and DOS hung. Serve such a
    // document from a local copy instead -- the first read pays for the
    // download, everything after it is a plain file.
    if (atariFiles[fileNo].source.startsWith(QLatin1String("content:"))
            && !m_openFile->seek(0)) {
        m_openFile->close();
        delete m_openFile;
        m_openFile = nullptr;
        const QString local = cachedCopy(atariFiles[fileNo].source,
                                         atariFiles[fileNo].longName);
        if (local.isEmpty())
            return nullptr;
        m_openFile = new ContentFile(local);
        if (!m_openFile->open(QFile::ReadOnly)) {
            delete m_openFile;
            m_openFile = nullptr;
            return nullptr;
        }
    }
#endif
    m_openFileNo = fileNo;
    return m_openFile;
}

#ifdef Q_OS_ANDROID
// Local cache copy of a SAF document, made once per mount. Named after a hash of
// the document URI so two files with the same name cannot collide.
QString FolderImage::cachedCopy(const QString &uri, const QString &name)
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                      + QStringLiteral("/foldercache");
    QDir().mkpath(dir);
    const QString tag = QString::fromLatin1(
        QCryptographicHash::hash(uri.toUtf8(), QCryptographicHash::Sha1).toHex().left(12));
    QString safe = name;
    safe.remove(QRegularExpression("[^A-Za-z0-9._-]"));
    const QString path = dir + "/" + tag + "-" + safe;
    if (QFileInfo::exists(path))
        return path;

    QJniObject ju = QJniObject::fromString(uri);
    QJniObject jp = QJniObject::fromString(path);
    const jint n = QJniObject::callStaticMethod<jint>(
        "net/greblus/SerialActivity", "copyUriToFile",
        "(Ljava/lang/String;Ljava/lang/String;)I",
        ju.object<jstring>(), jp.object<jstring>());
    if (n < 0) {
        QFile::remove(path);
        return QString();
    }
    qDebug() << "!d" << QString("[FolderImage] cached '%1' (%2 B) from a non-seekable provider")
                        .arg(name).arg(int(n));
    return path;
}
#endif

bool FolderImage::format(quint16, quint16)
{
    return false;
}

// Return the long file name of a short Atari file name from a given (last mounted) Folder Image

QString FolderImage::longName(QString &lastMountedFolder, QString &atariFileName)
{
    if (FolderImage::open(lastMountedFolder, FileTypes::Dir)) {
        for (int i = 0; i < 64; i++) {
            if(atariFiles[i].atariName + "." + atariFiles[i].atariExt == atariFileName)
                return atariFiles[i].longName;
        }
     }
     return NULL;
}
void FolderImage::buildDirectory()
{
    m_nameCache.clear();
    QVector<Entry> infos = listFolder();
    QString name, longName;
    QString ext;

    int j = -1, k, i;
    for (i = 0; i < 64; i++) {
        Entry entry;
        do {
            j++;
            if (j >= infos.count()) {
                atariFiles[i].exists = false;
                break;
            }
            entry = infos.at(j);
            QFileInfo info(entry.name);   // parse base/suffix only, no I/O
            longName = info.completeBaseName();
            name = longName.toUpper();
            if(aspeqtSettings->filterUnderscore()) {
                name.remove(QRegularExpression("[^A-Z0-9]"));
            } else {
                name.remove(QRegularExpression("[^A-Z0-9_]"));
            }
            name = name.left(8);
            if (name.isEmpty()) {
                name = "BADNAME";
            }
            longName += "." + info.suffix();
            ext = info.suffix().toUpper();
            if(aspeqtSettings->filterUnderscore()) {
                ext.remove(QRegularExpression("[^A-Z0-9]"));
            } else {
                ext.remove(QRegularExpression("[^A-Z0-9_]"));
            }
            ext = ext.left(3);
            QString baseName = name.left(7);

            int l = 2;
            do {
                for (k = 0; k < i; k++) {
                    if (atariFiles[k].atariName == name && atariFiles[k].atariExt == ext) {
                        break;
                    }
                }
                if (k < i) {
                    name = QString("%1%2").arg(baseName).arg(l);
                    l++;
                }
            } while (k < i && l < 10000000);
            if (l == 10) {baseName = name.left(6);}
            if (l == 100) {baseName = name.left(5);}
            if (l == 1000) {baseName = name.left(4);}
            if (l == 10000) {baseName = name.left(3);}
            if (l == 100000) {baseName = name.left(2);}
            if (l == 1000000) {baseName = name.left(1);}
            if (l == 10000000) {baseName = "";}
            if (l == 100000000) {
                qWarning() << "!w" << tr("Cannot mirror '%1' in '%2': No suitable Atari name can be found.")
                               .arg(info.fileName())
                               .arg(dir.path());
            }
        } while (k < i);

        if (j >= infos.count()) {
            break;
        }

        atariFiles[i].exists = true;
        atariFiles[i].source = entry.source;
        atariFiles[i].size = entry.size;
        atariFiles[i].atariName = name;
        atariFiles[i].longName = longName;
        atariFiles[i].atariExt = ext;
        atariFiles[i].lastSector = 0;
        atariFiles[i].pos = 0;
        atariFiles[i].sectPass = 0;
    }

    if (i < infos.count()) {
        qWarning() << "!w" << tr("Cannot mirror %1 of %2 files in '%3': Atari directory is full.")
                       .arg(infos.count() - i)
                       .arg(infos.count())
                       .arg(dir.path());
    }
}

bool FolderImage::open(const QString &fileName, FileTypes::FileType /* type */)
{
#ifdef Q_OS_ANDROID
    // A SAF tree URI has no filesystem path; mount it directly (files are read
    // in place through content:// descriptors, nothing is copied).
    if (fileName.startsWith(QLatin1String("content:"))) {
        m_tree = fileName;
    } else
#endif
    if (dir.exists(fileName)) {
        dir.setPath(fileName);
    } else {
        return false;
    }

    buildDirectory();

    m_originalFileName = fileName;
    m_geometry.initialize(false, 40, 26, 128);
    m_newGeometry.initialize(m_geometry);
    m_isReadOnly = true;
    m_isModified = false;
    m_isUnmodifiable = true;
    return true;
}

// Force a buffer to exactly one sector's worth of bytes. Every caller below
// writes at fixed offsets (data[15], data[125]...), and QByteArray::operator[]
// does no bounds checking -- a short read of $boot.bin (an empty stub left by a
// provider that accepted the create but swallowed the write, as Google Drive
// does) used to walk straight off the end and take the app down with SIGSEGV.
// Qt 6's resize() leaves the added bytes uninitialised, hence the explicit fill.
static void padSector(QByteArray &d, int size)
{
    if (d.size() > size)
        d.truncate(size);
    else if (d.size() < size)
        d.append(QByteArray(size - d.size(), '\0'));
}

bool FolderImage::readSector(quint16 sector, QByteArray &data)
{
    /* Boot */

    ContentFile boot(nameSource("$boot.bin", false));
    data = QByteArray(128, 0);
    int bootFileSector;

    if (sector == 1) {
         if (!boot.open(QFile::ReadOnly)) {
             data[1] = 0x01;
             data[3] = 0x07;
             data[4] = 0x40;
             data[5] = 0x15;
             data[6] = 0x4c;
             data[7] = 0x14;
             data[8] = 0x07;     // JMP 0x0714
             data[0x14] = 0x38;  // SEC
             data[0x15] = 0x60;  // RTS
         } else {
             data = boot.read(128);
             padSector(data, 128);
             buildDirectory();
             for(int i=0; i<64; i++) {
                 // AtariDOS, MyDos, SmartDOS  and DosXL
                 if(atariFiles[i].longName.toUpper() == "DOS.SYS") {
                     bootFileSector = 369 + i;
                     data[15] = bootFileSector % 256;
                     data[16] = bootFileSector / 256;
                     break;
                 }
                 // MyPicoDOS
                 if(atariFiles[i].longName.toUpper() == "PICODOS.SYS") {
                     bootFileSector = 369 + i;
                     if(g_disablePicoHiSpeed) {
                         data[15] = 0;
                         ContentFile boot(nameSource("$boot.bin", false));
                         QByteArray speed;
                         boot.open(QFile::ReadWrite);
                         boot.seek(15);
                         speed = boot.read(1);
                         if (speed.isEmpty()) speed = QByteArray(1, 0);   // Qt6: no auto-grow
                         speed[0] = '\x30';
                         boot.seek(15);
                         boot.write(speed);
                         boot.close();
                     }
                     data[9] = bootFileSector % 256;
                     data[10] = bootFileSector / 256;
                     // Build piconame.txt in full and write it in one call: a
                     // streaming write through a file descriptor is what left a
                     // 0-byte file on providers that refuse a truncating open.
                     QString folderLabel = dir.dirName();
#ifdef Q_OS_ANDROID
                     if (m_tree.startsWith(QLatin1String("content:")))
                         folderLabel = androidTreeCall("treeDisplayName", m_tree);
#endif
                     QByteArray picoData;
                     picoData.append(folderLabel.toStdString());
                     picoData.append('\x9b');
                     for(int i=0; i<64; i++){
                     if(atariFiles[i].exists) {
                         if(atariFiles[i].longName != "$boot.bin") {
                                 picoData.append(atariFiles[i].atariName.toStdString());
                                 QByteArray space(qMax(0, 8 - (int)atariFiles[i].atariName.size()), '\x20');
                                 picoData.append(space.toStdString());
                                 picoData.append(atariFiles[i].atariExt.toStdString());
                                 picoData.append('\x20');
                                 picoData.append(atariFiles[i].longName.mid(0, atariFiles[i].longName.indexOf(".", -1)-1).toStdString());
                                 picoData.append('\x9B');
                         }
                      } else {
                             break;
                      }
                     }
                     writeWholeFile(nameSource("piconame.txt", true), picoData);
                     break;
                 }
                 // SpartaDOS, force it to change to AtariDOS format after the boot
                 if(atariFiles[i].longName.toUpper() == "X32.DOS") {
                     ContentFile x32Dos(nameSource("x32.dos", false));
                     x32Dos.open(QFile::ReadOnly);
                     QByteArray flag;
                     flag = x32Dos.readAll();
                     if(!flag.isEmpty() && flag.at(0) == '\xFF') {
                         data[1] = 0x01;
                         data[3] = 0x07;
                         data[4] = 0x40;
                         data[5] = 0x15;
                         data[6] = 0x4c;
                         data[7] = 0x14;
                         data[8] = 0x07;
                         data[0x14] = 0x38;
                         data[0x15] = 0x60;
                     }
                   break;
                 }
             }
         }
         return true;
    }
    if (sector == 2) {
        boot.open(QFile::ReadOnly);
        boot.seek(128);
        data = boot.read(128);
        padSector(data, 128);
        return true;
    }
    if (sector == 3) {
        boot.open(QFile::ReadOnly);
        boot.seek(256);
        data = boot.read(128);
        padSector(data, 128);
        return true;
    }
    // SpartaDOS Boot
    if ((sector >= 32 && sector <= 134) ||
        sector == 5 || sector == 6) {
        boot.open(QFile::ReadOnly);
        boot.seek((sector-1)*128);
        data = boot.read(128);
        if(sector == 134) {
            ContentFile x32Dos(nameSource("x32.dos", true));
            x32Dos.open(QFile::ReadWrite);
            QByteArray flag;
            flag = x32Dos.readAll();
            if(!flag.isEmpty() && flag.at(0) == '\x00') {
                flag[0] = '\xFF';
                x32Dos.seek(0);
                x32Dos.write(flag);
                x32Dos.close();
            }
        }
        return true;
    }

    /* VTOC */
    if (sector == 360) {
        data = QByteArray(128, 0);
        data[0] = 2;
        data[1] = 1010 % 256;
        data[2] = 1010 / 256;
        data[10] = 0x7F;
        for (int i = 11; i < 100; i++) {
            data[i] = 0xff;
        }
        return true;
    }

    /* Directory sectors */
    if (sector >= 361 && sector <=368) {
        if (sector == 361) {
            buildDirectory();
        }
        data.resize(0);
        for (int i = (sector - 361) * 8; i < (sector - 360) * 8; i++) {
            QByteArray entry;
            if (!atariFiles[i].exists) {
                entry = QByteArray(16, 0);
            } else {
                // Qt 6: operator[] no longer grows the array, so pre-size the
                // 5 header bytes before indexing (name/ext are appended after).
                entry = QByteArray(5, 0);
                entry[0] = 0x42;
                int size = (atariFiles[i].size + 124) / 125;
                if (size > 999) {
                    size = 999;
                }
                entry[1] = size % 256;
                entry[2] = size / 256;
                int first = 369 + i;
                entry[3] = first % 256;
                entry[4] = first / 256;
                entry += atariFiles[i].atariName.toLatin1();
                while (entry.count() < 13) {
                    entry += 32;
                }
                entry += atariFiles[i].atariExt.toLatin1();
                while (entry.count() < 16) {
                    entry += 32;
                }
            }
            data += entry;
        }
        return true;
    }

    /* Data sectors */

    /* First sector of the file */
        int size, next;
        if  (sector >= 369 && sector <= 432) {
            atariFileNo = sector - 369;
            if (!atariFiles[atariFileNo].exists) {
                data = QByteArray(128, 0);
                return true;
            }
            ContentFile *file = dataFile(atariFileNo);
            if (!file) {
                data = QByteArray(128, 0);
                return true;
            }
            file->seek(0);
            data = file->read(125);
            size = data.size();
            padSector(data, 128);
            if (file->atEnd()) {
                next = 0;
            }
            else {
                next = 433;
            }
            data[125] = (atariFileNo * 4) | (next / 256);
            data[126] = next % 256;
            data[127] = size;
            return true;
        }

    /* Rest of the file sectors */
        if ((sector >= 433 && sector <= 1023)) {
            if (atariFileNo < 0 || atariFileNo >= 64 || !atariFiles[atariFileNo].exists) {
                data = QByteArray(128, 0);
                return true;
            }
            ContentFile *file = dataFile(atariFileNo);
            if (!file) {
                data = QByteArray(128, 0);
                return true;
            }
	    atariFiles[atariFileNo].pos = (125+((sector-433)*125))+(atariFiles[atariFileNo].sectPass*73875);
            file->seek(atariFiles[atariFileNo].pos);
            data = file->read(125);
            next = sector + 1;
            if (sector == 1023) {
                next = 433;
                atariFiles[atariFileNo].sectPass += 1;
	    }
            size = data.size();
            padSector(data, 128);
            atariFiles[atariFileNo].lastSector = sector;
            if (file->atEnd()) next = 0;
            data[125] = (atariFileNo * 4) | (next / 256);
            data[126] = next % 256;
            data[127] = size;
            return true;
        }

    /* Any other sector */

        data = QByteArray(128, 0);
        return true;
}

bool FolderImage::writeSector(quint16, const QByteArray &)
{
    return false;
}
