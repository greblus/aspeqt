package net.greblus;
import org.greblus.AspeQt.R;

import java.lang.System;
import android.widget.Toast;
import android.os.Bundle;
import java.lang.String;
import android.util.Log;

import com.hoho.android.usbserial.driver.UsbSerialPort;
import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.FtdiSerialDriver;
import com.hoho.android.usbserial.driver.UsbId;
import android.hardware.usb.UsbManager;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbEndpoint;
import android.hardware.usb.UsbRequest;
import java.nio.ByteBuffer;

import java.util.HashMap;
import java.util.Iterator;
import android.app.PendingIntent;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.Context;
import android.content.BroadcastReceiver;
import java.io.IOException;
import android.view.WindowManager;

// SIO2PC-USB backend: talks to an FTDI SIO2PC cable through the vendored
// usb-serial-for-android (upstream 3.10.0) driver and implements the Atari SIO
// command/data framing on top of it.
//
// The whole class is the "wrapper" that adapts upstream 3.10's API to what the
// Atari SIO protocol needs. Everything non-obvious here exists because SIO is
// timing-sensitive and reads/writes tiny frames (1-5 bytes) at up to ~126 kbit/s.
// The four spots that differ from a plain serial read/write are called out in
// comments: sioRead(), setSpeed(), rawStatus() and the direct driver construction.
public class SIO2PCUS4A implements SerialDevice
{
    private int devCount = 0;
    private int counter;
    private UsbDevice device = null;
    private UsbSerialDriver driver;
    private UsbSerialPort sPort;
    private UsbManager manager;
    private PendingIntent pintent;
    private static final String ACTION_USB_PERMISSION =
                                    "com.android.example.USB_PERMISSION";

    private boolean debug = false;
    private SerialActivity sa = SerialActivity.s_activity;

    // Cached at open() and used by the raw read path and the modem-status poll.
    private UsbDeviceConnection sioConn;
    private UsbEndpoint sioReadEp;
    private int sioMaxPkt = 64;

    // ---- Raw async read ----------------------------------------------------
    // We deliberately bypass the driver's own read() and talk to the read
    // endpoint directly, exactly like the old (pre-3.10) fork did. Why:
    //   * The FTDI prefixes every USB packet with 2 modem-status bytes and only
    //     flushes on its latency timer, so a read returns "one packet worth".
    //   * A queued UsbRequest + requestWait() returns as soon as that one packet
    //     arrives, yielding 0 payload when only the status header came in. AspeQt's
    //     speed auto-detection (readCommandFrame in serialport-android.cpp) relies
    //     on these prompt, per-packet, sometimes-empty returns to decide when to
    //     flip between normal and high speed.
    //   * Upstream 3.10's read(dest,len,timeout) instead spins/blocks until real
    //     payload or the timeout, which is far too slow per call and stalls the
    //     speed toggle.
    // A fresh request is created and closed each call so nothing stays queued to
    // collide with the control transfers done by setSpeed()/rawStatus().
    private int sioRead(byte[] dest, int want, int timeout) throws IOException {
        if (sioConn == null || sioReadEp == null) return 0;
        UsbRequest req = new UsbRequest();
        try {
            if (!req.initialize(sioConn, sioReadEp)) return 0;
            ByteBuffer buf = ByteBuffer.wrap(dest);
            if (!req.queue(buf, dest.length)) return 0;
            if (sioConn.requestWait() == null) return 0;
            int total = buf.position();
            if (total < 2) return 0;                 // header only -> no payload
            return filterStatus(dest, total);
        } catch (Exception e) {
            // A transient async/USB hiccup must not escape (it would break the
            // SIO worker) — treat it as an empty read so the caller just retries.
            return 0;
        } finally {
            req.close();
        }
    }

    // Strip the 2-byte FTDI modem-status header that prefixes each 64-byte USB
    // packet, compacting the real payload down in place. Returns the payload
    // length. (Mirrors the old fork's filterStatusBytes.)
    private int filterStatus(byte[] buf, int total) {
        int mp = sioMaxPkt;
        int packets = (total + mp - 1) / mp;
        int dst = 0;
        for (int p = 0; p < packets; p++) {
            int count = (p == packets - 1) ? total - p * mp - 2 : mp - 2;
            if (count > 0) {
                System.arraycopy(buf, p * mp + 2, buf, dst, count);
                dst += count;
            }
        }
        return dst;
    }

    SIO2PCUS4A() {
        manager = (UsbManager)sa.getSystemService(Context.USB_SERVICE);
    }

