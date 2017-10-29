# -------------------------------------------------
# Project created by QtCreator 2009-11-22T14:13:00
# Last Update: Apr 10, 2014
# -------------------------------------------------
#CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT
DEFINES += VERSION=\\\"1.0\\\"
TARGET = AspeQt
TEMPLATE = app
CONFIG += qt
QT += core gui widgets printsupport svg
CONFIG += mobility
CONFIG += static
MOBILITY = bearer
SOURCES += main.cpp \
    mainwindow.cpp \
    sioworker.cpp \
    optionsdialog.cpp \
    aboutdialog.cpp \
    diskimage.cpp \
    diskimagepro.cpp \
    folderimage.cpp \
    miscdevices.cpp \
    createimagedialog.cpp \
    diskeditdialog.cpp \
    aspeqtsettings.cpp \
    autoboot.cpp \
    autobootdialog.cpp \
    atarifilesystem.cpp \
    miscutils.cpp \
    textprinterwindow.cpp \
    cassettedialog.cpp \
    docdisplaywindow.cpp \
    bootoptionsdialog.cpp \
    logdisplaydialog.cpp \
    pclink.cpp
win32:LIBS += -lwinmm -lz
win32:SOURCES += serialport-win32.cpp
unix:
{
    android: {
        QT += androidextras
        SOURCES += serialport-android.cpp
        HEADERS += serialport-android.h
        FORMS += \
            android/optionsdialog.ui \
            android/autobootdialog.ui \
            android/cassettedialog.ui \
            android/diskeditdialog.ui

        DISTFILES += \
            android/AndroidManifest.xml \
            android/gradle/wrapper/gradle-wrapper.jar \
            android/gradlew \
            android/res/values/libs.xml \
            android/build.gradle \
            android/gradle/wrapper/gradle-wrapper.properties \
            android/gradlew.bat

        ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
    }

    linux:!android||macx {
    SOURCES += serialport-unix.cpp
    HEADERS += serialport-unix.h
    }
}

HEADERS += mainwindow.h \
    serialport.h \
    sioworker.h \
    optionsdialog.h \
    aboutdialog.h \
    diskimage.h \
    diskimagepro.h \
    folderimage.h \
    miscdevices.h \
    createimagedialog.h \
    diskeditdialog.h \
    aspeqtsettings.h \
    autoboot.h \
    autobootdialog.h \
    atarifilesystem.h \
    miscutils.h \
    textprinterwindow.h \
    cassettedialog.h \
    docdisplaywindow.h \
    bootoptionsdialog.h \
    logdisplaydialog.h \
    pclink.h \
    atari/hsio/src/hicode.h \
    atari/hsio/src/patchrom.h \
    headers/atarisio.h

win32:HEADERS += serialport-win32.h

!android:FORMS += \
    optionsdialog.ui \
    autobootdialog.ui \
    cassettedialog.ui \
    diskeditdialog.ui \

FORMS += mainwindow.ui \
    aboutdialog.ui \
    createimagedialog.ui \
    textprinterwindow.ui \
    docdisplaywindow.ui \
    bootoptionsdialog.ui \
    logdisplaydialog.ui

RESOURCES += icons.qrc \
    atarifiles.qrc \
    i18n.qrc \
    documentation.qrc \
    images.qrc

OTHER_FILES += \
    license.txt \
    history.txt \
    atascii_read_me.txt \
    AspeQt.rc \
    about.html \
    compile.txt \

TRANSLATIONS = i18n/aspeqt_pl.ts \
               i18n/aspeqt_tr.ts \
               i18n/aspeqt_ru.ts \
               i18n/aspeqt_sk.ts \
               i18n/aspeqt_de.ts \
               i18n/aspeqt_es.ts \
               i18n/qt_pl.ts \
               i18n/qt_tr.ts \
               i18n/qt_ru.ts \
               i18n/qt_sk.ts \
               i18n/qt_de.ts \
               i18n/qt_es.ts

RC_FILE = AspeQt.rc \

