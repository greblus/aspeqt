package net.greblus;

import android.widget.Toast;
import android.os.Bundle;
import java.lang.String;
import java.util.List;
import android.util.Log;

import org.qtproject.qt5.android.bindings.QtActivity;

import com.hoho.android.usbserial.driver.UsbSerialPort;
import com.hoho.android.usbserial.driver.UsbSerialProber;
import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialPort;
import com.hoho.android.usbserial.driver.UsbId;

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

public class MyActivity extends QtActivity
{
        public static int devCount = 0;
        public static UsbManager manager;
        public static PendingIntent pintent;
        private static final String ACTION_USB_PERMISSION =
            "com.android.example.USB_PERMISSION";
        public static ByteBuffer bbuf = ByteBuffer.allocateDirect(16384);
        public static byte b[] = new byte [16384];
        public static int ret;
        public static int counter;
        public static UsbDevice device;
        public static UsbSerialDriver driver;
        public static UsbSerialPort sPort;
        public static native void sendBufAddr(ByteBuffer buf);
        private static boolean debug = false;
        public static String m_chosen;
        public static int m_filter;
        public static String m_action;

        public static MyActivity s_activity = null;

        @Override
	public void onCreate(Bundle savedInstanceState)
 	{
		s_activity = this;
                super.onCreate(savedInstanceState);

                manager = (UsbManager)getSystemService(Context.USB_SERVICE);
                pintent = PendingIntent.getBroadcast(this, 0, new Intent(ACTION_USB_PERMISSION), 0);
                registerBroadcastReceiver();
                getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

                sendBufAddr(bbuf);
        }

        public static void runFileChooser(int filter, int action) {
            m_chosen = "None";
            m_filter = filter;

            if (action == 0)
                m_action = "FileOpen";
            else
                m_action = "FileSave";

            MyActivity.s_activity.runOnUiThread( new FileChooser() );

         }

         public static void runDirChooser() {
            m_chosen = "None";
            m_filter = 0;
            MyActivity.s_activity.runOnUiThread( new DirChooser() );

          }

        @Override
	protected void onDestroy()
	{
                super.onDestroy();
                s_activity = null;
                ftdiCloseDevice();
	}

        public void fileChooser()
        {
            SimpleFileDialog FileOpenDialog =  new SimpleFileDialog(MyActivity.this, m_action, m_filter,
                                                            new SimpleFileDialog.SimpleFileDialogListener()
            {
                    @Override
                    public void onChosenDir(String chosenDir)
                    {
                            // The code in this function will be executed when the dialog OK button is pushed
                            m_chosen = chosenDir;
                            if (m_chosen != "Cancelled") Toast.makeText(MyActivity.this, "Chosen File: " +
                                            m_chosen, Toast.LENGTH_LONG).show();
                    }
            });

            //You can change the default filename using the public variable "Default_File_Name"
            FileOpenDialog.Default_File_Name = "";
            FileOpenDialog.chooseFile_or_Dir();
        }

        public void dirChooser()
        {
            SimpleFileDialog FileOpenDialog =  new SimpleFileDialog(MyActivity.this, "FolderChoose", m_filter,
                                                            new SimpleFileDialog.SimpleFileDialogListener()
            {
                    @Override
                    public void onChosenDir(String chosenDir)
                    {
                            // The code in this function will be executed when the dialog OK button is pushed
                            m_chosen = chosenDir;
                            if (m_chosen != "Cancelled") Toast.makeText(MyActivity.this, "Chosen Directory: " +
                                            m_chosen, Toast.LENGTH_LONG).show();
                    }
            });

            FileOpenDialog.chooseFile_or_Dir();
        }

        public static void registerBroadcastReceiver() {
                if (MyActivity.s_activity != null) {
                        // Qt is running on a different thread than Android. (step 2)
                        // In order to register the receiver we need to execute it in the UI thread
                        MyActivity.s_activity.runOnUiThread( new RegisterReceiverRunnable());
                        Log.i("USB", "Broadcast Receiver registered");
                }
        }

