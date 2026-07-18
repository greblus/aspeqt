# AspeQt — Atari SIO Peripheral Emulator for Android

**AspeQt** emulates 8-bit Atari SIO peripherals (disk drives, cassette, printer,
PCLINK host access) on Android. It talks to a real Atari through one of two serial
interfaces:

* **SIO2PC-USB** via an FTDI adapter (USB OTG host support is required), or
* **SIO2BT** via a SIO2BT bluetooth dongle, supported by most modern Android devices.

> **Since 1.2** the interface is written in **Qt Quick / QML** (Material Design):
> the same drive-panel layout, but built for touch throughout. The engine behind it
> — SIO, mounting, sessions, the cassette and executable loader — is unchanged and
> now runs headless, with no widget code left anywhere in the app.

<p align="center">
  <img src="src/screenshots/aspeqt_qml.jpg" alt="AspeQt QML redesign: loader slot, drive slots and log" width="42%">
</p>

## What's new in the Android port

The Android build has been reworked around a touch-friendly drive panel.

### Dynamic drive slots

* You start with **5 drive slots** and can add more with the **`+`** row at the
  bottom — up to the hardware limit of **15** (device numbers `D1:`–`D15:`).
* To remove a slot, empty it first (its **eject** button turns into a **red ✗**);
  pressing it again drops that slot. Removing a slot **never renumbers the
  others**, so the drive numbers your DOS expects stay put.
* When the slots don't all fit on screen the list **scrolls with your finger**.
* The **number and contents of the slots are saved** in the default session and
  in any session you save to a file, so your setup is restored on the next start.

### CAS / XEX loader slot

A dedicated **loader slot sits at the top** of the list and replaces the old
"Boot executable" / "Play cassette" menu entries:

* The **load** button (gear icon) opens a file picker for Atari programs
  (`.xex` / `.com` / `.exe`) and cassette images (`.cas`).
* An **executable** is booted straight into the Atari using autoboot loader; the
  slot fills up as a progress bar while the program loads.
* A **cassette** arms the **play** button — do whatever your Atari needs
  (`CLOAD`, or reboot holding *Option*+*Start*) and, when you hear the beep, press
  **play** while pressing a key on the Atari. The slot shows the tape length,
  e.g. *Cassette (5:05)*, and fills as it plays.
* **retry** re-runs the last load, **eject** stops playback / boot and clears the slot.

### One-tap high-speed DOS for folders

When you mount a **folder** as a virtual disk, the third slot button turns into a
hard-disk icon labelled **DOS**. Tapping it copies **MyPicoDOS** (`$boot.bin` +
`picodos.sys`) into the folder — MyPicoDOS boots at high speed without any OS
modifications and is a great `.xex` loader too.

## SIO2PC-USB

* Software command-frame detection (**SOFT**, the default handshake method) works
  with anything at 19200 bps and behaves very well at non-standard speeds and with
  the POKEY divisor set to, say, 0 or 1 (SOFT mode is faster).
* Try binary-file loading with the **"use high-speed executable loader"** option
  turned on.

### What you need

1. An Android device with USB host support.
2. An OTG cable.
3. A SIO2PC-USB converter — the one [made by Lotharek](https://lotharek.pl/productdetail.php?id=108) is recommended.
4. **No root required.**
5. **No drivers required** — the [usb-serial-for-android](https://github.com/greblus/usb-serial-for-android)
   Java driver is bundled.
6. Connecting a SIO2PC-USB adapter launches AspeQt automatically. With Lotharek's
   adapter, start with the **DSR** or **SOFT** flow-control method at **19200 bps**
   to confirm it works, then experiment with higher speeds and POKEY divisors
   (a high-speed OS, SDX or QMEG is required for the fast SIO routines).

## SIO2BT

My favourite SIO2BT devices are available from
[Marcin "The Montezuma" Sochacki](https://www.facebook.com/Sio2bt/), or you can
build your own following [this simple diagram](http://atarionline.pl/cn/data/upimages/bluetooth_03.jpg).

**The U1MB [PBI BIOS](http://atari8.co.uk/firmware/) supports higher baud rates,
up to 57600 bps, and SIO2BT can work with an unmodified Atari OS.**

Pair your Android device with the SIO2BT dongle and enter the name of your
bluetooth device in the Options window.

On an unmodified Atari, SIO2BT requires a
[special loader](http://abbuc.de/~montezuma/SIO2BT.zip) or a modified OS (with the
ACK timeout increased). Speed, name and PIN can be configured with
[BTCONFIG](http://www.mr-atari.com/Mr.Atari/SIO2BT/BTCONFIG.XEX) by Mr Atari, or
[BTCFG](http://atariage.com/forums/index.php?app=core&module=attach&section=attach&attach_id=468608) by FJC.

## Downloads

* **AspeQt** will soon be available again from [Google Play Store](https://play.google.com/store/search?q=aspeqt).
* All releases (including older ones) are on the [releases page](https://github.com/greblus/aspeqt/releases).
* The latest compiled APK is always at
  [`apk/aspeqt.apk`](https://github.com/greblus/aspeqt/raw/ng/apk/aspeqt.apk).

## Under the hood

The 1.2 release also cleared out a lot of history. The UI is Qt Quick end to end,
so the QtWidgets and print-support modules are gone from the build, along with every
`.ui` form, the old dialogs and the widget-era window handling.

## Where to get the adapters

For SIO2BT see the
[SIO2BT ordering thread on AtariAge](http://atariage.com/forums/topic/241984-sio2bt-ordering-thread/),
or build your own.

## License

See `license.txt` for the full text.

* Original code up to version 0.6.0 — Copyright 2009 by Fatih Aygün.
* Updates since v0.6.0 — Copyright 2012 by Ray Ataergin.
* Android port of aspeqt-1.0.0.Preview_6 by Wiktor Grębla.

You may freely copy, use, modify and distribute it under the **GPL 2.0** license.

### Credits

* Qt libraries — Copyright by The Qt Company.
* usb-serial-for-android (with FTDI support) is based on
  [mik3y/usb-serial-for-android](https://github.com/mik3y/usb-serial-for-android).
* [Tango Icons](http://tango.freedesktop.org/Tango_Icon_Library).
* The high-speed code used in the EXE loader was written by Matthias Reichl —
  <http://www.horus.com/~hias/atari/>.
* PCLINK by TheMontezuma — <https://github.com/TheMontezuma/RespeQt>.

## Contact

Android port: Wiktor Grębla — greblus@gmail.com
