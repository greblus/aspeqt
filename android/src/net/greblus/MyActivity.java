package net.greblus;

import android.widget.Toast;
import android.os.Bundle;
import java.lang.String;
import android.util.Log;

import org.qtproject.qt5.android.bindings.QtActivity;

import com.ftdi.j2xx.D2xxManager;
import com.ftdi.j2xx.FT_Device;

import android.content.Context;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import android.hardware.usb.UsbManager;
import android.hardware.usb.UsbDevice;
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
        public static D2xxManager ftdid2xx=null;
        public static FT_Device ftDevice = null;
        public static int devCount = 0;
        public static UsbManager manager;
        public static PendingIntent pintent;
        private static final String ACTION_USB_PERMISSION =
            "com.android.example.USB_PERMISSION";
        public static D2xxManager.FtDeviceInfoListNode devInfo;
        public static ByteBuffer bbuf = ByteBuffer.allocateDirect(16384);
        public static byte b[] = new byte [16384];
        public static int ret;
        public static int counter;
        public static UsbDevice device;
        public static native void sendBufAddr(ByteBuffer buf);
        private static boolean debug = false;
        public static String m_chosen;
        public static int m_filter;

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

        public static void runFileChooser(int filter) {
            m_chosen = "None";
            m_filter = filter;
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
            SimpleFileDialog FileOpenDialog =  new SimpleFileDialog(MyActivity.this, "FileOpen", m_filter,
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

            try {
                ftdid2xx = D2xxManager.getInstance(s_activity);
                devCount = (int)ftdid2xx.createDeviceInfoList(s_activity);

                ftdid2xx.setVIDPID(1027, 33713);
                ftdid2xx.setVIDPID(1027, 24597);
                ftdid2xx.setVIDPID(1027, 33712);

                if (!ftdid2xx.isFtDevice(device))
                    return -1;

                if (debug) Log.i("USB", "Requesting permissions");
                manager.requestPermission(device, pintent);

                        if (devCount > 0) {
                            try {
                                ftDevice = ftdid2xx.openByUsbDevice(s_activity, device);
                             } catch(NullPointerException e){
                                        if (debug) Log.i("FTDI", e.getMessage(), e);
                                    }
                                    finally {
                                        if (ftDevice == null) {
                                            if (debug) Log.i("FTDI", "No Devices opened!");
                                            return devCount;
                                        }
                                    }
                            if (ftDevice.isOpen()) {
                                devInfo = ftDevice.getDeviceInfo();
                                ftDevice.setBitMode((byte)0, D2xxManager.FT_BITMODE_RESET);

                                boolean ret = ftDevice.setFlowControl(D2xxManager.FT_FLOW_NONE, (byte) 0x00, (byte)0x00 );
                                //boolean ret = ftDevice.setFlowControl(D2xxManager.FT_FLOW_DTR_DSR, (byte) 0x00, (byte)0x00 );
                                if (debug) Log.i("FTDI", "setFlowControl " + ret);

                                ftDevice.setDataCharacteristics(D2xxManager.FT_DATA_BITS_8, D2xxManager.FT_STOP_BITS_1,
                                    D2xxManager.FT_PARITY_NONE);

                                ftDevice.clrDtr();
                                ftDevice.clrRts();

                                ftDevice.setLatencyTimer((byte)16);
                                //ftdid2xx.DriverParameters.setReadTimeout(5000);
                                ftDevice.purge((byte)(D2xxManager.FT_PURGE_TX | D2xxManager.FT_PURGE_RX));
                                ftDevice.restartInTask();

                                if (debug) Log.i("FTDI", "Opened device " + devInfo.description);
                            } else { if (debug) Log.i("FTDI", "No device opened!"); return 0; }
                        } else  if (debug) Log.i("FTDI", "No device connected!");
 		} catch (D2xxManager.D2xxException ex) {
                        ex.printStackTrace();
                        //ftDevice.close();
                    	}

                    if (debug) Log.i("FTDI", "devCount: " + devCount);
                    return devCount;
       }

     public static void ftdiCloseDevice() {
        if (ftDevice != null) ftDevice.close();
     }

     public static boolean setSpeed(int speed) {
         if (debug) Log.i("FTDI", "setBaudrate: " + speed);
         if (ftDevice == null) return false;
         return ftDevice.setBaudRate(speed);
     }

    public static int ftdiRead(int size, int total)
    {
        ret = ftDevice.read(b, size);
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
        bbuf.position(total);
        bbuf.get(b, 0, size);
        ret = ftDevice.write(b, size);
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

    public static int getQueueStatus() {
        int ret = ftDevice.getQueueStatus();
        if (debug) {
            counter +=1;
            if (counter < 3) {
                Log.i("FTDI", "getQueueStatus: " + ret );
            } else {
                 if (counter == 3) Log.i("FTDI", "getQueueStatus! called too many times!");
                 if (counter > 50000) counter = 0;
            }
        }
        return ret;
    }

    public static boolean purge() {
        boolean ret = ftDevice.purge((byte)(D2xxManager.FT_PURGE_TX | D2xxManager.FT_PURGE_RX));
        if (debug) Log.i("FTDI", "purge: " + ret);
        return ret;
    }

    public static boolean purgeTX() {
        boolean ret = ftDevice.purge((byte)(D2xxManager.FT_PURGE_TX));
        if (debug) Log.i("FTDI", "purgeTX: " + ret);
        return ret;
    }

    public static boolean purgeRX() {
        boolean ret = false;
        ret = ftDevice.purge((byte)(D2xxManager.FT_PURGE_RX));
        if (debug) Log.i("FTDI", "purgeRX: " + ret);
        return ret;
    }

    public static boolean resetDevice(int line) {
        boolean ret = false;
        try {
            ret = ftDevice.resetDevice();
            if (debug) Log.i("FTDI", "resetDevice: " + ret);
        }
        catch(Exception e) {
            if (debug) Log.i("FTDI", e.getMessage(), e);
        }
        return ret;
    }

    public static void qLog(String msg) {
        if (debug) Log.i("FTDI", msg);
    }

    public static int getModemStatus() {
        ret = ftDevice.getModemStatus();
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

    public static void restartInTask() {
        if (debug) Log.i("FTDI", "restartInTask!");
        ftDevice.restartInTask();
    }
    
    public static void stopInTask() {
        if (debug) Log.i("FTDI", "stopInTask!");
        ftDevice.stopInTask();
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
