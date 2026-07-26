package net.greblus;

import org.greblus.AspeQt.R;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import java.util.UUID;
import java.io.InputStream;
import java.io.OutputStream;
import android.util.Log;
import android.content.Intent;
import android.widget.Toast;

import java.util.Set;
import java.io.IOException;
import java.lang.System;

public class SIO2BT implements SerialDevice
{
    private boolean debug = false;
    private BluetoothAdapter m_BluetoothAdapter = null;
    private BluetoothDevice m_device = null;
    private BluetoothSocket m_socket = null;
    private UUID uuid;
    private InputStream m_input = null;
    private OutputStream m_output = null;
    private SerialActivity sa = SerialActivity.s_activity;

    SIO2BT() {
        m_BluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        // Nothing else here: this runs from onCreate(), and enable() would need
        // a permission we have not asked for yet (SecurityException on API 31+).
        if (m_BluetoothAdapter == null)
            Toast.makeText(sa, sa.getResources().getString(R.string.bt_module_not_present), Toast.LENGTH_SHORT).show();
    }

    private void toast(final int resId, final int length) {
        sa.runOnUiThread(new Runnable() {
            public void run() {
                Toast.makeText(sa, sa.getResources().getString(resId), length).show();
            }
        });
    }

    public int openDevice() {
        if (m_BluetoothAdapter == null) {
            Log.i("BT", "No BT adapter found!");
            return 0;
        }

        // API 31+ gates everything below this line behind BLUETOOTH_CONNECT.
        if (!SerialActivity.ensureBluetoothPermission()) {
            Log.i("BT", "BLUETOOTH_CONNECT not granted");
            toast(R.string.bt_no_permission, Toast.LENGTH_LONG);
            return 0;
        }

        // enable() is a no-op for apps since API 33, so the old "spin until the
        // adapter comes up" loop would hang forever with Bluetooth off. Ask the
        // user with the system dialog instead and let them start again.
        if (!m_BluetoothAdapter.isEnabled()) {
            sa.runOnUiThread(new Runnable() {
                public void run() {
                    try {
                        sa.startActivity(new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE));
                    } catch (Exception e) {
                        Log.i("BT", "cannot ask to enable BT: " + e);
                    }
                }
            });
            toast(R.string.bt_turn_on, Toast.LENGTH_LONG);
            return 0;
        }

        // No cancelDiscovery() here: we never start discovery, and the call
        // needs BLUETOOTH_SCAN -- which is what used to throw right here.
        m_device = null;
        try {
            Set<BluetoothDevice> pairedDevices = m_BluetoothAdapter.getBondedDevices();
            for (BluetoothDevice device : pairedDevices) {
                final String name = device.getName();
                if (name != null && name.equals(sa.bluetoothName)) {
                    if (debug) Log.i("BT", name);
                    m_device = device;
                    break;
                }
            }
        } catch (SecurityException e) {
            Log.i("BT", "getBondedDevices: " + e);
        }

        if (m_device == null) {
            sa.runOnUiThread(new Runnable() {
                public void run() {
                    Toast.makeText(sa, sa.getResources().getString(R.string.bt_module_check), Toast.LENGTH_SHORT).show();
                }
            });
        return 0;
        }

        uuid = UUID.fromString("00001101-0000-1000-8000-00805f9b34fb");
        BluetoothSocket tmp = null;
        try {
                tmp = m_device.createRfcommSocketToServiceRecord(uuid);
        } catch (IOException e) {
            Log.i("BT", "createRfcommSocket: " + e);
        } catch (SecurityException e) {
            Log.i("BT", "createRfcommSocket: " + e);
        }
        m_socket = tmp;

        if (m_socket != null)
        {
            toast(R.string.bt_try_connecting, Toast.LENGTH_LONG);

            try {
                m_socket.connect();
            } catch (Exception e) {
                // Was silently swallowed, and the isConnected() below then threw
                // NullPointerException on a socket that was never created.
                Log.i("BT", "connect: " + e);
                try { m_socket.close(); } catch (IOException ignored) { }
                m_socket = null;
            }
        }

        if (m_socket != null && m_socket.isConnected()) {
            try {
                m_input = m_socket.getInputStream();
                m_output = m_socket.getOutputStream();
            } catch (IOException e) { }
            if (debug)
                Log.i("BT", "Device opened");
                sa.runOnUiThread(new Runnable() {
                    public void run() {
                        Toast.makeText(sa, sa.getResources().getString(R.string.bt_connected), Toast.LENGTH_LONG).show();
                    }
                });
            return 1;
        } else {
            if (debug)
                Log.i("BT", "Device not connected");
            sa.runOnUiThread(new Runnable() {
                public void run() {
                    Toast.makeText(sa, sa.getResources().getString(R.string.bt_failed_connecting), Toast.LENGTH_LONG).show();
                }
            });
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

    // Stream (modem) mode read: whatever is available now, no fill loop. Streamed
    // R: over bluetooth is unsupported anyway (no COMMAND line), but keep the
    // interface honest.
    public int readStream(int maxSize, int mMethod)
    {
        int rd = 0;
        try {
            int avail = m_input.available();
            if (avail <= 0) return 0;
            sa.rbuf.position(0);
            rd = m_input.read(sa.rb, 0, Math.min(avail, maxSize));
            if (rd > 0) sa.rbuf.put(sa.rb, 0, rd);
        } catch (IOException e) {}
        return rd < 0 ? 0 : rd;
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

    // Bluetooth carries no modem status lines, so COMMAND cannot be read at
    // all here. The R: device's stream mode relies on this, and therefore
    // only works over SIO2PC-USB.
    public boolean isCommandAsserted(int mMethod) {
         return false;
    }

    public void resetCommandLatch() {
    }

    public void armFrameResync() {
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