DISTFILES += \
    android/src/com/hoho/android/usbserial/driver/CommonUsbSerialPort.java \
    android/src/com/hoho/android/usbserial/driver/FtdiSerialDriver.java \
    android/src/com/hoho/android/usbserial/driver/ProbeTable.java \
    android/src/com/hoho/android/usbserial/driver/UsbId.java \
    android/src/com/hoho/android/usbserial/driver/UsbSerialDriver.java \
    android/src/com/hoho/android/usbserial/driver/UsbSerialPort.java \
    android/src/com/hoho/android/usbserial/driver/UsbSerialProber.java \
    android/src/com/hoho/android/usbserial/driver/UsbSerialRuntimeException.java \
    android/src/net/greblus/SerialActivity.java \
    android/src/net/greblus/SimpleFileDialog.java \
    android/res/xml/device_filter.xml \
    android/src/com/hoho/android/usbserial/util/HexDump.java \
    android/src/com/hoho/android/usbserial/util/SerialInputOutputManager.java \
    android/src/com/hoho/android/usbserial/BuildInfo.java \
    android/src/net/greblus/SerialDevice.java \
    android/src/net/greblus/SIO2BT.java \
    android/src/net/greblus/SIO2PCUS4A.java \
    android/res/values/strings.xml \
    android/res/values-pl/strings.xml \
    $bootata/$boot.bin \
    $bootata/dos.sys \
    $bootata/dup.sys \
    $bootata/ramdisk.com \
    $bootdxl/$boot.bin \
    $bootdxl/dos.sys \
    $bootdxl/dosxl.sys \
    $bootdxl/menu.com \
    $bootmyd/$boot.bin \
    $bootmyd/dos.sys \
    $bootmyd/dup.sys \
    $bootpic/$boot.bin \
    $bootpic/picodos.sys \
    $bootsma/$boot.bin \
    $bootsma/dos.sys \
    $bootsma/dup.sys \
    $bootspa/$boot.bin \
    $bootspa/x32.dos \
    atari/aspecl/AspeCl.com \
    atari/hsio/diag/diag-ext.atr \
    atari/hsio/diag/diag.atr \
    atari/hsio/hipatch.atr \
    android/apk/testing/aspeqt.apk \
    android/apk/testing/aspeqt_x86.apk \
    android/apk/aspeqt.apk \
    android/apk/aspeqt_x86.apk \
    android/libs/android-support-v4.jar \
    aboutdialog.obj \
    atari/autoboot/autoboot.src \
    atari/autoboot/hisiocode-break.src \
    atari/autoboot/hisiocode-check.src \
    atari/autoboot/hisiocode-cleanup.src \
    atari/autoboot/hisiocode-main.src \
    atari/autoboot/hisiocode-receive.src \
    atari/autoboot/hisiocode-send.src \
    atari/autoboot/hisiocode-vbi.src \
    atari/autoboot/hisiocode.src \
    aspeqt.pro.user \
    atascii_fonts_for_PC.zip \
    android/res/drawable-hdpi/icon.png \
    android/res/drawable-ldpi/icon.png \
    android/res/drawable-mdpi/icon.png \
    main-icon/main-icon/AspeQt.png \
    main-icon/main-icon/AspeQt128.png \
    main-icon/main-icon/AspeQt64.png \
    main-icon/main-icon/main16.png \
    main-icon/AspeQt.png \
    main-icon/AspeQt128.png \
    main-icon/AspeQt64.png \
    main-icon/main-icon/AspeQt.ico \
    main-icon/AspeQt.ico \
    compile.html \
    atari/autoboot/hisio.inc \
    atari/hsio/src/cio.inc \
    atari/hsio/src/hipatch.inc \
    atari/hsio/src/hisio.inc \
    atari/hsio/src/hwdebug.inc \
    atari/hsio/LICENSE \
    atari/hsio/README.txt \
    i18n/aspeqt_de.ts \
    i18n/aspeqt_es.ts \
    i18n/aspeqt_pl.ts \
    i18n/aspeqt_ru.ts \
    i18n/aspeqt_sk.ts \
    i18n/aspeqt_tr.ts \
    android/src/com/felhr/deviceids/CH34xIds.java \
    android/src/com/felhr/deviceids/CP210xIds.java \
    android/src/com/felhr/deviceids/CP2130Ids.java \
    android/src/com/felhr/deviceids/FTDISioIds.java \
    android/src/com/felhr/deviceids/PL2303Ids.java \
    android/src/com/felhr/deviceids/XdcVcpIds.java \
    android/src/com/felhr/usbserial/BLED112SerialDevice.java \
    android/src/com/felhr/usbserial/CDCSerialDevice.java \
    android/src/com/felhr/usbserial/CH34xSerialDevice.java \
    android/src/com/felhr/usbserial/CP2102SerialDevice.java \
    android/src/com/felhr/usbserial/CP2130SpiDevice.java \
    android/src/com/felhr/usbserial/FTDISerialDevice.java \
    android/src/com/felhr/usbserial/PL2303SerialDevice.java \
    android/src/com/felhr/usbserial/SerialBuffer.java \
    android/src/com/felhr/usbserial/SerialInputStream.java \
    android/src/com/felhr/usbserial/SerialOutputStream.java \
    android/src/com/felhr/usbserial/UsbSerialDebugger.java \
    android/src/com/felhr/usbserial/UsbSerialDevice.java \
    android/src/com/felhr/usbserial/UsbSerialInterface.java \
    android/src/com/felhr/usbserial/UsbSpiDevice.java \
    android/src/com/felhr/usbserial/UsbSpiInterface.java \
    android/src/com/felhr/usbserial/XdcVcpSerialDevice.java \
    android/src/com/felhr/utils/HexData.java \
    atari/autoboot/Makefile \
    atari/hsio/src/Makefile \
    README.md \
    android/gradle.properties \
    android/local.properties