    public int openDevice() {
        HashMap<String, UsbDevice> deviceList = manager.getDeviceList();
        Iterator<UsbDevice> deviceIterator = deviceList.values().iterator();

        if (!deviceIterator.hasNext()) {
            sa.runOnUiThread(new Runnable() {
                public void run() {
                    Toast.makeText(sa, sa.getResources().getString(R.string.sio2pc_not_attached), Toast.LENGTH_LONG).show();
                }
            });
            return 0;
        }

            int dev_pid, dev_vid;
            boolean dev_found = false;
             do {
                 device = deviceIterator.next();
                 dev_pid = device.getProductId();
                 dev_vid = device.getVendorId();
                 if ((dev_vid == 1027) &&
                    (
                      (dev_pid == 24577) || //Lotharek's Sio2PC-USB
                      (dev_pid == 33712) || //Ray's Sio2USB-1050PC
                      (dev_pid == 33713) ||
                      (dev_pid == 24597)    //Ray's Sio2PC-USB
                    )
                ) { dev_found = true; break; }

//                 if ((dev_vid  == 1659) && (dev_pid == 8963)) //PL2303
//                    { dev_found = true; break;}

        } while (deviceIterator.hasNext());

        if (!dev_found) {
            sa.runOnUiThread(new Runnable() {
                public void run() {
                    Toast.makeText(sa, sa.getResources().getString(R.string.sio2pc_not_attached), Toast.LENGTH_LONG).show();
                }
            });
            return 0;
        }

        // Ask for USB permission if we don't have it yet, then WAIT for the
        // async result before opening. On targetSdk 31+ the PendingIntent must
        // carry an explicit mutability flag (FLAG_MUTABLE for USB permission),
        // otherwise getBroadcast() throws and the device is never opened.
        if (!manager.hasPermission(device)) {
            int flag = (android.os.Build.VERSION.SDK_INT >= 31) ? PendingIntent.FLAG_MUTABLE : 0;
            pintent = PendingIntent.getBroadcast(sa, 0, new Intent(ACTION_USB_PERMISSION), flag);
            SerialActivity.usbPermissionLatch = new java.util.concurrent.CountDownLatch(1);
            manager.requestPermission(device, pintent);
            try {
                SerialActivity.usbPermissionLatch.await(30, java.util.concurrent.TimeUnit.SECONDS);
            } catch (InterruptedException e) {}
            if (!manager.hasPermission(device)) {
                sa.runOnUiThread(new Runnable() {
                    public void run() {
                        Toast.makeText(sa, sa.getResources().getString(R.string.sio2pc_no_permissions), Toast.LENGTH_LONG).show();
                    }
                });
                return 0;
            }
        }

        if (debug) Log.i("USB", "Device found!");
        // The device was already matched by vid/pid above (including Ray's custom
        // PIDs 0x83B0/0x83B1 which upstream's default prober does not know), so
        // construct the FTDI driver directly instead of going through
        // UsbSerialProber.findAllDrivers().
        driver = new FtdiSerialDriver(device);

        UsbDeviceConnection connection = manager.openDevice(device);

        sPort = driver.getPorts().get(0);

        try {
            sPort.open(connection);
            sPort.setParameters(19200, 8, UsbSerialPort.STOPBITS_1, UsbSerialPort.PARITY_NONE);
            sPort.setDTR(true);
            sPort.setRTS(true);
            // Cache the connection + read endpoint for sioRead() and rawStatus().
            // (The FTDI latency timer is left at its 16 ms default, like the old
            // fork — lower values split the 5-byte command frame across packets.)
            sioConn = connection;
            sioReadEp = sPort.getReadEndpoint();
            if (sioReadEp != null) sioMaxPkt = sioReadEp.getMaxPacketSize();
        } catch (IOException e) {
            if (debug) Log.i("USB", "Can't open port");
            sa.runOnUiThread(new Runnable() {
                public void run() {
                    Toast.makeText(sa, sa.getResources().getString(R.string.sio2pc_failed_connecting), Toast.LENGTH_LONG).show();
                }
            });
            return -1;
        }
        if (debug) Log.i("USB", "Device opened");
        sa.runOnUiThread(new Runnable() {
            public void run() {
                Toast.makeText(sa, sa.getResources().getString(R.string.sio2pc_connected), Toast.LENGTH_LONG).show();
            }
        });
        return 1;
    }

    public void closeDevice() {
     try {
       if (sPort != null)
            sPort.close();
     } catch (IOException e) {
            Log.i("USB", "Can't close port");
    }
    }

