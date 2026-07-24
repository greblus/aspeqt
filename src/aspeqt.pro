# -------------------------------------------------
# Project created by QtCreator 2009-11-22T14:13:00
# Last Update: Apr 10, 2014
# -------------------------------------------------
#CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT
DEFINES += VERSION=\\\"1.2\\\"
TARGET = AspeQt
TEMPLATE = app
CONFIG += qt
QT += core gui svg
# network: the R: device (850 emulation) bridges the Atari to TCP.
QT += network
CONFIG += mobility
CONFIG += static
MOBILITY = bearer

# The UI is Qt Quick (src/qml/), driven by AppController in qmlbridge.cpp; the
# engine itself (engine.cpp) is headless and knows nothing about it.
QT += quick qml quickdialogs2
SOURCES += qmlbridge.cpp
HEADERS += qmlbridge.h
RESOURCES += qml.qrc
SOURCES += main.cpp \
    engine.cpp \
    sioworker.cpp \
    diskimage.cpp \
    diskimagepro.cpp \
    folderimage.cpp \
    miscdevices.cpp \
    aspeqtsettings.cpp \
    autoboot.cpp \
    atarifilesystem.cpp \
    miscutils.cpp \
    printeroutput.cpp \
    rdevice.cpp \
    phonebook.cpp \
    tnfsclient.cpp \
    ftpclient.cpp \
    networkbrowser.cpp \
    pclink.cpp
win32:LIBS += -lwinmm -lz
win32:SOURCES += serialport-win32.cpp
unix:
{
    android: {
        # Google Play target API requirement (Android 16 / API 36 from 2026-08-31)
        ANDROID_TARGET_SDK_VERSION = 36
        ANDROID_MIN_SDK_VERSION = 28
        # Android 15+ requires native libs aligned to 16 KB ELF LOAD segments.
        QMAKE_LFLAGS += -Wl,-z,max-page-size=16384 -Wl,-z,common-page-size=16384
        SOURCES += serialport-android.cpp
        HEADERS += serialport-android.h

        # Qt dlopens libssl/libcrypto at runtime but may not ship them, so bundle
        # the Qt-recommended KDAB prebuilts. We build arm64-v8a only, hence the
        # explicit pair rather than including their all-ABI openssl.pri.
        # Override with: qmake ANDROID_OPENSSL_DIR=/path/to/android_openssl
        isEmpty(ANDROID_OPENSSL_DIR): ANDROID_OPENSSL_DIR = $$PWD/../../android_openssl
        exists($$ANDROID_OPENSSL_DIR/ssl_3/arm64-v8a/libssl_3.so) {
            ANDROID_EXTRA_LIBS += \
                $$ANDROID_OPENSSL_DIR/ssl_3/arm64-v8a/libcrypto_3.so \
                $$ANDROID_OPENSSL_DIR/ssl_3/arm64-v8a/libssl_3.so
        } else {
            warning("OpenSSL not found under $$ANDROID_OPENSSL_DIR -- TLS and SSH will be unavailable. Clone https://github.com/KDAB/android_openssl")
        }

        # libssh, cross-built static (Android will not load versioned .so, and a
        # static lib keeps it out of the package). See src/doc/build-libssh.md.
        # Override with: qmake LIBSSH_SRC=... LIBSSH_BUILD=...
        isEmpty(LIBSSH_SRC):   LIBSSH_SRC   = $$PWD/../../libssh
        isEmpty(LIBSSH_BUILD): LIBSSH_BUILD = $$PWD/../../build-libssh-arm64
        exists($$LIBSSH_BUILD/src/libssh.a) {
            DEFINES     += HAVE_LIBSSH
            INCLUDEPATH += $$LIBSSH_SRC/include $$LIBSSH_BUILD/include
            SOURCES     += sshclient.cpp sftpclient.cpp
            HEADERS     += sshclient.h sftpclient.h
            # Order matters: libssh.a first, then the crypto it depends on.
            LIBS        += $$LIBSSH_BUILD/src/libssh.a \
                           $$ANDROID_OPENSSL_DIR/ssl_3/arm64-v8a/libssl.so \
                           $$ANDROID_OPENSSL_DIR/ssl_3/arm64-v8a/libcrypto.so
        } else {
            warning("libssh.a not found at $$LIBSSH_BUILD -- SSH dialling will be unavailable. See src/doc/build-libssh.md")
        }

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
    LIBS += -lz
    }
}

HEADERS += engine.h \
    serialport.h \
    sioworker.h \
    diskimage.h \
    diskimagepro.h \
    folderimage.h \
    miscdevices.h \
    aspeqtsettings.h \
    autoboot.h \
    atarifilesystem.h \
    miscutils.h \
    printeroutput.h \
    rdevice.h \
    rdevice_handler.h \
    phonebook.h \
    bbsdata.h \
    inetworkclient.h \
    tnfsclient.h \
    ftpclient.h \
    networkbrowser.h \
    pclink.h

win32:HEADERS += serialport-win32.h


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
    android/src/com/hoho/android/usbserial/BuildConfig.java \
    android/src/com/hoho/android/usbserial/driver/CdcAcmSerialDriver.java \
    android/src/com/hoho/android/usbserial/driver/Ch34xSerialDriver.java \
    android/src/com/hoho/android/usbserial/driver/ChromeCcdSerialDriver.java \
    android/src/com/hoho/android/usbserial/driver/CommonUsbSerialPort.java \
    android/src/com/hoho/android/usbserial/driver/Cp21xxSerialDriver.java \
    android/src/com/hoho/android/usbserial/driver/FtdiSerialDriver.java \
    android/src/com/hoho/android/usbserial/driver/GsmModemSerialDriver.java \
    android/src/com/hoho/android/usbserial/driver/ProbeTable.java \
    android/src/com/hoho/android/usbserial/driver/ProlificSerialDriver.java \
    android/src/com/hoho/android/usbserial/driver/SerialTimeoutException.java \
    android/src/com/hoho/android/usbserial/driver/UsbId.java \
    android/src/com/hoho/android/usbserial/driver/UsbSerialDriver.java \
    android/src/com/hoho/android/usbserial/driver/UsbSerialPort.java \
    android/src/com/hoho/android/usbserial/driver/UsbSerialProber.java \
    android/src/net/greblus/SerialActivity.java \
    android/src/net/greblus/SimpleFileDialog.java \
    android/res/xml/device_filter.xml \
    android/src/com/hoho/android/usbserial/util/HexDump.java \
    android/src/com/hoho/android/usbserial/util/MonotonicClock.java \
    android/src/com/hoho/android/usbserial/util/SerialInputOutputManager.java \
    android/src/com/hoho/android/usbserial/util/UsbUtils.java \
    android/src/com/hoho/android/usbserial/util/XonXoffFilter.java \
    android/src/net/greblus/SerialDevice.java \
    android/src/net/greblus/SIO2BT.java \
    android/src/net/greblus/SIO2PCUS4A.java \
    android/res/values/strings.xml \
    android/res/values-pl/strings.xml





