package net.greblus;

import java.lang.System;
import android.widget.Toast;
import android.os.Bundle;
import java.lang.String;
import java.util.List;
import android.util.Log;

import org.qtproject.qt5.android.bindings.QtActivity;

import com.felhr.usbserial.CDCSerialDevice;
import com.felhr.usbserial.UsbSerialDevice;
import com.felhr.usbserial.UsbSerialInterface;

import android.content.Context;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import android.hardware.usb.UsbManager;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import java.util.HashMap;
import java.util.Iterator;
import android.app.PendingIntent;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.BroadcastReceiver;
import android.widget.Toast;
import android.view.WindowManager;

public class SerialActivity extends QtActivity
{
        private static int devCount = 0;
        private static UsbManager manager;
        private static PendingIntent pintent;
        private static final String ACTION_USB_PERMISSION =
            "com.android.example.USB_PERMISSION";
        private static ByteBuffer rbuf = ByteBuffer.allocateDirect(65535);
        private static ByteBuffer wbuf = ByteBuffer.allocateDirect(65535);
        private static byte t[] = new byte [1024];
        private static byte t1[] = new byte [1];
        private static int counter;
        private static UsbDevice device = null;
        private static UsbSerialDevice sPort;
        public static native void sendBufAddr(ByteBuffer rbuf, ByteBuffer wbuf);
        private static boolean debug = false;
        public static String m_chosen;
        private static int m_filter;
        private static String m_action;
        private static String m_dir;

        public static SerialActivity s_activity = null;

        @Override
	public void onCreate(Bundle savedInstanceState)
 	{
		s_activity = this;
                super.onCreate(savedInstanceState);

                manager = (UsbManager)getSystemService(Context.USB_SERVICE);
                pintent = PendingIntent.getBroadcast(this, 0, new Intent(ACTION_USB_PERMISSION), 0);
                registerBroadcastReceiver();
                getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

                sendBufAddr(rbuf, wbuf);
        }

       @Override
       public void onPause() {
           m_chosen = "Cancelled";
           super.onPause();
        }


        public static void runFileChooser(int filter, int action, String dir) {
            Log.i("ASPEQT:", "DIR:" + dir);
            m_chosen = "None";
            m_filter = filter;
            m_dir = dir;

            if (action == 0)
                m_action = "FileOpen";
            else
                m_action = "FileSave";

            SerialActivity.s_activity.runOnUiThread( new FileChooser() );

         }

         public static void runDirChooser(String dir) {
            m_dir = dir;
            m_chosen = "None";
            m_filter = 0;
            SerialActivity.s_activity.runOnUiThread( new DirChooser() );

          }

        @Override
	protected void onDestroy()
	{
                super.onDestroy();
                s_activity = null;
                closeDevice();
	}

        public void fileChooser()
        {
            SimpleFileDialog FileOpenDialog =  new SimpleFileDialog(SerialActivity.this, m_action, m_filter, m_dir,
                                                            new SimpleFileDialog.SimpleFileDialogListener()
            {
                    @Override
                    public void onChosenDir(String chosenDir)
                    {
                            // The code in this function will be executed when the dialog OK button is pushed
                            m_chosen = chosenDir;
                            if (m_chosen != "Cancelled") Toast.makeText(SerialActivity.this, "Chosen File: " +
                                            m_chosen, Toast.LENGTH_LONG).show();
                    }
            });

            //You can change the default filename using the public variable "Default_File_Name"
            FileOpenDialog.Default_File_Name = "";
            FileOpenDialog.chooseFile_or_Dir();
        }

        public void dirChooser()
        {
            SimpleFileDialog FileOpenDialog =  new SimpleFileDialog(SerialActivity.this, "FolderChoose", m_filter, m_dir,
                                                            new SimpleFileDialog.SimpleFileDialogListener()
            {
                    @Override
                    public void onChosenDir(String chosenDir)
                    {
                            // The code in this function will be executed when the dialog OK button is pushed
                            m_chosen = chosenDir;
                            if (m_chosen != "Cancelled") Toast.makeText(SerialActivity.this, "Chosen Directory: " +
                                            m_chosen, Toast.LENGTH_LONG).show();
                    }
            });

            FileOpenDialog.chooseFile_or_Dir();
        }

