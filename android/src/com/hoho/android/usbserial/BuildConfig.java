package com.hoho.android.usbserial;

// Minimal stand-in for the gradle-generated BuildConfig. The upstream library
// module generates this class per build, but here the usb-serial-for-android
// sources are compiled as part of the AspeQt app, whose real BuildConfig lives
// in another package (org.qtproject.example.AspeQt). Providing this lets the
// vendored drivers (Ch34x/Prolific reference BuildConfig.DEBUG) compile without
// modifying any upstream file. Keep across library updates.
public final class BuildConfig {
    public static final boolean DEBUG = false;

    private BuildConfig() {}
}
