#ifndef MISCUTILS_H
#define MISCUTILS_H

#include <QString>
#include <QFile>
#include "zlib.h"

void deltree(const QString &name);

class QUrl;
// Rebuild a SAF content:// URI string that ContentResolver/QFile can open:
// re-encode each document/tree id segment (%2F, %3A, %20, ...) that QUrl
// pretty-decodes when it parses the picker result. Without this, files in
// sub-folders or with special characters fail to open.
QString androidContentUri(const QUrl &url);

// A QFile that opens a SAF content:// URI through a real file descriptor
// (ContentResolver.openFileDescriptor via SerialActivity.openFd), so the picked
// file is read/written in place with no copy into app storage. For a normal
// path it behaves exactly like QFile. On non-Android builds it is a plain QFile.
class ContentFile : public QFile
{
    Q_OBJECT

public:
    explicit ContentFile(const QString &name);
    bool open(OpenMode mode) override;

private:
    // The name exactly as given. QFile's Android file engine re-encodes a
    // content:// URI (double-encodes parens, mangles %2F), so we must open the
    // descriptor from this untouched string, not from fileName().
    QString m_name;
};

class GzFile : public ContentFile
{
    Q_OBJECT

public:
    GzFile(const QString& path);
    ~GzFile();

    bool open(OpenMode mode);
    void close();
    bool seek(qint64 pos);
    bool isSequential () const;
    bool atEnd() const;

protected:
    gzFile mHandle;
    QString mPath;
    qint64 readData(char *data, qint64 maxSize);
    qint64 writeData(const char *data, qint64 maxSize);
};

class FileTypes : public QObject
{
    Q_OBJECT

public:
    enum FileType {
        Unknown,
        Dir,
        Atr, AtrGz,
        Xfd, XfdGz,
        Dcm, DcmGz,
        Scp, ScpGz,
        Di, DiGz,
        Pro, ProGz,
        Atx, AtxGz,
        Cas, CasGz,
        Xex, XexGz
    };
    static FileType getFileType(const QString &fileName);
    static QString getFileTypeName(FileType type);
};


#endif // MISCUTILS_H