        public static void registerBroadcastReceiver() {
                if (SerialActivity.s_activity != null) {
                        // Qt is running on a different thread than Android.
                        // In order to register the receiver we need to execute it in the UI thread
                        SerialActivity.s_activity.runOnUiThread( new RegisterReceiverRunnable());
                        Log.i("USB", "Broadcast Receiver registered");
                }
        }

        public static int openDevice() {
            HashMap<String, UsbDevice> deviceList = manager.getDeviceList();
            Iterator<UsbDevice> deviceIterator = deviceList.values().iterator();

            if (!deviceIterator.hasNext())
                return 0;

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

                     if ((dev_vid  == 1659) && (dev_pid == 8963)) //PL2303
                        { dev_found = true; break;}

            } while (deviceIterator.hasNext());

            if (dev_found)
                manager.requestPermission(device, pintent);
            else
                return 0;

            UsbDeviceConnection usbConnection = manager.openDevice(device);
            sPort = UsbSerialDevice.createUsbSerialDevice(device, usbConnection);
            sPort.open();
            sPort.setBaudRate(19200);
            sPort.setDataBits(UsbSerialInterface.DATA_BITS_8);
            sPort.setParity(UsbSerialInterface.PARITY_ODD);
            sPort.setFlowControl(UsbSerialInterface.FLOW_CONTROL_OFF);
            sPort.syncOpen();

