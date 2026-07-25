// Hands the Epson printer's rendered page to QML. The image is produced on the
// SIO worker thread and requested on Qt Quick's render thread, so access is
// mutex-guarded; QImage's implicit sharing keeps the copies cheap.
#ifndef PAPERPROVIDER_H
#define PAPERPROVIDER_H

#include <QQuickImageProvider>
#include <QImage>
#include <QMutex>
#include <QMutexLocker>

class PaperProvider : public QQuickImageProvider
{
public:
    PaperProvider() : QQuickImageProvider(QQuickImageProvider::Image) {}

    void setImage(const QImage &img)
    {
        QMutexLocker locker(&m_mutex);
        m_image = img;
    }

    QImage requestImage(const QString & /*id*/, QSize *size, const QSize & /*requested*/) override
    {
        QMutexLocker locker(&m_mutex);
        if (size)
            *size = m_image.size();
        return m_image;
    }

private:
    QImage m_image;
    QMutex m_mutex;
};

#endif // PAPERPROVIDER_H