    public int setSpeed(int speed) {
     // Upstream 3.10 has no standalone setBaudRate; setParameters re-sends the
     // full framing plus the new baud.
     //
     // Stop bits: PAL Atari high-speed SIO (POKEY divisor 0-3) needs TWO stop
     // bits so the POKEY receiver gets enough inter-byte gap to resync — without
     // it the receiver drops the last byte of a frame and the Atari retries
     // (that was the source of the high-speed hiccups). Per HiassofT's AtariSIO
     // baud table ("needs 2 stopbits on PAL"). The command frame stays at
     // standard 19200 with one stop bit.
     int stop = (speed > 19200) ? UsbSerialPort.STOPBITS_2 : UsbSerialPort.STOPBITS_1;
     //
     // The baud control transfer can transiently return result=-1 (which
     // setParameters turns into an IOException) when it lands right after an async
     // read — the USB stack is momentarily busy. A short sleep lets it settle and
     // a few retries make sure the speed change actually takes effect; otherwise
     // the speed the code thinks it is at and the speed the FTDI is really at
     // diverge, which desyncs the Atari. (See also USB_WRITE_TIMEOUT_MILLIS in the
     // vendored FtdiSerialDriver, raised to 50000 for the same reason.)
     for (int tries = 0; tries < 4; tries++) {
        try {
            sPort.setParameters(speed, 8, stop, UsbSerialPort.PARITY_NONE);
            return speed;
        } catch (Exception e) {
            try { Thread.sleep(1); } catch (InterruptedException ie) {}
        }
     }
     if (debug) Log.i("USB", "Can't set speed " + speed);
     return -1;
    }

    public int read(int size, int total)
    {
    sa.rbuf.position(total);
    int ret = 0, rd = 0;

    try {
        do {
             rd = sioRead(sa.rb, size, 5000);
             sa.rbuf.put(sa.rb, 0, rd);
             size -= rd; ret += rd;
        } while (size > 0);
    } catch (IOException e) {
       if (debug) Log.i("USB", "Can't read");
    }
    return ret;
    }

    public int write(int size, int total) {
    // Upstream write() writes the whole buffer or throws (SerialTimeoutException
    // is an IOException), so no manual chunk loop is needed here.
    int ret = 0;
    sa.wbuf.position(total);
    sa.wbuf.get(sa.wb, 0, size);

    try {
        sPort.write(sa.wb, size, 5000);
        ret = size;
    } catch (IOException e) {
       if (debug) Log.i("USB", "Can't write");
    }
    return ret;
    }

    // Upstream purgeHwBuffers is void and takes (purgeWrite, purgeRead) — the
    // argument order is flipped versus the old fork, which was (flushRX, flushTX).
    public boolean purge() {
    boolean ret = true;
    try {
        sPort.purgeHwBuffers(true, true);
    } catch (IOException e) {
        if (debug) Log.i("USB", "Can't purge");
        ret = false;
    }
    return ret;
    }

    public boolean purgeTX() {
    boolean ret = true;
    try {
        sPort.purgeHwBuffers(true, false);
    } catch (IOException e) {
        if (debug) Log.i("USB", "Can't purge TX buffer");
        ret = false;
    }
    return ret;
    }

    public boolean purgeRX() {
    boolean ret = true;
    try {
        sPort.purgeHwBuffers(false, true);
    } catch (IOException e) {
        if (debug) Log.i("USB", "Can't purge RX buffer");
        ret = false;
    }
    return ret;
    }

    public void qLog(String msg) {
    if (debug) Log.i("USB", msg);
    }

    // Raw FTDI GET_MODEM_STATUS control transfer, like the old fork. We do this by
    // hand instead of using upstream's getControlLines() because getControlLines()
    // allocates an EnumSet on every call, and the hardware command-frame detection
    // (getHWCommandFrame, for the RI/DSR/CTS handshake methods) polls the status in
    // a tight loop where that per-call overhead measurably hurts timing.
    // getHWCommandFrame only tests bits 4-6 (CTS 0x10 / DSR 0x20 / RI 0x40), so the
    // raw status byte is exactly what it needs.
    private final byte[] modemStatusBuf = new byte[2];
    private int rawStatus() throws IOException {
        int result, tries = 0;
        do {
            result = sioConn.controlTransfer(0xC0 /*vendor, device->host*/, 5 /*GET_MODEM_STATUS*/,
                    0, 0, modemStatusBuf, 2, 200);
        } while (result < 0 && ++tries < 4);
        if (result < 0) throw new IOException("modem status failed: " + result);
        return modemStatusBuf[0] & 0xff;
    }

    public int getModemStatus() {
    int ret = -2;
    try {
        ret = rawStatus();
    } catch (IOException e) {
        if (debug) Log.i("USB", "Can't get modem status");
        ret = -1;
    }
    return ret;
    }

    // Level read of COMMAND for the R: device's stream mode: no waiting, no
    // reading, just "is the Atari asserting it right now". Masks match
    // getHWCommandFrame: 64 = RI, 32 = DSR, 16 = CTS.
    public boolean isCommandAsserted(int mMethod) {
    int mask;

    switch (mMethod) {
        case 0:
            mask = 64;
            break;
        case 1:
            mask = 32;
            break;
        case 2:
            mask = 16;
            break;
        default:
            mask = 32; }

    int status = getModemStatus();
    if (status < 0) return false;
    return (status & mask) > 0;
    }

