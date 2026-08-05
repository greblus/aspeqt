#ifndef REMOTECONTROL_H
#define REMOTECONTROL_H

#include "sioworker.h"
#include "diskimage.h"
#include <QElapsedTimer>

// Device $61 -- AtariSIO's remote-control protocol, as spoken by Matthias
// Reichl's hisioboot-atarisio.atr: the Atari patches its OS for high-speed SIO,
// asks us to exchange D1: and D2:, and then boots D1: again. The exchange has to
// be in effect before we answer, which is why it happens right here on the SIO
// thread (SioWorker's device table is guarded by a recursive mutex we already
// hold while a command frame is being handled).
//
//   $43 'C'  execute: aux = length of an ASCII command, sent as a data frame
//   $53 'S'  status:  4 bytes -- [0] 1 = ok / 146 = error, [2..3] result length
//   $52 'R'  result:  128-byte chunk of the text output, aux = chunk number
// The bundled loader ATR, standing in for D1: until the Atari has booted it.
// It answers only for its own boot record; anything else means the Atari is no
// longer booting (a program reading D1:, or a disk mounted mid-session), and the
// command is passed straight to the disk the user actually mounted. The device
// table is put right afterwards by the engine, on the UI thread, where taking
// SioWorker's device mutex cannot cut into a command frame.
class BootShimImage : public SimpleDiskImage
{
    Q_OBJECT

public:
    BootShimImage(SioWorker *worker, SioDevice *real, int bootSectors)
        : SimpleDiskImage(worker), m_real(real), m_bootSectors(bootSectors) {}

    void handleCommand(quint8 command, quint16 aux) override;
    void handOver();                       // "xc" arrived: step aside
    void forward(quint8 command, quint16 aux);  // hand a command to the mounted disk
    SioDevice *realDisk() const { return m_real; }

signals:
    void steppedAside();                   // queued: engine swaps us out and frees us

private:
    SioDevice *m_real = nullptr;
    int  m_bootSectors = 0;
    int  m_lastSector = 0;      // boot records are read in order
    bool m_serving = true;      // answering as the loader, or forwarding?
    bool m_usedReal = false;    // the mounted disk has been read since the handover
    bool m_afterGap = false;    // first command after the Atari went quiet (a reset)
    QElapsedTimer m_idle;
};

class RemoteControl : public SioDevice
{
    Q_OBJECT

public:
    RemoteControl(SioWorker *worker) : SioDevice(worker) {}
    void handleCommand(quint8 command, quint16 aux);

    // While a shim is armed, $31 serves the boot loader and the disk the user
    // actually mounted waits here. "xc" then puts it back instead of moving
    // slots around -- the UI never changes and D2: stays free for disk two of a
    // multi-disk game. Called from the engine before the Atari boots.
    void armBootShim(BootShimImage *shim) { m_shim = shim; }
    void forgetBootShim() { m_shim = nullptr; }
    bool bootShimArmed() const { return m_shim != nullptr; }

signals:
    // The loader has been swapped out for the real disk; the engine can drop it.
    void bootShimReleased();
    // Reported to the engine (queued) so the log and the UI can catch up.
    void drivesExchanged(int d1, int d2);

private:
    enum { StatusOk = 1, StatusError = 146 };

    bool runCommand(const QString &line);

    BootShimImage *m_shim = nullptr;  // the loader, while it stands in for D1:
    QByteArray m_result;             // text answer, read back with 'R'
    quint8     m_status = StatusError;
};

#endif // REMOTECONTROL_H