        public static int ftdiOpenDevice() {
            HashMap<String, UsbDevice> deviceList = manager.getDeviceList();
            Iterator<UsbDevice> deviceIterator = deviceList.values().iterator();

            if (!deviceIterator.hasNext())
                return 0;

            int ft2xx_pid;
            do {
                device = deviceIterator.next();
                ft2xx_pid = device.getProductId();
                if ((device.getVendorId() == 1027) &&
                   (
                     (ft2xx_pid == 24577) || //Lotharek's Sio2PC-USB
                     (ft2xx_pid == 33712) || //Ray's Sio2USB-1050PC
                     (ft2xx_pid == 33713) ||
                     (ft2xx_pid == 24597)    //Ray's Sio2PC-USB
                   )
                )
                    break;
            } while (deviceIterator.hasNext());

            List<UsbSerialDriver> availableDrivers = UsbSerialProber.getDefaultProber().findAllDrivers(manager);

            if (availableDrivers.isEmpty()) {
                Log.i("FTDI", "No drivers found for attached usb devices");
                return 0;
            }

            Log.i("FTDI", "Driver found for attached usb device");
            driver = availableDrivers.get(0);

            Log.i("FTDI", "Requesting permissions");
            //device = driver.getDevice();
            manager.requestPermission(device, pintent);
            UsbDeviceConnection connection = manager.openDevice(device);

            if (connection == null) Log.i("USB", "No permissions to open device");

            sPort = driver.getPorts().get(0);

            try {
                sPort.open(connection);
                sPort.setParameters(19200, 8, UsbSerialPort.STOPBITS_1, UsbSerialPort.PARITY_NONE);
            } catch (IOException e) {
                Log.i("USB", "Can't open port");
            }
            Log.i("USB", "Device opened");
            return 1;
       }

     public static void ftdiCloseDevice() {
         try {
            if (sPort != null) sPort.close();
         } catch (IOException e) {
            Log.i("USB", "Can't close port");
        }
     }

     public static boolean setSpeed(int speed) {
         boolean ret = true;
         if (debug) Log.i("FTDI", "setBaudrate: " + speed);
         try {
            sPort.setBaudRate(speed);
         } catch (IOException e) {
           Log.i("USB", "Can't set speed");
           ret = false;
       }
       return ret;
     }

    public static int ftdiRead(int size, int total)
    {
        int ret = 0;
        try {
            ret = sPort.read(b, 5000);
        } catch (IOException e) {
           Log.i("USB", "Can't read");
        }

        if (debug) Log.i("FTDI", "ftdiRead() size: " + size + " total: " + total + " ret: " + ret);
        if (ret > 0) {
            bbuf.position(total);
            bbuf.put(b, 0, ret);

            if (debug) {
                String tmp = "Java side buf = ";
                for (int i=0; i<size-1; i++) {
                    tmp +=  Integer.toString(0xff & b[i]) + ", ";
                }
                tmp = tmp.substring(0, tmp.length()-2);
                Log.i("FTDI", tmp);
            }
        }
        return ret;
    }

    public static int ftdiWrite(int size, int total) {
        int ret = 0;
        byte[] tb = new byte[size];

        bbuf.position(total);
        bbuf.get(tb, 0, size);

        try {
            ret = sPort.write(tb, 1000);
        } catch (IOException e) {
           Log.i("USB", "Can't write");
       }
        if (ret > 0) {
            if (debug) {
                String tmp = "Java side ftdiWrite(): ret= " + Integer.toString(ret) + " data: ";
                for (int i=0; i<size; i++)
                {
                    tmp +=  Integer.toString(b[i] & 0xff)  + " ";

                }
                Log.i("FTDI", tmp);
            }
        }
        return ret;
    }

    public static boolean purge() {
        boolean ret;
        try {
            ret = sPort.purgeHwBuffers(true, true);
        } catch (IOException e) {
            Log.i("USB", "Can't purge");
            ret = false;
        }
        if (debug) Log.i("FTDI", "purge: " + ret);
        return ret;
    }

    public static boolean purgeTX() {
        boolean ret;
        try {
            ret = sPort.purgeHwBuffers(false, true);
        } catch (IOException e) {
            Log.i("USB", "Can't purge TX buffer");
            ret = false;
        }
        if (debug) Log.i("FTDI", "purgeTX: " + ret);
        return ret;
    }

    public static boolean purgeRX() {
        boolean ret;
        try {
            ret = sPort.purgeHwBuffers(true, false);
        } catch (IOException e) {
            Log.i("USB", "Can't purge RX buffer");
            ret = false;
       }
        if (debug) Log.i("FTDI", "purgeRX: " + ret);
        return ret;
    }

    public static void qLog(String msg) {
        if (debug) Log.i("FTDI", msg);
    }

    public static int getModemStatus() {
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
                Log.i("FTDI", "getModemStatus: " + ret);
            } else {
                 if (counter == 3 ) Log.i("FTDI", "getModemStatus called too many times!");
                 if (counter > 50000) counter = 0;
            }
        }
        return ret;
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
                MyActivity.s_activity.registerReceiver(new USBReceiver(), filter);
                }
}

class FileChooser implements Runnable
{
    @Override
    public void run() {
        MyActivity.s_activity.fileChooser();
    }
}

class DirChooser implements Runnable
{
    @Override
    public void run() {
        MyActivity.s_activity.dirChooser();
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
