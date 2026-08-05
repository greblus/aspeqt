#include "remotecontrol.h"

#include <QStringList>
#include <QRegularExpression>
#include <QtDebug>

void RemoteControl::handleCommand(quint8 command, quint16 aux)
{
    switch (command) {

    // Execute an ASCII command whose length arrives in aux.
    case 0x43:
    {
        m_status = StatusError;
        m_result.clear();

        if (!sio->port()->writeCommandAck())
            return;

        QByteArray data;
        if (aux) {
            data = sio->port()->readDataFrame(aux);
            if (data.isEmpty()) {
                qCritical() << "!e" << tr("[%1] Read data frame failed").arg(deviceName());
                sio->port()->writeDataNak();
                sio->port()->writeError();
                return;
            }
            sio->port()->writeDataAck();
        }

        // An empty command is the no-op the boot loader sends to finish up.
        const bool ok = aux ? runCommand(QString::fromLatin1(data).trimmed()) : true;
        if (!ok) {
            sio->port()->writeError();
            return;
        }
        sio->port()->writeComplete();
        m_status = StatusOk;
        break;
    }

    // Status of the last command, plus the length of its text output.
    case 0x53:
    {
        if (!sio->port()->writeCommandAck())
            return;

        QByteArray data(4, 0);
        data[0] = (char) m_status;
        data[1] = 0;
        data[2] = (char) (m_result.size() & 0xff);
        data[3] = (char) ((m_result.size() >> 8) & 0xff);

        sio->port()->writeComplete();
        sio->port()->writeDataFrame(data);
        break;
    }

    // One 128-byte chunk of the text output.
    case 0x52:
    {
        if (!sio->port()->writeCommandAck())
            return;

        QByteArray chunk = m_result.mid(aux * 128, 128);
        if (chunk.size() < 128)                       // never resize(): Qt 6
            chunk.append(QByteArray(128 - chunk.size(), '\0'));   // leaves it uninitialised

        sio->port()->writeComplete();
        sio->port()->writeDataFrame(chunk);
        break;
    }

    default:
        qWarning() << "!w" << tr("[%1] command: $%2, aux: $%3 unknown.")
                      .arg(deviceName())
                      .arg(command, 2, 16, QChar('0'))
                      .arg(aux, 4, 16, QChar('0'));
        sio->port()->writeCommandNak();
        break;
    }
}

// The subset of AtariSIO's command language we answer to. Everything else fails
// with the same status its server returns, so a client can tell it went nowhere.
bool RemoteControl::runCommand(const QString &line)
{
    const QStringList tok = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    if (tok.isEmpty())
        return true;

    // "xc <d1> <d2>" -- exchange two drives. This is what the high-speed boot
    // loader sends after patching the OS; it then boots D1: straight away.
    if (tok.at(0).compare(QLatin1String("xc"), Qt::CaseInsensitive) == 0) {
        bool ok1 = false, ok2 = false;
        const int d1 = tok.value(1).toInt(&ok1);
        const int d2 = tok.value(2).toInt(&ok2);
        if (!ok1 || !ok2 || d1 < 1 || d1 > 8 || d2 < 1 || d2 > 8) {
            m_result = "xc <driveno1> <driveno2>\n";
            return false;
        }
        // With the loader standing in for D1:, "exchange" means: give the Atari
        // the disk it was always shown. Nothing else moves.
        if (m_shim && d1 == 1) {
            // Keep the pointer: the loader stays in the device table and takes
            // the Atari's next boot too. Forgetting it here made the second
            // "xc" fall through to the literal exchange below -- which is how
            // D1: and D2: ended up swapped after a cold reset.
            m_shim->handOver();
            qDebug() << "!n" << tr("[%1] Handed D1: over to the mounted disk.")
                        .arg(deviceName());
            return true;
        }

        sio->swapDevices(0x30 + d1, 0x30 + d2);
        qDebug() << "!n" << tr("[%1] Exchanged D%2: and D%3:.")
                    .arg(deviceName()).arg(d1).arg(d2);
        emit drivesExchanged(d1, d2);
        return true;
    }

    m_result = "unknown command\n";
    return false;
}

// "xc" arrived: the loader is in RAM and has done its job, so everything from
// here on belongs to the mounted disk.
void BootShimImage::handOver()
{
    if (!m_serving)
        return;
    m_serving = false;
    m_usedReal = false;
    emit steppedAside();
}

// The loader stays in the device table for as long as the option is on, and
// decides per command who answers. It takes the Atari's boot -- a status
// followed by sectors read in order from 1 -- and passes everything else to the
// disk the user mounted. That way each reboot is patched for high speed again,
// while a program (or disk two of a game) reading D1: never sees the loader.
void BootShimImage::handleCommand(quint8 command, quint16 aux)
{
    if (!m_real) {
        SimpleDiskImage::handleCommand(command, aux);
        return;
    }

    // A reset leaves the drive quiet while the Atari runs its own startup, so a
    // sector 1 that arrives after a pause is the OS booting. Without this, DOS
    // reading sector 1 in the middle of its work looked exactly like a boot: we
    // answered with the loader's boot record, and the boot that came later was
    // handed straight to the disk -- unpatched, at standard speed.
    const qint64 gap = m_idle.isValid() ? m_idle.elapsed() : 1000;
    m_idle.restart();
    if (gap > 700)
        m_afterGap = true;

    if (m_serving) {
        if (command == 0x52) {
            const bool inOrder = (aux == 1)
                              || (aux == (quint16) (m_lastSector + 1)
                                  && aux <= (quint16) m_bootSectors);
            if (!inOrder) {                   // not our boot after all
                qDebug() << "!n" << tr("[%1] Not a boot read; the mounted disk takes over.")
                            .arg(deviceName());
                m_serving = false;
                m_usedReal = false;
                forward(command, aux);
                return;
            }
            m_lastSector = aux;
        }
        SimpleDiskImage::handleCommand(command, aux);
        return;
    }

    // Forwarding. A status followed by a read of sector 1 is the OS starting a
    // boot; take it, but only once the mounted disk has really been used since
    // the last handover -- otherwise we would grab the boot we just handed over
    // and the Atari would loop through the loader forever.
    if (command == 0x52) {
        if (aux == 1 && m_usedReal && m_afterGap) {
            qDebug() << "!n" << tr("[%1] The Atari is booting again; patching once more.")
                        .arg(deviceName());
            m_serving = true;
            m_usedReal = false;
            m_lastSector = 1;
            m_afterGap = false;
            SimpleDiskImage::handleCommand(command, aux);
            return;
        }
        if (aux > (quint16) m_bootSectors)
            m_usedReal = true;
        m_afterGap = false;
    }
    forward(command, aux);
}

// SioWorker takes the device's lock before dispatching, but that is our lock,
// not the mounted disk's -- and the disk editor holds the disk's lock while it
// has the image open. Take it here too, so a sector cannot be served from an
// image that is being written to.
void BootShimImage::forward(quint8 command, quint16 aux)
{
    if (!m_real->tryLock()) {
        qWarning() << "!w" << tr("[%1] command: $%2, aux: $%3 ignored because the image explorer is open.")
                      .arg(deviceName())
                      .arg(command, 2, 16, QChar('0'))
                      .arg(aux, 4, 16, QChar('0'));
        return;
    }
    m_real->handleCommand(command, aux);
    m_real->unlock();
}
