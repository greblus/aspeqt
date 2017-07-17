package net.greblus;
import org.qtproject.example.AspeQt.R;

import java.lang.System;
import android.widget.Toast;
import android.os.Bundle;
import java.lang.String;
import java.util.List;
import android.util.Log;

import com.hoho.android.usbserial.driver.UsbSerialPort;
import com.hoho.android.usbserial.driver.UsbSerialProber;
import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialPort;
import com.hoho.android.usbserial.driver.UsbId;
import android.hardware.usb.UsbManager;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;

import java.util.HashMap;
import java.util.Iterator;
import android.app.PendingIntent;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.Context;
import android.content.BroadcastReceiver;
import java.io.IOException;
import android.widget.Toast;
import android.view.WindowManager;

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

        if (dev_found) {
            pintent = PendingIntent.getBroadcast(sa, 0, new Intent(ACTION_USB_PERMISSION), 0);
            manager.requestPermission(device, pintent);
        } else {
            sa.runOnUiThread(new Runnable() {
                public void run() {
                    Toast.makeText(sa, sa.getResources().getString(R.string.sio2pc_not_attached), Toast.LENGTH_LONG).show();
                }
            });
            return 0;
        }

        Log.i("USB", "Device found!");
        List<UsbSerialDriver> availableDrivers = UsbSerialProber.getDefaultProber().findAllDrivers(manager);

        if (availableDrivers.isEmpty()) {
            Log.i("USB", "No drivers found for attached usb devices");
            return 0;
        }

        Log.i("USB", "Driver found for attached usb device");
        driver = availableDrivers.get(0);

        UsbDeviceConnection connection = manager.openDevice(device);

        sPort = driver.getPorts().get(0);

        try {
            sPort.open(connection);
            sPort.setParameters(19200, 8, UsbSerialPort.STOPBITS_1, UsbSerialPort.PARITY_NONE);
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
     int ret = 0;
     try {
        ret = sPort.setBaudRate(speed);
     } catch (IOException e) {
       Log.i("USB", "Can't set speed");
     }
     if (debug) Log.i("USB", "setBaudrate: " + ret);
    return ret;
    }

    public int read(int size, int total)
    {
    sa.rbuf.position(total);
    int ret = 0, rd = 0;

    try {
        do {
             rd = sPort.sread(sa.rb, size, 5000);
             sa.rbuf.put(sa.rb, 0, rd);
             size -= rd; ret += rd;
        } while (size > 0);
    } catch (IOException e) {
       Log.i("USB", "Can't read");
    }
    return ret;
    }

    public int write(int size, int total) {
    int ret = 0, wn = 0;
    sa.wbuf.position(total);
    sa.wbuf.get(sa.wb, 0, size);

    try {
        do {
            wn = sPort.swrite(sa.wb, size, 5000);
            size -= wn; ret += wn;
        } while (size > 0);
    } catch (IOException e) {
       Log.i("USB", "Can't write");
    }
    return ret;
    }

    public boolean purge() {
    boolean ret;
    try {
        ret = sPort.purgeHwBuffers(true, true);
    } catch (IOException e) {
        Log.i("USB", "Can't purge");
        ret = false;
    }
    if (debug) Log.i("USB", "purge: " + ret);
    return ret;
    }

    public boolean purgeTX() {
    boolean ret;
    try {
        ret = sPort.purgeHwBuffers(false, true);
    } catch (IOException e) {
        Log.i("USB", "Can't purge TX buffer");
        ret = false;
    }
    if (debug) Log.i("USB", "purgeTX: " + ret);
    return ret;
    }

    public boolean purgeRX() {
    boolean ret;
    try {
        ret = sPort.purgeHwBuffers(true, false);
    } catch (IOException e) {
        Log.i("USB", "Can't purge RX buffer");
        ret = false;
    }
    if (debug) Log.i("USB", "purgeRX: " + ret);
    return ret;
    }

    public void qLog(String msg) {
    if (debug) Log.i("USB", msg);
    }

    public int getModemStatus() {
    int ret = -2;
    try {
        ret = sPort.getStatus();
    } catch (IOException e) {
        Log.i("USB", "Can't get modem status");
        ret = -1;
    }
    if (debug) {
        counter +=1;
        if (counter < 3) {
            Log.i("USB", "getModemStatus: " + ret);
        } else {
             if (counter == 3 ) Log.i("USB", "getModemStatus called too many times!");
             if (counter > 50000) counter = 0;
        }
    }
    return ret;
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
                ret = sPort.sread(sa.rb, 5-total, 5000);
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
                            ret = sPort.sread(sa.t, 1, 5000); }
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
                res = sPort.sread(sa.rb, 5-total, 5000); }
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

