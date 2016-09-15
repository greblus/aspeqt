package net.greblus;

import java.lang.System;
import android.widget.Toast;
import android.os.Bundle;
import java.lang.String;
import java.util.List;
import android.util.Log;
import java.util.Set;

import org.qtproject.qt5.android.bindings.QtActivity;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;

import java.util.UUID;
import java.io.InputStream;
import java.io.OutputStream;
import android.provider.Settings.Secure;
import android.content.Intent;
import android.app.Activity;

import android.content.Context;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import android.widget.Toast;
import android.view.WindowManager;

public class SerialActivity extends QtActivity
{
        private static ByteBuffer rbuf = ByteBuffer.allocateDirect(65535);
        private static ByteBuffer wbuf = ByteBuffer.allocateDirect(65535);
        private static byte rb[] = new byte [65535];
        private static byte wb[] = new byte [65535];
        private static byte t[] = new byte [1024];
        private static int counter;
        public static native void sendBufAddr(ByteBuffer rbuf, ByteBuffer wbuf);
        private static boolean debug = false;
        public static String m_chosen;
        private static int m_filter;
        private static String m_action;
        private static String m_dir;
        private static BluetoothAdapter mBluetoothAdapter = null;
        private static BluetoothDevice m_device = null;
        private static BluetoothSocket m_socket = null;
        private static UUID uuid;
        private static InputStream m_input = null;
        private static OutputStream m_output = null;
        private static int rd_delay1 = 50;
        private static int rd_delay2 = 40;
        private static int rd_delay3 = 10;
        private final static int REQUEST_ENABLE_BT = 1;
        public static SerialActivity s_activity = null;

        @Override
	public void onCreate(Bundle savedInstanceState)
 	{
            s_activity = this;
            super.onCreate(savedInstanceState);
            getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
            sendBufAddr(rbuf, wbuf);

            mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
            if (mBluetoothAdapter == null) {
                Log.i("BT", "No BT adapter found!");
            }
            else
                if (!mBluetoothAdapter.isEnabled()) {
                    Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
                    startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT);
                }
        }

        @Override
        public void onPause() {
           m_chosen = "Cancelled";
           super.onPause();
        }

        @Override
        protected void onActivityResult(int requestCode, int resultCode, Intent data) {
            if (requestCode == REQUEST_ENABLE_BT) {
                if (resultCode == 0) {
                    Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
                    startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT);
                }
            }
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

        public static int openDevice() {

            if (mBluetoothAdapter == null)
                return 0;

            while (!mBluetoothAdapter.isEnabled()) { };

            mBluetoothAdapter.cancelDiscovery();
            Set<BluetoothDevice> pairedDevices = mBluetoothAdapter.getBondedDevices();
            if (pairedDevices.size() > 0) {
                outer:
                for (BluetoothDevice device : pairedDevices) {
                    if (device.getName().startsWith("SIO2BT")) {
                        Log.i("BT", device.getName());
                        m_device = device;
                        break outer;
                    } else
                        m_device = null;
                }
            }
            if (m_device == null)
                return 0;

            uuid = UUID.fromString("00001101-0000-1000-8000-00805f9b34fb");
            BluetoothSocket tmp = null;
            try {
                    tmp = m_device.createRfcommSocketToServiceRecord(uuid);
            } catch (IOException e) { }
            m_socket = tmp;

            if (m_socket != null)
                try {
                    m_socket.connect();
                } catch (IOException e) { }

            if (m_socket.isConnected()) {
                try {
                    m_input = m_socket.getInputStream();
                    m_output = m_socket.getOutputStream();
                } catch (IOException e) { }

                Log.i("BT", "Device opened");
                return 1;
            } else {
                Log.i("BT", "Device not connected");
                return 0;
            }
       }

     public static void closeDevice() {
        if (m_socket != null) {
            try {
                m_socket.close();
            } catch (IOException e) { }
        }
    }

     public static int setSpeed(int speed) {
        return speed;
     }

    public static int read(int size, int total)
    {
        rbuf.position(total);
        int ret = 0, rd = 0;       
        long t_now, t_start;
        t_start = System.currentTimeMillis();

        do {
            try {
               int available = 0;
                while (true) {                
                    available = m_input.available();
                    if (available > 0) break;

                    try { Thread.sleep(1);
                    } catch (Exception e) { }

                    t_now = System.currentTimeMillis();
                    if (t_now-t_start > rd_delay1)
                        return 0;
                }
                Log.i("USB", Integer.toString(available));
                rd = m_input.read(rb, 0, available);
                rbuf.put(rb, 0, rd);
                size -= rd; ret += rd;
            } catch (IOException e) {}
        } while (size > 0);
        return ret;
    }

    public static int write(int size, int total) {

        wbuf.position(total);
        wbuf.get(wb, 0, size);

        try {
            m_output.write(wb, 0, size);
        } catch (IOException e) { size = 0; }

        return size;
    }

    public static boolean purge() {        
        return true;
    }

    public static boolean purgeTX() {
        return true;
    }

    public static boolean purgeRX() {        
        return true;
    }

    public static void qLog(String msg) {
        if (debug) Log.i("USB", msg);
    }

    public static int getModemStatus() {
        int ret = -2;
        return ret;
    }


    public static int getSWCommandFrame() {
        int expected = 0, sync_attempts = 0, got = 1, total_retries = 0;
        int ret = 0, total = 0;
        boolean prtl = false;
        long t_now, t_start;
        rbuf.position(0);        
        mainloop:
        while (true) {
            ret = 0; total = 0; total_retries = 0;
            do {
                if (total_retries > 2) return 2;                
                try {
                   int available = 0;
                   t_start = System.currentTimeMillis();
                   while (true) {
                        available = m_input.available();
                        if (available > 0) break;

                        try { Thread.sleep(1);
                        } catch (Exception e) { }

                        t_now = System.currentTimeMillis();
                        if (t_now-t_start > rd_delay2)
                            return 2;
                    }
                    ret = m_input.read(rb, 0, available);
                } catch (IOException e) { }
                if (ret == 5) break;
                if ((ret > 0) && (ret < 5)) {
                    System.arraycopy(rb, 0, t, total, ret);
                    prtl = true;
                    total += ret;
                }
                if ((total == 5) && (prtl == true))
                        System.arraycopy(t, 0, rb, 0, 5);
                if (ret <= 0)
                    total_retries++;
        } while (total<5);

        expected = rb[4] & 0xff;
        got = sioChecksum(rb, 4) & 0xff;

        if (checkDesync(rb, got, expected) > 0) {
            if (debug) Log.i("USB", "Apparent desync");

            if (debug) {
               String data = "";
               for (int i=0; i<4; i++)
                   data += Integer.toString(rb[i] & 0xff) + " ";
               Log.i("USB", "Desync Frame: " + data);
            }
            if (sync_attempts < 10) {
                sync_attempts++;
                for (int i = 0; i < 4; i++)
                        rb[i] = rb[i+1];
                ret = 0;
                byte tmp = rb[0];                
                try {
                    int available = 0;
                    t_start = System.currentTimeMillis();
                    while (true) {
                        available = m_input.available();
                        if (available > 0) break;
                        try { Thread.sleep(1);
                        } catch (Exception e) { }
                        t_now = System.currentTimeMillis();
                        if (t_now-t_start > rd_delay3)
                            return 2;
                    }
                    ret = m_input.read(t, 0, 1);
                } catch (IOException e) { }
                rb[4] = t[0];
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
