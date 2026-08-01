#include "miscutils.h"

#include <QDir>
#include <QFile>
#include <QDebug>
#include <QUrl>
#include <QStringList>
#ifdef Q_OS_ANDROID
#include <QJniObject>
#include <QJniEnvironment>
#endif

#ifdef Q_OS_ANDROID
// content:// -> real file descriptor, via SerialActivity.openFd. Returns -1 on
// failure. The caller owns the fd (QFile::AutoCloseHandle closes it).
static int androidOpenFd(const QString &uri, const char *mode)
{
    QJniObject juri = QJniObject::fromString(uri);
    QJniObject jmode = QJniObject::fromString(QString::fromLatin1(mode));
    return QJniObject::callStaticMethod<jint>(
        "net/greblus/SerialActivity", "openFd",
        "(Ljava/lang/String;Ljava/lang/String;)I",
        juri.object<jstring>(), jmode.object<jstring>());
}

static const char *fdModeFor(QIODevice::OpenMode m)
{
    if (m & QIODevice::WriteOnly)
        return (m & QIODevice::ReadOnly) ? "rw" : "wt";
    return "r";
}
#endif

ContentFile::ContentFile(const QString &name)
    : QFile(name), m_name(name)
{
}

bool ContentFile::open(OpenMode mode)
{
#ifdef Q_OS_ANDROID
    if (m_name.startsWith(QLatin1String("content:"))) {
        int fd = androidOpenFd(m_name, fdModeFor(mode));
        if (fd < 0)
            return false;
        return QFile::open(fd, mode, QFile::AutoCloseHandle);
    }
#endif
    return QFile::open(mode);
}

bool writeWholeFile(const QString &target, const QByteArray &bytes)
{
    if (target.isEmpty())
        return false;
#ifdef Q_OS_ANDROID
    if (target.startsWith(QLatin1String("content:"))) {
        QJniEnvironment env;
        jbyteArray arr = env->NewByteArray(bytes.size());
        if (!arr)
            return false;
        env->SetByteArrayRegion(arr, 0, bytes.size(),
                                reinterpret_cast<const jbyte *>(bytes.constData()));
        QJniObject juri = QJniObject::fromString(target);
        const jint written = QJniObject::callStaticMethod<jint>(
            "net/greblus/SerialActivity", "writeUriBytes", "(Ljava/lang/String;[B)I",
            juri.object<jstring>(), arr);
        env->DeleteLocalRef(arr);
        return written == bytes.size();
    }
#endif
    QFile f(target);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    const bool ok = f.write(bytes) == bytes.size() && f.flush();
    f.close();
    return ok;
}

QString androidContentUri(const QUrl &url)
{
    if (url.scheme() != QLatin1String("content"))
        return url.toString(QUrl::FullyEncoded);

    QString out = QStringLiteral("content://") + url.authority();
    // Component-decode gives the raw id characters (%2F -> /, %3A -> :, %20 ->
    // space); re-encode each id segment as one component below.
    QString rest = url.path(QUrl::FullyDecoded);   // /document/<id> or /tree/<id>/document/<id>

    const QStringList markers = { QStringLiteral("/document/"), QStringLiteral("/tree/") };
    while (!rest.isEmpty()) {
        QString marker;
        for (const QString &m : markers)
            if (rest.startsWith(m)) { marker = m; break; }
        // Android's Uri.encode leaves these "mark" chars un-encoded; excluding
        // them reproduces the provider's exact document id (otherwise e.g.
        // "(" -> %28 makes ContentResolver/QFile miss the file).
        static const QByteArray keep = "!'()*";
        if (marker.isEmpty()) {                // unexpected shape: encode as-is
            out += QString::fromUtf8(QUrl::toPercentEncoding(rest, "/" + keep));
            break;
        }
        QString after = rest.mid(marker.size());
        int nextIdx = -1;                      // start of the next id segment
        for (const QString &m : markers) {
            int p = after.indexOf(m);
            if (p >= 0 && (nextIdx < 0 || p < nextIdx))
                nextIdx = p;
        }
        QString id = (nextIdx >= 0) ? after.left(nextIdx) : after;
        out += marker + QString::fromUtf8(QUrl::toPercentEncoding(id, keep));
        rest = (nextIdx >= 0) ? after.mid(nextIdx) : QString();
    }
    return out;
}

void deltree(const QString &name)
{
    QFileInfo info(name);

    if (info.isDir()) {
        QDir dir(name);
        QFileInfoList list = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files);
        foreach (QFileInfo file, list) {
            deltree(file.absoluteFilePath());
        }
        QString n = dir.dirName();
        dir.cdUp();
        dir.rmdir(n);
    } else {
        QFile::remove(name);
    }
}

/* FileTypes */

