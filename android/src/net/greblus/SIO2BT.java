package net.greblus;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import java.util.UUID;
import java.io.InputStream;
import java.io.OutputStream;
import android.util.Log;
import android.content.Intent;

import java.util.Set;
import java.io.IOException;
import java.lang.System;

public class SIO2BT implements SerialDevice {

    private static int counter;
    private static boolean debug = true;
    private static BluetoothAdapter m_BluetoothAdapter = null;
    private static BluetoothDevice m_device = null;
    private static BluetoothSocket m_socket = null;
    private static UUID uuid;
    private static InputStream m_input = null;
    private static OutputStream m_output = null;
    private final static int REQUEST_ENABLE_BT = 1;
    public static SerialActivity sa = SerialActivity.s_activity;

    SIO2BT() {

        m_BluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        if (m_BluetoothAdapter == null) {
            Log.i("BT", "No BT adapter found!");
        }
        else
            if (!m_BluetoothAdapter.isEnabled()) {
                Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
                sa.startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT);
            }
    }

    public void activityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == REQUEST_ENABLE_BT) {
            if (resultCode == 0) {
                Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
                sa.startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT);
            }
        }
    }

    public int openDevice() {

        if (m_BluetoothAdapter == null)
            return 0;

        while (!m_BluetoothAdapter.isEnabled()) { };

        m_BluetoothAdapter.cancelDiscovery();
        Set<BluetoothDevice> pairedDevices = m_BluetoothAdapter.getBondedDevices();
        if (pairedDevices.size() > 0) {
            for (BluetoothDevice device : pairedDevices) {
                if (device.getName().startsWith("SIO2BT")) {
                    Log.i("BT", device.getName());
                    m_device = device;
                    break;
                } else
                    m_device = null;
             }
        }

        if (m_device == null) return 0;

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

    public void closeDevice() {
        if (m_socket != null) {
            try {
                m_socket.close();
            } catch (IOException e) { }
        }
    }

    public int setSpeed(int speed) {
        return speed;
    }

    public int waitTillAvailable(int timeout) {
        int avail = 0;
        long t_now, t_start;
        t_start = System.currentTimeMillis();

        while (true) {
            try { avail = m_input.available(); }
            catch (Exception e) {}

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

    public int read(int size, int total)
    {
        int rd = 0;
        try {
            sa.rbuf.position(total);
            rd = m_input.read(sa.rb, 0, size);
            sa.rbuf.put(sa.rb, 0, rd);
        } catch (IOException e) {}
    return rd;
    }

    public int write(int size, int total)
    {
        sa.wbuf.position(total);
        sa.wbuf.get(sa.wb, 0, size);

        try {
            m_output.write(sa.wb, 0, size);
            m_output.flush();
        } catch (IOException e) { size = 0; }

        return size;
    }

    public boolean purge() {
        return true;
    }

    public boolean purgeTX() {
        return true;
    }

    public boolean purgeRX() {
        return true;
    }

    public int getModemStatus() {
        int ret = -2;
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
                        int avail = waitTillAvailable(40);
                        if (avail == 0)
                            return 2;
                        else
                            ret = m_input.read(sa.rb, 0, avail);
                } catch (IOException e) { }
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

            if (debug) {
               String data = "";
               for (int i=0; i<4; i++)
                   data += Integer.toString(sa.rb[i] & 0xff) + " ";
               Log.i("USB", "Desync Frame: " + data);
            }

            if (sync_attempts < 10) {
                sync_attempts++;
                for (int i = 0; i < 4; i++)
                        sa.rb[i] = sa.rb[i+1];

                int avail = waitTillAvailable(10);
                if (avail > 0) {
                    try {
                        ret = m_input.read(sa.t, 0, 1);
                    } catch (IOException e) { }
                    if (ret > 0) sa.rb[4] = sa.t[0];
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

