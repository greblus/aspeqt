package net.greblus;

import android.os.Bundle;
import java.lang.String;
import android.util.Log;

import org.qtproject.qt5.android.bindings.QtActivity;

import com.ftdi.j2xx.D2xxManager;
import com.ftdi.j2xx.FT_Device;
import com.ftdi.j2xx.FT_EEPROM_232R;
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

    public static MyActivity s_activity = null;

        // every time you override a method, always make sure you
	// then call super method as well
	@Override
	public void onCreate(Bundle savedInstanceState)
 	{
		s_activity = this;
                super.onCreate(savedInstanceState);

                manager = (UsbManager)getSystemService(Context.USB_SERVICE);
                pintent = PendingIntent.getBroadcast(this, 0, new Intent(ACTION_USB_PERMISSION), 0);
                registerBroadcastReceiver();

                sendBufAddr(bbuf);
        }

        @Override
	protected void onDestroy()
	{
                super.onDestroy();
                s_activity = null;
                ftdiCloseDevice();
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

            if (deviceIterator.hasNext())
                device = deviceIterator.next();
            else
                return 0;

            while (deviceIterator.hasNext()) {
                if ((device.getProductId()) == 24577 && (device.getVendorId() == 1027))
                    break;
                device = deviceIterator.next();
            }
            if (debug) Log.i("USB", "Requesting permissions");

            manager.requestPermission(device, pintent);

            try {
                ftdid2xx = D2xxManager.getInstance(s_activity);
                devCount = (int)ftdid2xx.createDeviceInfoList(s_activity);

                        if (devCount > 0) {
                            try {
                                ftDevice = ftdid2xx.openByUsbDevice(s_activity, device);
                             } catch(NullPointerException e){
                                        if (debug) Log.i("FTDI", e.getMessage(), e);
                                    }
                                    finally {
                                        if (ftDevice == null) {
                                            if (debug) Log.i("FTDI", "No Devices found");
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

                                ftDevice.setLatencyTimer((byte)5);
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
        ftDevice.close();
     }

     public static boolean setSpeed(int speed) {
         if (debug) Log.i("FTDI", "setBaudrate: " + speed);
         return ftDevice.setBaudRate(speed);
     }

    public static int ftdiRead(int size, int total) {

    if (!ftDevice.isOpen()) {
        ftdiOpenDevice();
    }

    ret = ftDevice.read(b, size);
    if (debug) Log.i("FTDI", "ftdiRead() size: " + size + " total: " + total + " ret: " + ret);

    if (ret > 0) {

        bbuf.position(total);
        for (int i=0; i<ret; i++)
            bbuf.put((byte) (b[i] & 0xff));

        //bbuf.put(b, total, ret);

        if (debug) {
            String tmp = "Java side buf = ";
            for (int i=0; i<size-1; i++)
                {
                    tmp +=  Integer.toString(0xff & b[i]) + ", ";
                }
                tmp = tmp.substring(0, tmp.length()-2);
            Log.i("FTDI", tmp);
        }
    }

    return ret;

    }

    public static int ftdiWrite(int size, int total) {

        if (!ftDevice.isOpen()) {
            ftdiOpenDevice();
        }

        bbuf.position(total);
        for (int i=0; i<size; i++)
            b[i] = (byte) (bbuf.get(i) & 0xff);

        ret = ftDevice.write(b, size, true);

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

    public static int getQueueStatus(int line) {

        if (!ftDevice.isOpen()) {
            ftdiOpenDevice();
        }

        int ret = ftDevice.getQueueStatus();

        if (debug) {
            counter +=1;
            if (counter < 3) {
                Log.i("FTDI", "getQueueStatus " + line + " " + ret );
            } else {
                 if (counter == 3) Log.i("FTDI", "getQueueStatus! called too many times!");
                 if (counter > 50000) counter = 0;
            }
        }
        return ret;
    }

    public static boolean purge(int line) {

        if (!ftDevice.isOpen()) {
            ftdiOpenDevice();
        }

        boolean ret = ftDevice.purge((byte)(D2xxManager.FT_PURGE_TX | D2xxManager.FT_PURGE_RX));
        if (debug) Log.i("FTDI", "purge " + line + " " + ret);
        return ret;
    }

    public static boolean purgeTX(int line) {

        if (!ftDevice.isOpen()) {
            ftdiOpenDevice();
        }

        boolean ret = ftDevice.purge((byte)(D2xxManager.FT_PURGE_TX));
        if (debug) Log.i("FTDI", "purgeTX " + line + " " + ret);
        return ret;
    }

    public static boolean purgeRX(int line) {

        boolean ret = false;

        if (!ftDevice.isOpen()) {
            ftdiOpenDevice();
        }

        ret = ftDevice.purge((byte)(D2xxManager.FT_PURGE_RX));

        if (debug) Log.i("FTDI", "purgeRX " + line + " " + ret);

        return ret;
    }

    public static boolean resetDevice(int line) {

        boolean ret = false;

        if (!ftDevice.isOpen()) {
            ftdiOpenDevice();
        }
        try {
            ret = ftDevice.resetDevice();
            if (debug) Log.i("FTDI", "resetDevice " + line + " " + ret);
        }
        catch(Exception e) {
            if (debug) Log.i("FTDI", e.getMessage(), e);
        }

        return ret;
    }


    public static void qLog(String msg) {

        if (debug) Log.i("FTDI", msg);
    }

    public static int getModemStatus(int line) {

        if (!ftDevice.isOpen()) {
            ftdiOpenDevice();
        }

        ret = ftDevice.getModemStatus();

        if (debug) {
            counter +=1;
            if (counter < 3) {
                Log.i("FTDI", "getModemStatus " + line + ": " + ret);
            } else {
                 if (counter == 3 ) Log.i("FTDI", "getModemStatus called too many times!");
                 if (counter > 50000) counter = 0;
            }
        }
        return ret;
    }

    public static void restartInTask() {
        if (!ftDevice.isOpen()) {
            ftdiOpenDevice();
        }

        if (debug) Log.i("FTDI", "restartInTask!");
        ftDevice.restartInTask();
    }
    
    public static void stopInTask() {
        if (!ftDevice.isOpen()) {
            ftdiOpenDevice();
        }

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
                    Log.i("USB", "permission denied for device " + device);
                }
            }
        }
    }
}