FileTypes::FileType FileTypes::getFileType(const QString &fileName)
{
    QByteArray header;
    FileType result = Unknown;

    /* Check if it is a folder */

#ifdef Q_OS_ANDROID
    // A SAF folder pick (ACTION_OPEN_DOCUMENT_TREE) is a bare tree URI with no
    // /document/ segment; treat it as a folder image. A single-file pick is a
    // /document/ URI and falls through to header sniffing.
    if (fileName.startsWith(QLatin1String("content:"))
            && fileName.contains(QLatin1String("/tree/"))
            && !fileName.contains(QLatin1String("/document/"))) {
        return Dir;
    }
#endif

    if (QFileInfo(fileName).isDir()) {
        return Dir;
    }

    /* Read the file header */
    {
        ContentFile file(fileName);
        if (file.open(QFile::ReadOnly)) {
            header = file.read(4);
        }
        while (header.count() < 4) {
            header.append('\x0');
        }
    }

    /* Check if the file is gzipped */

    bool gz = false;

    if ((quint8)header.at(0) == 0x1f && (quint8)header.at(1) == 0x8b ) {
        /* The file is gzipped, read the real header */
        gz = true;
        GzFile file(fileName);
        if (file.open(QFile::ReadOnly)) {
            header = file.read(4);
        } else {
            header = QByteArray(4, 0);
        }
        while (header.count() < 4) {
            header.append('\x0');
        }
    }

    quint8 b0 = header.at(0);
    quint8 b1 = header.at(1);
    quint8 b2 = header.at(2);
    quint8 b3 = header.at(3);

    /* Determine the file type */

    if (b0 == 'A' && b1 == 'T' && b2 == '8' && b3 == 'X') {
        result = Atx;
    } else if (b0 == 'F' && b1 == 'U' && b2 == 'J' && b3 == 'I') {
        result = Cas;
    } else if (b0 == 0x96 && b1 == 0x02) {
        result = Atr;
    } else if (b0 == 0xF9 || b0 == 0xFA) {
        result = Dcm;
    } else if (b0 == 0xFF && b1 == 0xFF) {
        result = Xex;
    } else if (b0 == 0xFD && b1 == 0xFD) {
        result = Scp;
    } else if (b0 == 'D' && b1 == 'I') {
        result = Di;
    } else if (b2 == 'P' && (b3 == '2' || b3 == '3')) {
        result = Pro;
    } else if (fileName.endsWith(".XFD", Qt::CaseInsensitive) || fileName.endsWith(".XFZ", Qt::CaseInsensitive) || fileName.endsWith(".XFD.GZ", Qt::CaseInsensitive)) {
        result = Xfd;
    }

    if (result != Unknown && gz) {
        result = (FileType) (result + 1);
    }

    return result;
}

QString FileTypes::getFileTypeName(FileType type)
{
    switch (type) {
    case Atr:
        return tr("ATR disk image");
    case AtrGz:
        return tr("gzipped ATR disk image");
    case Xfd:
        return tr("XFD disk image");
    case XfdGz:
        return tr("gziped XFD disk image");
    case Dcm:
        return tr("DCM disk image");
    case DcmGz:
        return tr("gzipped DCM disk image");
    case Scp:
        return tr("SCP disk image");
    case ScpGz:
        return tr("gzipped SCP disk image");
    case Di:
        return tr("DI disk image");
    case DiGz:
        return tr("gzipped DI disk image");
    case Pro:
        return tr("PRO disk image");
    case ProGz:
        return tr("gzipped PRO disk image");
    case Atx:
        return tr("VAPI (ATX) disk image");
    case AtxGz:
        return tr("gzipped VAPI (ATX) disk image");
    case Cas:
        return tr("CAS cassette image");
    case CasGz:
        return tr("gzipped CAS cassette image");
    case Xex:
        return tr("Atari executable");
    case XexGz:
        return tr("gzipped Atari executable");
    default:
        return tr("unknown file type");
    }
}

/* GzFile */

GzFile::GzFile(const QString &path)
    :ContentFile(path)
{
    mPath = path;
    mHandle = 0;
}

GzFile::~GzFile()
{
    close();
}

bool GzFile::open(OpenMode mode)
{
    if (ContentFile::open(mode)) {
        if((mode & ReadOnly) == ReadOnly) {
            mHandle = gzdopen(handle(), "rb");
        }  else if((mode & ReadWrite) == ReadWrite) {
            mHandle = gzdopen(handle(), "wb+");
        } else if((mode & WriteOnly) == WriteOnly) {
            mHandle = gzdopen(handle(), "wb");
        }

        if(mHandle == NULL) {
            setErrorString(tr("gzdopen() failed."));
            return false;
        }
    } else {
        return false;
    }
    return true;
}

void GzFile::close()
{
    if (mHandle) {
        gzclose(mHandle);
    }
    QFile::close();
    mHandle = NULL;
}

bool GzFile::isSequential() const
{
    return true;
}

bool GzFile::seek(qint64 pos)
{
    bool result = gzseek(mHandle, pos, SEEK_SET) != -1;
    if (!result) {
        setErrorString(tr("gzseek() failed."));
    }
    return result;
}

qint64 GzFile::readData(char *data, qint64 maxSize)
{
    if (mHandle == NULL) {
        return 0;
    }

    return gzread(mHandle, data, maxSize);
}

qint64 GzFile::writeData(const char *data, qint64 maxSize)
{
    if (mHandle == NULL) {
        return 0;
    }

    return gzwrite(mHandle, data, maxSize);
}

bool GzFile::atEnd() const
{
    return gzeof(mHandle);
}