    public int getSWCommandFrame() {
    int expected = 0, sync_attempts = 0, got = 1, total_retries = 0;
    int ret = 0, total = 0;
    boolean prtl = false;

    sa.rbuf.position(0);
    mainloop:
    while (true) {
        ret = 0; total = 0; total_retries = 0;
        do {
            if (total_retries > 2) return 2;
            try {
                ret = sioRead(sa.rb, 5-total, 5000);
                if (ret == 5) break;
            }
            catch (IOException e) {};

            if ((ret > 0) && (ret < 5)) {
                System.arraycopy(sa.rb, 0, sa.t, total, ret);
                prtl = true;
                total += ret;
            }
            if ((total == 5) && (prtl == true))
                    System.arraycopy(sa.t, 0, sa.rb, 0, 5);
            if (ret <= 0)
                total_retries++;
        } while (total<5);

        expected = sa.rb[4] & 0xff;
        got = sioChecksum(sa.rb, 4) & 0xff;

        if (checkDesync(sa.rb, got, expected) > 0) {
            if (debug) Log.i("USB", "Apparent desync");
            if (sync_attempts < 10) {
                    sync_attempts++;
                    for (int i = 0; i < 4; i++)
                            sa.rb[i] = sa.rb[i+1];
                    ret = 0;
                    do {
                        try {
                            ret = sioRead(sa.t, 1, 5000); }
                        catch (IOException e) {};
                    } while (ret < 1);
                    sa.rb[4] = sa.t[0];
            } else
                continue mainloop;
        } else {
            if (debug) Log.i("USB", "No desync");

            for (int i=0; i<4; i++)
               sa.rbuf.put((byte)(sa.rb[i] & 0xff));

               if (debug) {
                   String data = "";
                   for (int i=0; i<4; i++)
                       data += Integer.toString(sa.rb[i] & 0xff) + " ";
                   Log.i("USB", "Command Frame: " + data);
               }
            break;
        }
     };
     if (debug) Log.i("USB", "got=" + got + " expected= " + expected);
     return 1;
    }

    public int getHWCommandFrame(int mMethod) {
    int mask, total_retries, status, total, res = 0;
    boolean ret;

    switch (mMethod) {
        case 0:
            mask = 64;
            break;
        case 1:
            mask = 32;
            break;
        case 2:
            mask = 16;
            break;
        default:
            mask = 32; }

    sa.rbuf.position(0);
    do {
        status = 0; total_retries = 0;
        do {
            if (total_retries > 10e2) return 2;
            status = getModemStatus();
            total_retries += 1;

            if (status < 0) {
                if (debug) Log.i("USB", "Cannot retrieve serial port status");
                return 0;
            }
        } while (!((status & mask) > 0));

        ret = purge();
        if (!ret) if (debug) Log.i("USB", "Cannot clear serial port");

        total = 0; total_retries = 0;
        do {
            res = 0;
            try {
                if (total_retries > 4) return 2;
                res = sioRead(sa.rb, 5-total, 5000); }
            catch (IOException e) {};

            if (res > 0) {
                for (int i=0; i<res; i++) {
                   if (debug) Log.i("USB", "CF: " + (sa.rb[i] & 0xff));
                   sa.rbuf.put((byte)(sa.rb[i] & 0xff));
                   total += 1;
                }
            } else
                total_retries++;
        } while (total<5);

        int expected = (byte) sa.rb[4] & 0xff;
        int got = sioChecksum(sa.rb, 4);

        if (expected != got) return 2;

        total_retries = 0;
        do {
            if (total_retries > 10e2) return 2;
            status = getModemStatus();
            total_retries += 1;

            if (status < 0) {
                if (debug) Log.i("USB", "Cannot retrieve serial port status");
                return 0;
            }
        } while ((status & mask) > 0);
        break;
     } while (true);
     return 1;
    }


    public static int sioChecksum(byte[] data, int size)
    {
            int sum = 0;
            for (int i=0; i < size; i++) {

                sum += data[i] & 0xff;
                if (sum > 255) {
                    sum -= 255;
                }
            }
            return sum;
    }

    public static int checkDesync(byte[] cmd, int got, int expected)
    {
        if (got != expected)
            return 1;

        int ccom = (byte) cmd[1] & 0xff;

        int cdev = (byte) cmd[0] & 0xff;
        int cid = cmd[0] & 0xf0;

        if ((cid != 0x20) && (cid != 0x30) && (cid != 0x40) && (cid != 0x50) &&
            (cid != 0x60) && (cid != 0xf0))
             return 1;

        return 0;
    }
}
