AspeQt, Atari Serial Peripheral Emulator for Qt

Android
=======

It's a work in progress. Currently it already supports booting atr, xex 
and cas file loading.

What's necessary to use it:

1. Android device with USB Host Support.
   Check USB Host Support with this tool:
   https://play.google.com/store/apps/details?id=eu.chainfire.usbhostdiagnostics
2. OTG cable.
3. Sio2PC-USB (I used the one made by Lotharek, it has DSR line connected).
4. I've tested it on Lark FreeMe 8" Android 4.2.2 tablet and Kazam 345 4.5"
   phone with KitKat 4.4 (should work on anything from 3.1 up)
5. No root necessary.
6. No drivers necessary (d2xx java driver is in the package).
7. After you connect the SIO2PC-USB to your Android device it'll automatically
   launch AspeQt. If you have SIO2PC-USB from Lotharek, select DSR flow control
   method and set speed to 19200bps first, to see if it works at all. Feel free to 
   experiment with higher speeds and Pokey divisors (HiSpeed OS, SDX or Qmeg 
   is required for fast SIO routines). 
   Speed greatly depends on your device CPU speed and I/O performance:
   https://www.youtube.com/watch?v=-w4mppTym1I

For the brave ones, who would like to compile or experiment with the code:

    * Use qt-opensource-windows-android-5.4.2, on Windows Qt5.5 has some JNI issues. 
      On Linux Qt5.5 works flawlessly.

some more videos of AspeQt on Android:

https://www.youtube.com/watch?v=4nPa-w399y0
https://www.youtube.com/watch?v=c8BRUNzVarc
https://www.youtube.com/watch?v=JZhYxHLi-I8
https://www.youtube.com/watch?v=x4FylJJehdA

Summary
=======
AspeQt emulates Atari SIO peripherals when connected to an Atari 8-bit computer with an SIO2PC cable.
In that respect it's similar to programs like APE and Atari810. The main difference is that it's free
(unlike APE) and it's cross-platform (unlike Atari810 and APE).

License (see license.txt file for more details)
===============================================
Original code up to version 0.6.0 Copyright 2009 by Fatih Aygün. 
Updates since v0.6.0 Copyright 2012 by Ray Ataergin.
Android port of aspeqt-1.0.0.Preview_6 by Wiktor Grebla.

You can freely copy, use, modify and distribute it under the GPL 2.0
license. Please see license.txt for details.

Qt libraries: Copyright 2009 Nokia Corporation and/or its subsidiary(-ies).
Used in this package under LGPL 2.0 license.

Silk Icons: Copyright by Mark James (famfamfam.com).
Used in this package under Creative Commons Attribution 3.0 license.

Additional Icons by Oxygen Team. Used in this package under Creative Commons Attribution-ShareAlike 3.0 license.

AtariSIO Linux kernel module and high speed code used in the EXE loader.
Copyright Matthias Reichl <hias@horus.com>. Used in this package under GPL 2.0 license.

Atascii Fonts by Mark Simonson (http://members.bitstream.net/~marksim/atarimac).
Used in this package under Freeware License.

Contact concerning this Android port:
====================================

Wiktor Grebla: greblus@gmail.com

For SIO2PC-USB hardware purchase visit: 

www.lotharek.pl 

or: 

http://www.atari8warez.com/
