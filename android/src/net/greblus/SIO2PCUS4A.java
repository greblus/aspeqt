package net.greblus;
import org.qtproject.example.AspeQt.R;

import java.lang.System;
import android.widget.Toast;
import android.os.Bundle;
import java.lang.String;
import java.util.List;
import android.util.Log;

import com.ftdi.j2xx.D2xxManager;
import com.ftdi.j2xx.FT_Device;

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
    private UsbManager manager;

    private static D2xxManager ftdid2xx = null;
    private static FT_Device ftDevice = null;
    private static D2xxManager.FtDeviceInfoListNode devInfo;

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
        try {
            ftdid2xx = D2xxManager.getInstance(sa);
            devCount = (int)ftdid2xx.createDeviceInfoList(sa);
            ftdid2xx.setVIDPID(1027, 33713);
            ftdid2xx.setVIDPID(1027, 24597);
            ftdid2xx.setVIDPID(1027, 24577);
            ftdid2xx.setVIDPID(1027, 33712);
            if (!ftdid2xx.isFtDevice(device))
                return -1;

            D2xxManager.DriverParameters drvParams = new D2xxManager.DriverParameters();
            drvParams.setMaxTransferSize(64);
            drvParams.setReadTimeout(1000);

            if (devCount > 0) {
            try {               
                ftDevice = ftdid2xx.openByUsbDevice(sa, device, drvParams);
            } catch(NullPointerException e){
                if (debug) Log.i("FTDI", e.getMessage(), e);
            } finally {
                if (ftDevice == null) {
                    if (debug) Log.i("FTDI", "No Devices opened!");
                    return devCount;
                }
            }

        if (ftDevice.isOpen()) {
                devInfo = ftDevice.getDeviceInfo();
                ftDevice.setBitMode((byte)0, D2xxManager.FT_BITMODE_RESET);
                boolean ret = ftDevice.setFlowControl(D2xxManager.FT_FLOW_NONE, (byte) 0x00, (byte)0x00 );
                ftDevice.setDataCharacteristics(D2xxManager.FT_DATA_BITS_8, D2xxManager.FT_STOP_BITS_1,
                                                D2xxManager.FT_PARITY_NONE);

                ftDevice.purge((byte)(D2xxManager.FT_PURGE_TX | D2xxManager.FT_PURGE_RX));


                if (debug) Log.i("FTDI", "Opened device " + devInfo.description);
            } else
                { if (debug) Log.i("FTDI", "No device opened!"); return 0; }
            } else  if (debug) Log.i("FTDI", "No device connected!");
         } catch (D2xxManager.D2xxException ex) {
                ex.printStackTrace();
         }

        if (debug) Log.i("FTDI", "devCount: " + devCount);
            return devCount;
        }

        public void closeDevice() {
            if (ftDevice != null) ftDevice.close();
        }

        public int setSpeed(int speed)
        {
            if (ftDevice == null) return 0;

            boolean ret = ftDevice.setBaudRate(speed);
            if (ret) return speed;
            else
                return 0;
        }

        public int read(int size, int total)
        {
            int avail = ftDevice.getQueueStatus();
            if (avail <= 0) return 0;
            sa.rbuf.position(total);
            if (size < avail)
                avail = size;
            int rd = ftDevice.read(sa.rb, avail);
            return rd;
        }

        public int write(int size, int total)
        {
            sa.wbuf.position(total);
            sa.wbuf.get(sa.wb, 0, size);
            return ftDevice.write(sa.wb, size);
        }

        public boolean purge() {
            boolean ret = ftDevice.purge((byte)(D2xxManager.FT_PURGE_TX | D2xxManager.FT_PURGE_RX));
            if (debug) Log.i("USB", "purge: " + ret);
        return ret;
        }

        public boolean purgeTX() {
            boolean ret = ftDevice.purge((byte)(D2xxManager.FT_PURGE_TX));
            if (debug) Log.i("USB", "purgeTX: " + ret);
        return ret;
        }

        public boolean purgeRX() {
            boolean ret = ftDevice.purge((byte)(D2xxManager.FT_PURGE_RX));
            if (debug) Log.i("USB", "purgeRX: " + ret);
        return ret;
        }

        public void qLog(String msg) {
            if (debug) Log.i("USB", msg);
        }

        public int getModemStatus() {
            //int ret = -2;
            int ret = ftDevice.getModemStatus();
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
                    int avail = waitTillAvailable(40);
                    if (avail == 0)
                        return 2;
                    else
                        ret = ftDevice.read(sa.rb, 5-total);
                    if (ret == 5) break;
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

                            if (waitTillAvailable(10) > 0) {
                                ftDevice.read(sa.t, 1);
                                sa.rb[4] = sa.t[0];                             
                            } else
                                return 2;
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

                ret = purgeRX();
                if (!ret) if (debug) Log.i("USB", "Cannot clear serial port");

                total = 0; total_retries = 0;
                do {
                    res = 0;
                    int avail = waitTillAvailable(40);
                    if (avail == 0) return 2;
                    res = ftDevice.read(sa.rb, 5-total);

                    if (res > 0) {
                        for (int i=0; i<res; i++) {
                           if (debug) Log.i("USB", "CF: " + (sa.rb[i] & 0xff));
                           sa.rbuf.put((byte)(sa.rb[i] & 0xff));
                           total += 1;
                        }
                    }
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

        public int waitTillAvailable(int timeout) {
            int avail = 0;
            long t_now, t_start;
            t_start = System.currentTimeMillis();

            while (true) {
            avail = ftDevice.getQueueStatus();
            if (avail > 0) break;

            try { Thread.sleep(1);
            } catch (Exception e) { }

            t_now = System.currentTimeMillis();
            if (t_now-t_start > timeout)
                return 0;
            }
            if (debug) Log.i("USB", Integer.toString(avail));
            return avail;
        }

}

