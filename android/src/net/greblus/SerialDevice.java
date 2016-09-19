
package net.greblus;

import android.content.Intent;

public interface SerialDevice {
    public int openDevice();
    public void closeDevice();
    public int setSpeed(int speed);
    public int read(int size, int total);
    public int write(int size, int total);
    public boolean purge();
    public boolean purgeTX();
    public boolean purgeRX();
    public int getModemStatus();
    public int getSWCommandFrame();
    public int getHWCommandFrame(int mMethod);
    public void activityResult(int requestCode, int resultCode, Intent data);
}
