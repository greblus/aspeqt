AspeQt, Atari Serial Peripheral Emulator for Android
====================================================

Experimental branch using modified usb-serial-for-android driver:
https://github.com/mik3y/usb-serial-for-android 

Supports FTDI's FT232 and Prolific's PL2303 chips (support for PL2303 is 
currently broken. What works for FTDI doesn't work for Prolific, have 
to find a good timing for both).

Previous version with FT232 support only using ftd2xx Java driver 
is available here: https://github.com/greblus/aspeqt/tree/ftd2xx

It's a work in progress. Currently it already handles atr and xex 
booting and cas files loading.

What's necessary to use it:
===========================

1. Android device with USB Host Support.
   Check USB Host Support with this tool:
   https://play.google.com/store/apps/details?id=eu.chainfire.usbhostdiagnostics

2. OTG cable.

3. Sio2PC-USB. I recommend this one made by Lotharek:

   http://lotharek.pl/product.php?pid=98

   it has DSR line connected and for Prolific tests I have a converter which I made 
   by myself (with RI line connected) using PL2303 according to this diagram:

   http://blog.greblus.net/2012/02/19/sio2pc-minimalistycznie/
   
4. I've tested it on Lark FreeMe 8" Android 4.2.2 tablet and Kazam 345 4.5"
   phone with KitKat 4.4 (should work on anything from 3.1 up)

5. No root necessary.

6. No drivers necessary (usb-serial-for-android java driver is in the package).

7. After you connect the SIO2PC-USB to your Android device it'll automatically
   launch AspeQt. If you have SIO2PC-USB from Lotharek, select DSR flow control
   method and set speed to 19200bps first, to see if it works at all. Feel free to 
   experiment with higher speeds and Pokey divisors (HiSpeed OS, SDX or Qmeg 
   is required for fast SIO routines). 

   Speed greatly depends on your CPU possibilities and I/O performance:
   https://www.youtube.com/watch?v=-w4mppTym1I

   My PL2303 won't open with speed higher than 34800bps and also Pokey divisors do 
   not work.

Videos of AspeQt on Android:
============================

https://www.youtube.com/watch?v=xw8QZHCt5BY&list=PL_0_k9JuZLPiox8zAklF3qvG3SbsZ08g8

Summary
=======
AspeQt emulates Atari SIO peripherals when connected to an Atari 8-bit computer with 
an SIO2PC cable. In that respect it's similar to programs like APE and Atari810. 
The main difference is that it's free (unlike APE) and it's cross-platform (unlike 
Atari810 and APE).

License (see license.txt file for more details)
===============================================
Original code up to version 0.6.0 Copyright 2009 by Fatih Aygün. 
Updates since v0.6.0 Copyright 2012 by Ray Ataergin.
Android port of aspeqt-1.0.0.Preview_6 by Wiktor Grebla.

You can freely copy, use, modify and distribute it under the GPL 2.0
license. Please see license.txt for details.

Qt libraries: Copyright 2009 Nokia Corporation and/or its subsidiary(-ies).
Used in this package under LGPL 2.0 license.

usb-serial-for-Android with support for FTDI and PL2303 chips is 
based on this project: https://github.com/mik3y/usb-serial-for-android

Silk Icons: Copyright by Mark James (famfamfam.com).
Used in this package under Creative Commons Attribution 3.0 license.

Additional Icons by Oxygen Team. Used in this package under Creative Commons 
Attribution-ShareAlike 3.0 license.

AtariSIO Linux kernel module and high speed code used in the EXE loader.
Copyright Matthias Reichl <hias@horus.com>. Used in this package under GPL 2.0 
license.

Atascii Fonts by Mark Simonson (http://members.bitstream.net/~marksim/atarimac).
Used in this package under Freeware License.

Simple Filedialog for Android:
http://www.scorchworks.com/Blog/simple-file-dialog-for-android-applications/

Contact concerning this Android port:
=====================================
Wiktor Grebla: greblus@gmail.com
For SIO2PC-USB hardware purchase visit: 
www.lotharek.pl 
or: 
http://www.atari8warez.com/
