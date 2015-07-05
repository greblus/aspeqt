AspeQt, Atari Serial Peripheral Emulator for Qt

Android
=======

It's really a work in progress. Currently it supports 19200 bps only, booting atr files and xex files works. Cas loading is not working yet.

What's necessary to use it:

1. Android device with USB host (OTG) support.
2. OTG cable.
3. Sio2PC-USB (I used the one made by Lotharek, it has DSR line connected).
4. Currently it works on Android 4.2.2 (maybe earlier versions too, but probably won't work on anything older than Android 4.
5. No root necessary.
6. No drivers necessary (d2xx java driver is in the package).
7. On first run, when it will ask you to open Aspeqt's settings, you can safely say no. The settings are currently hardcoded.


IMPORTANT:
There is a bug in ftd2xx driver due to which the device can't be opened on first try, so in order to start the emulation:

    * Start AspeQt
    * Connect Sio2USB
    * Answer Yes to the permissions request
    * Click on the connection icon to start the emulation (yes, I'm planning to make 
      the original icon bigger in the future ;)).

I'll find a good solution to this soon.

For the brave ones, who would like to compile or experiment with the code:

    * Use qt-opensource-$YOUR_PLATFORM-android-5.4.2, Qt5.5 has some JNI issues. I've yet to report 
      them or ask the author to enlighten me of what's happening.
    * It won't run on KitKat or above (SIGABRT, signal 6 due to some threading issues, hopefully to be solved soon).

Summary
=======
AspeQt emulates Atari SIO peripherals when connected to an Atari 8-bit computer with an SIO2PC cable.
In that respect it's similar to programs like APE and Atari810. The main difference is that it's free
(unlike APE) and it's cross-platform (unlike Atari810 and APE).

Some features
=============
* Qt based GUI with drag and drop support.
* Cross-platform (currently Windows and x86-Linux)
* 15 disk drive emulation (drives 9-15 are only supported by SpartaDOS X)
* Text-only printer emulation with saving and printing of the output
* Cassette image playback
* Folders can be mounted as simulated Dos20s disks. (read-only, now with SDX compatibility, and bootable)
* Atari executables can be booted directly, optionally with high speed.
* Contents of image files can be viewed / changed
* AspeQt Client module (ASPECL.COM). Runs on the Atari and is used to get/set Date/Time on the Atari plus a variety of other remote tasks.
* Upto 6xSIO speed and more if the serial port adaptor supports it (FTDI chip based cables are recommanded).
* Localization support
* Multi-session support

License (see license.txt file for more details)
===============================================
Original code up to version 0.6.0 Copyright 2009 by Fatih Aygün. 
Updates since v0.6.0 Copyright 2012 by Ray Ataergin.

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


Contact
=======
Ray Ataergin, mail to ray@atari8warez.com. Please include the word "aspeqt" in the subject field.
Fatih Aygun, mail to cyco130@yahoo.com, Please include the word "aspeqt" in the subject field.

For SIO2PC/10502PC hardware purchase/support visit www.atari8warez.com