            Log.i("USB", "Device opened");
            return 1;
       }

     public static void closeDevice() {
        if (sPort != null) {
            sPort.close();
            sPort.syncClose();
        }
    }

     public static int setSpeed(int speed) {
         sPort.setBaudRate(speed);
         return speed;
     }

    public static int read(int size, int total)
    {
        rbuf.position(total);
        int ret = 0, rd = 0;
        byte[] rb = new byte[size];
        do {
            rd = sPort.syncRead(rb, 5000);
            rbuf.put(rb, 0, rd);
            size -= rd; ret += rd;
        } while (size > 0);
        return ret;
    }

    public static int write(int size, int total) {
        int ret = 0, wn = 0;
        byte[] wb = new byte[size];
        wbuf.position(total);
        wbuf.get(wb, 0, size);

        do {
            wn = sPort.syncWrite(wb, 5000);
            size -= wn; ret += wn;
        } while (size > 0);

        return ret;
    }

    public static boolean purge() {
        boolean ret = true;
//        try {
//            ret = sPort.purgeHwBuffers(true, true);
//        } catch (IOException e) {
//            Log.i("USB", "Can't purge");
//            ret = false;
//        }
//        if (debug) Log.i("USB", "purge: " + ret);
        return ret;
    }

    public static boolean purgeTX() {
        boolean ret = true;
//        try {
//            ret = sPort.purgeHwBuffers(false, true);
//        } catch (IOException e) {
//            Log.i("USB", "Can't purge TX buffer");
//            ret = false;
//        }
//        if (debug) Log.i("USB", "purgeTX: " + ret);
        return ret;
    }

    public static boolean purgeRX() {
        boolean ret = true;
//        try {
//            ret = sPort.purgeHwBuffers(true, false);
//        } catch (IOException e) {
//            Log.i("USB", "Can't purge RX buffer");
//            ret = false;
//       }
//        if (debug) Log.i("USB", "purgeRX: " + ret);
        return ret;
    }

    public static void qLog(String msg) {
        if (debug) Log.i("USB", msg);
    }

    public static int getModemStatus() {
        int ret = -2;
//        try {
//            ret = sPort.getStatus();
//        } catch (IOException e) {
//            Log.i("USB", "Can't get modem status");
//            ret = -1;
//        }
//        if (debug) {
//            counter +=1;
//            if (counter < 3) {
//                Log.i("USB", "getModemStatus: " + ret);
//            } else {
//                 if (counter == 3 ) Log.i("USB", "getModemStatus called too many times!");
//                 if (counter > 50000) counter = 0;
//            }
//        }
        return ret;
    }


    public static int getSWCommandFrame() {
        int expected = 0, sync_attempts = 0, got = 1, ret = 0, total = 0;
        rbuf.position(0);
        byte[] rb = new byte[5];
        mainloop:
        while (true) {
            ret = 0; total = 0;
            ret = sPort.syncRead(rb, 5000); // trzeba coś tu wymyślić
            expected = rb[4] & 0xff;
            got = sioChecksum(rb, 4) & 0xff;

            if (checkDesync(rb, got, expected) > 0) {
                if (debug) Log.i("USB", "Apparent desync");
                if (sync_attempts < 4) {
                        sync_attempts++;
                        for (int i = 0; i < 4; i++)
                                rb[i] = rb[i+1];
                        ret = 0;
                        do {
                            ret = sPort.syncRead(t1, 5000);
                        } while (ret < 1);
                        rb[4] = t1[0];
                } else
                    continue mainloop;
            } else {
                if (debug) Log.i("USB", "No desync");

                for (int i=0; i<4; i++)
                   rbuf.put((byte)(rb[i] & 0xff));

                   if (debug) {
                       String data = "";
                       for (int i=0; i<4; i++)
                           data += Integer.toString(rb[i] & 0xff) + " ";
                       Log.i("USB", "Command Frame: " + data);
                   }
                break;
            }
         };
         if (debug) Log.i("USB", "got=" + got + " expected= " + expected);
         return 1;
    }

    public static int getHWCommandFrame(int mMethod) {
//        int mask, total_retries, status, total, res = 0;
//        boolean ret;

//        switch (mMethod) {
//            case 0:
//                mask = 64;
//                break;
//            case 1:
//                mask = 32;
//                break;
//            case 2:
//                mask = 16;
//                break;
//            default:
//                mask = 32; }

//        rbuf.position(0);
//        do {
//            status = 0; total_retries = 0;
//            do {
//                if (total_retries > 10e2) return 2;
//                status = getModemStatus();
//                total_retries += 1;

//                if (status < 0) {
//                    if (debug) Log.i("USB", "Cannot retrieve serial port status");
//                    return 0;
//                }
//            } while (!((status & mask) > 0));

//            ret = purge();
//            if (!ret) if (debug) Log.i("USB", "Cannot clear serial port");

//            total = 0; total_retries = 0;
//            do {
//                res = 0;
//                try {
//                    if (total_retries > 4) return 2;
//                    res = sPort.sread(rb, 5-total, 5000); }
//                catch (IOException e) {};

//                if (res > 0) {
//                    for (int i=0; i<res; i++) {
//                       if (debug) Log.i("USB", "CF: " + (rb[i] & 0xff));
//                       rbuf.put((byte)(rb[i] & 0xff));
//                       total += 1;
//                    }
//                } else
//                    total_retries++;
//            } while (total<5);

//            int expected = (byte) rb[4] & 0xff;
//            int got = sioChecksum(rb, 4);

//            if (expected != got) return 2;

//            total_retries = 0;
//            do {
//                if (total_retries > 10e2) return 2;
//                status = getModemStatus();
//                total_retries += 1;

//                if (status < 0) {
//                    if (debug) Log.i("USB", "Cannot retrieve serial port status");
//                    return 0;
//                }
//            } while ((status & mask) > 0);
//            break;
//         } while (true);
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

class RegisterReceiverRunnable implements Runnable
{
        // this method is called on Android Ui Thread
        @Override
        public void run() {
                IntentFilter filter = new IntentFilter();
                filter.addAction("com.android.example.USB_PERMISSION");
                // this method must be called on Android Ui Thread
                SerialActivity.s_activity.registerReceiver(new USBReceiver(), filter);
                }
}

class FileChooser implements Runnable
{
    @Override
    public void run() {
        SerialActivity.s_activity.fileChooser();
    }
}

class DirChooser implements Runnable
{
    @Override
    public void run() {
        SerialActivity.s_activity.dirChooser();
    }
}

class USBReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
        // Step 4
        if (intent.getAction().equals("com.android.example.USB_PERMISSION"))
        synchronized (this) {
            UsbDevice device = (UsbDevice)intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
            if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
                Log.v("USB", "Received permission result OK");
                if(device != null){
                    Log.i("USB", "Device OK");
                }
                else {
                    Log.i("USB", "Permission denied for device " + device);
                }
            }
        }
    }
}
