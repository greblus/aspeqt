package net.greblus;

import org.greblus.AspeQt.R;
import net.greblus.SerialDevice;

import android.widget.Toast;
import android.os.Bundle;
import java.lang.String;
import java.util.List;
import android.util.Log;

import org.qtproject.qt.android.bindings.QtActivity;

import android.hardware.usb.UsbManager;
import android.hardware.usb.UsbDevice;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import android.widget.Toast;
import android.view.WindowManager;

import android.os.Build;
import android.content.pm.PackageManager;
import android.Manifest;

public class SerialActivity extends QtActivity
{
        private static boolean debug = false;
        protected static byte rb[] = new byte [65535];
        protected static byte wb[] = new byte [65535];
        protected static byte t[] = new byte [1024];
        public static native void sendBufAddr(ByteBuffer rbuf, ByteBuffer wbuf);
        // Result of a SAF pick, delivered to the engine (empty when cancelled).
        public static native void documentPicked(int reqId, String uri);
        private static final int SAF_REQ_BASE = 0x5AF0;
        private static int m_safReqId = 0;
        protected static ByteBuffer rbuf = ByteBuffer.allocateDirect(65535);
        protected static ByteBuffer wbuf = ByteBuffer.allocateDirect(65535);
        public static String m_chosen;
        private static int m_filter;
        private static String m_action;
        private static String m_dir;
        protected static SerialActivity s_activity = null;
        private static SerialDevice m_device = null;
        private static int m_serial = 0;
        public static String bluetoothName;
        // Signalled when the async USB-permission result arrives, so openDevice()
        // can wait for it instead of racing ahead and failing to open.
        public static java.util.concurrent.CountDownLatch usbPermissionLatch;

        @Override
	public void onCreate(Bundle savedInstanceState)
 	{            
            s_activity = this;
            super.onCreate(savedInstanceState);
            if (m_serial == 0)
                m_device = new SIO2PCUS4A();
            else
                m_device = new SIO2BT();

            registerBroadcastReceiver();

            getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
            applyImmersive();
            sendBufAddr(rbuf, wbuf);
        }

        @Override
        public void onResume() {
            super.onResume();
            applyImmersive();
        }

        @Override
        public void onWindowFocusChanged(boolean hasFocus) {
            super.onWindowFocusChanged(hasFocus);
            // Immersive-sticky bars come back on their own after a swipe / focus
            // change, so re-hide them whenever we regain focus. Qt re-shows the
            // bars shortly after its own window setup, so re-apply deferred too.
            if (hasFocus) {
                applyImmersive();
                final android.view.View dv = getWindow().getDecorView();
                dv.postDelayed(new Runnable() { public void run() { applyImmersive(); } }, 300);
                dv.postDelayed(new Runnable() { public void run() { applyImmersive(); } }, 800);
            }
        }

        // Go edge-to-edge full-screen: hide the status/navigation bars (the
        // minimise/back/recents controls) and draw under the display cutout.
        private void applyImmersive() {
            try {
                android.view.Window w = getWindow();
                if (Build.VERSION.SDK_INT >= 30) {
                    w.setDecorFitsSystemWindows(false);
                    android.view.WindowInsetsController c = w.getInsetsController();
                    if (c != null) {
                        c.hide(android.view.WindowInsets.Type.systemBars());
                        c.setSystemBarsBehavior(android.view.WindowInsetsController
                                .BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
                    }
                    android.view.WindowManager.LayoutParams lp = w.getAttributes();
                    lp.layoutInDisplayCutoutMode = android.view.WindowManager.LayoutParams
                            .LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS;
                    w.setAttributes(lp);
                    // Qt 6.9+ re-enables edge-to-edge (bars shown) after its own
                    // window setup and on every rotation, so re-hide the bars each
                    // time insets are (re)applied.
                    w.getDecorView().setOnApplyWindowInsetsListener(
                        new android.view.View.OnApplyWindowInsetsListener() {
                            @Override
                            public android.view.WindowInsets onApplyWindowInsets(
                                    android.view.View v, android.view.WindowInsets insets) {
                                android.view.WindowInsetsController cc = v.getWindowInsetsController();
                                if (cc != null) {
                                    cc.hide(android.view.WindowInsets.Type.systemBars());
                                    cc.setSystemBarsBehavior(android.view.WindowInsetsController
                                            .BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
                                }
                                return v.onApplyWindowInsets(insets);
                            }
                        });
                } else {
                    w.getDecorView().setSystemUiVisibility(
                            android.view.View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                            | android.view.View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                            | android.view.View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                            | android.view.View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                            | android.view.View.SYSTEM_UI_FLAG_FULLSCREEN
                            | android.view.View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
                    android.view.WindowManager.LayoutParams lp = w.getAttributes();
                    lp.layoutInDisplayCutoutMode = android.view.WindowManager.LayoutParams
                            .LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
                    w.setAttributes(lp);
                }
            } catch (Throwable e) {}
        }

        @Override
        public void onPause() {
           m_chosen = "Cancelled";
           super.onPause();
        }

        // Own SAF pickers. Qt's file dialog validates the document it returns
        // by stat()ing a partly-decoded URI, so any name with a space or
        // bracket is dropped as "not existing"; going through the intent
        // directly also keeps Android's exact URI encoding, which the content
        // resolver needs.
        // True when Android started us because the SIO cable was plugged in
        // (the manifest has a USB_DEVICE_ATTACHED filter), so the engine can
        // begin emulating without the user having to press start.
        public static boolean launchedByUsb() {
            if (s_activity == null)
                return false;
            Intent i = s_activity.getIntent();
            return i != null && UsbManager.ACTION_USB_DEVICE_ATTACHED.equals(i.getAction());
        }

        public static void pickDocument(int reqId, String mimeType) {
            m_safReqId = reqId;
            Intent i = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            i.addCategory(Intent.CATEGORY_OPENABLE);
            i.setType(mimeType == null || mimeType.length() == 0 ? "*/*" : mimeType);
            i.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION
                     | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
            s_activity.startActivityForResult(i, SAF_REQ_BASE);
        }

        public static void createDocument(int reqId, String mimeType, String suggestedName) {
            m_safReqId = reqId;
            Intent i = new Intent(Intent.ACTION_CREATE_DOCUMENT);
            i.addCategory(Intent.CATEGORY_OPENABLE);
            i.setType(mimeType == null || mimeType.length() == 0 ? "*/*" : mimeType);
            if (suggestedName != null && suggestedName.length() > 0)
                i.putExtra(Intent.EXTRA_TITLE, suggestedName);
            i.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION
                     | Intent.FLAG_GRANT_WRITE_URI_PERMISSION
                     | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
            s_activity.startActivityForResult(i, SAF_REQ_BASE + 1);
        }

        public static void pickFolder(int reqId) {
            m_safReqId = reqId;
            Intent i = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
            i.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION
                     | Intent.FLAG_GRANT_WRITE_URI_PERMISSION
                     | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
            s_activity.startActivityForResult(i, SAF_REQ_BASE + 2);
        }

        @Override
        protected void onActivityResult(int requestCode, int resultCode, Intent data) {
            if (requestCode >= SAF_REQ_BASE && requestCode <= SAF_REQ_BASE + 2) {
                String uri = "";
                if (resultCode == RESULT_OK && data != null && data.getData() != null)
                    uri = data.getData().toString();
                documentPicked(m_safReqId, uri);
                return;
            }
            super.onActivityResult(requestCode, resultCode, data);
        }

        public static void runFileChooser(int filter, int action, String dir) {
            Log.i("ASPEQT:", "DIR:" + dir);
            m_chosen = "None";
            m_filter = filter;
            m_dir = dir;

            if (action == 0)
                m_action = "FileOpen";
            else
                m_action = "FileSave";

            SerialActivity.s_activity.runOnUiThread( new FileChooser() );

         }

         public static void runDirChooser(String dir) {
            m_dir = dir;
            m_chosen = "None";
            m_filter = 0;
            SerialActivity.s_activity.runOnUiThread( new DirChooser() );

          }

        @Override
	protected void onDestroy()
	{
                super.onDestroy();
                s_activity = null;
                m_device.closeDevice();
	}

        public void fileChooser()
        {
            SimpleFileDialog FileOpenDialog =  new SimpleFileDialog(SerialActivity.this, m_action, m_filter, m_dir,
                                                            new SimpleFileDialog.SimpleFileDialogListener()
            {
                    @Override
                    public void onChosenDir(String chosenDir)
                    {
                            // The code in this function will be executed when the dialog OK button is pushed
                            m_chosen = chosenDir;
                            if (m_chosen != "Cancelled") Toast.makeText(SerialActivity.this, getResources().getString(R.string.file_chosen) +
                                            m_chosen, Toast.LENGTH_LONG).show();
                    }
            });

            //You can change the default filename using the public variable "Default_File_Name"
            FileOpenDialog.Default_File_Name = "";
            FileOpenDialog.chooseFile_or_Dir();
        }

        public void dirChooser()
        {
            SimpleFileDialog FileOpenDialog =  new SimpleFileDialog(SerialActivity.this, "FolderChoose", m_filter, m_dir,
                                                            new SimpleFileDialog.SimpleFileDialogListener()
            {
                    @Override
                    public void onChosenDir(String chosenDir)
                    {
                            // The code in this function will be executed when the dialog OK button is pushed
                            m_chosen = chosenDir;
                            if (m_chosen != "Cancelled") Toast.makeText(SerialActivity.this, getResources().getString(R.string.dir_chosen) +
                                            m_chosen, Toast.LENGTH_LONG).show();
                    }
            });

            FileOpenDialog.chooseFile_or_Dir();
        }

        public static void registerBroadcastReceiver() {
                if (SerialActivity.s_activity != null) {
                        // Qt is running on a different thread than Android.
                        // In order to register the receiver we need to execute it in the UI thread
                        SerialActivity.s_activity.runOnUiThread( new RegisterReceiverRunnable());
                        Log.i("USB", "Broadcast Receiver registered");
                }
        }

        public static void changeDevice(int type)
        {
            m_device.closeDevice();
            if (type == 1)
                m_device = new SIO2BT();
            else
                m_device = new SIO2PCUS4A();
        }

        // Native Android toast, callable from C++ (JNI). Runs on the UI thread.
        public static void showToast(final String msg)
        {
            if (s_activity == null) return;
            s_activity.runOnUiThread(new Runnable() {
                public void run() {
                    Toast.makeText(s_activity, msg, Toast.LENGTH_LONG).show();
                }
            });
        }

        // Resolve the human-readable file name of a content:// URI (SAF picks
        // return opaque URIs with no usable path), for showing in slot labels.
        public static String displayName(String uri) {
            try {
                android.net.Uri u = android.net.Uri.parse(uri);
                android.database.Cursor c = s_activity.getContentResolver()
                        .query(u, null, null, null, null);
                if (c != null) {
                    try {
                        int idx = c.getColumnIndex(android.provider.OpenableColumns.DISPLAY_NAME);
                        if (idx >= 0 && c.moveToFirst()) {
                            String name = c.getString(idx);
                            if (name != null) return name;
                        }
                    } finally { c.close(); }
                }
            } catch (Throwable e) {}
            return "";
        }

        // Persist access to a SAF content:// URI across app restarts, so a saved
        // session can re-mount the same document later.
        public static void takePersistable(String uri, boolean write) {
            try {
                android.net.Uri u = android.net.Uri.parse(uri);
                int flags = android.content.Intent.FLAG_GRANT_READ_URI_PERMISSION;
                if (write)
                    flags |= android.content.Intent.FLAG_GRANT_WRITE_URI_PERMISSION;
                s_activity.getContentResolver().takePersistableUriPermission(u, flags);
            } catch (Throwable e) {}
        }

        // --- Storage Access Framework directory support (folder images) ---
        // FolderImage needs a real filesystem path, and Qt can't enumerate a
        // content://tree, so we copy the picked tree into a local temp dir,
        // mount that, and copy changed files back to the tree on eject.

        // Display name (folder name) of a tree URI.
        public static String treeDisplayName(String treeUri) {
            try {
                android.content.ContentResolver r = s_activity.getContentResolver();
                android.net.Uri tree = android.net.Uri.parse(treeUri);
                String docId = android.provider.DocumentsContract.getTreeDocumentId(tree);
                android.net.Uri docUri = android.provider.DocumentsContract.buildDocumentUriUsingTree(tree, docId);
                android.database.Cursor c = r.query(docUri, new String[]{
                        android.provider.DocumentsContract.Document.COLUMN_DISPLAY_NAME}, null, null, null);
                if (c != null) {
                    try { if (c.moveToFirst()) return c.getString(0); }
                    finally { c.close(); }
                }
            } catch (Throwable e) {}
            return "";
        }

        // Copy the (flat) files of a tree into a local directory. Returns the
        // number of files copied, or -1 on error.
        public static int copyTreeToDir(String treeUri, String destPath) {
            int count = 0;
            try {
                android.content.ContentResolver r = s_activity.getContentResolver();
                android.net.Uri tree = android.net.Uri.parse(treeUri);
                String docId = android.provider.DocumentsContract.getTreeDocumentId(tree);
                android.net.Uri children = android.provider.DocumentsContract.buildChildDocumentsUriUsingTree(tree, docId);
                android.database.Cursor c = r.query(children, new String[]{
                        android.provider.DocumentsContract.Document.COLUMN_DOCUMENT_ID,
                        android.provider.DocumentsContract.Document.COLUMN_DISPLAY_NAME,
                        android.provider.DocumentsContract.Document.COLUMN_MIME_TYPE}, null, null, null);
                if (c == null) return -1;
                try {
                    while (c.moveToNext()) {
                        String childId = c.getString(0);
                        String name = c.getString(1);
                        String mime = c.getString(2);
                        if (android.provider.DocumentsContract.Document.MIME_TYPE_DIR.equals(mime))
                            continue;   // folder images are flat; skip sub-dirs
                        android.net.Uri childUri = android.provider.DocumentsContract.buildDocumentUriUsingTree(tree, childId);
                        java.io.InputStream in = r.openInputStream(childUri);
                        java.io.FileOutputStream out = new java.io.FileOutputStream(destPath + "/" + name);
                        byte[] buf = new byte[65536];
                        int n;
                        while ((n = in.read(buf)) > 0) out.write(buf, 0, n);
                        out.close();
                        in.close();
                        count++;
                    }
                } finally { c.close(); }
            } catch (Throwable e) { return -1; }
            return count;
        }

        // Copy the files of a local directory back into a tree (create or
        // overwrite by name). Returns the number written, or -1 on error.
        public static int copyDirToTree(String srcPath, String treeUri) {
            int count = 0;
            try {
                android.content.ContentResolver r = s_activity.getContentResolver();
                android.net.Uri tree = android.net.Uri.parse(treeUri);
                String parentId = android.provider.DocumentsContract.getTreeDocumentId(tree);
                android.net.Uri parentUri = android.provider.DocumentsContract.buildDocumentUriUsingTree(tree, parentId);
                android.net.Uri children = android.provider.DocumentsContract.buildChildDocumentsUriUsingTree(tree, parentId);
                java.util.HashMap<String, String> existing = new java.util.HashMap<String, String>();
                android.database.Cursor c = r.query(children, new String[]{
                        android.provider.DocumentsContract.Document.COLUMN_DOCUMENT_ID,
                        android.provider.DocumentsContract.Document.COLUMN_DISPLAY_NAME}, null, null, null);
                if (c != null) {
                    try { while (c.moveToNext()) existing.put(c.getString(1), c.getString(0)); }
                    finally { c.close(); }
                }
                java.io.File[] files = new java.io.File(srcPath).listFiles();
                if (files != null) for (java.io.File f : files) {
                    if (!f.isFile()) continue;
                    String name = f.getName();
                    android.net.Uri target;
                    if (existing.containsKey(name))
                        target = android.provider.DocumentsContract.buildDocumentUriUsingTree(tree, existing.get(name));
                    else
                        target = android.provider.DocumentsContract.createDocument(r, parentUri, "application/octet-stream", name);
                    if (target == null) continue;
                    java.io.FileInputStream in = new java.io.FileInputStream(f);
                    java.io.OutputStream out = r.openOutputStream(target, "wt");
                    byte[] buf = new byte[65536];
                    int n;
                    while ((n = in.read(buf)) > 0) out.write(buf, 0, n);
                    out.close();
                    in.close();
                    count++;
                }
            } catch (Throwable e) { return -1; }
            return count;
        }

        // Read a content:// document into a local file via ContentResolver.
        // Qt's QFile can't open some SAF URIs (files in sub-folders on the
        // external-storage provider), so this is the reliable read path.
        // Returns bytes copied, or -1 on error.
        public static int copyUriToFile(String uri, String destPath) {
            try {
                android.net.Uri u = android.net.Uri.parse(uri);
                java.io.InputStream in = s_activity.getContentResolver().openInputStream(u);
                if (in == null) return -1;
                java.io.FileOutputStream out = new java.io.FileOutputStream(destPath);
                byte[] buf = new byte[65536];
                int n, total = 0;
                while ((n = in.read(buf)) > 0) { out.write(buf, 0, n); total += n; }
                out.close();
                in.close();
                return total;
            } catch (Throwable e) { return -1; }
        }

        // Enumerate the flat file list of a picked tree, one record per line
        // "displayName\tdocumentUri\tsize" (sub-dirs skipped). Metadata only, no
        // file data is read, so this is cheap even for large collections. The
        // folder image opens each documentUri on demand via openFd.
        public static String listTree(String treeUri) {
            StringBuilder sb = new StringBuilder();
            try {
                android.content.ContentResolver r = s_activity.getContentResolver();
                android.net.Uri tree = android.net.Uri.parse(treeUri);
                String docId = android.provider.DocumentsContract.getTreeDocumentId(tree);
                android.net.Uri children = android.provider.DocumentsContract.buildChildDocumentsUriUsingTree(tree, docId);
                android.database.Cursor c = r.query(children, new String[]{
                        android.provider.DocumentsContract.Document.COLUMN_DOCUMENT_ID,
                        android.provider.DocumentsContract.Document.COLUMN_DISPLAY_NAME,
                        android.provider.DocumentsContract.Document.COLUMN_MIME_TYPE,
                        android.provider.DocumentsContract.Document.COLUMN_SIZE}, null, null, null);
                if (c != null) {
                    try {
                        while (c.moveToNext()) {
                            String id = c.getString(0);
                            String name = c.getString(1);
                            String mime = c.getString(2);
                            long size = c.isNull(3) ? 0 : c.getLong(3);
                            if (android.provider.DocumentsContract.Document.MIME_TYPE_DIR.equals(mime))
                                continue;
                            if (name == null || name.indexOf('\t') >= 0 || name.indexOf('\n') >= 0)
                                continue;
                            android.net.Uri childUri = android.provider.DocumentsContract.buildDocumentUriUsingTree(tree, id);
                            sb.append(name).append('\t').append(childUri.toString()).append('\t').append(size).append('\n');
                        }
                    } finally { c.close(); }
                }
            } catch (Throwable e) {}
            return sb.toString();
        }

        // Document uri of a direct child of the tree by display name, or "".
        public static String childInTree(String treeUri, String name) {
            try {
                android.content.ContentResolver r = s_activity.getContentResolver();
                android.net.Uri tree = android.net.Uri.parse(treeUri);
                String docId = android.provider.DocumentsContract.getTreeDocumentId(tree);
                android.net.Uri children = android.provider.DocumentsContract.buildChildDocumentsUriUsingTree(tree, docId);
                android.database.Cursor c = r.query(children, new String[]{
                        android.provider.DocumentsContract.Document.COLUMN_DOCUMENT_ID,
                        android.provider.DocumentsContract.Document.COLUMN_DISPLAY_NAME}, null, null, null);
                if (c != null) {
                    try {
                        while (c.moveToNext()) {
                            if (name.equals(c.getString(1)))
                                return android.provider.DocumentsContract
                                        .buildDocumentUriUsingTree(tree, c.getString(0)).toString();
                        }
                    } finally { c.close(); }
                }
            } catch (Throwable e) {}
            return "";
        }

        // Find a child by name, creating an empty document if absent. Returns its
        // document uri, or "". Used for the DOS marker files a folder mount writes.
        public static String ensureInTree(String treeUri, String name) {
            String existing = childInTree(treeUri, name);
            if (!existing.isEmpty()) return existing;
            try {
                android.content.ContentResolver r = s_activity.getContentResolver();
                android.net.Uri tree = android.net.Uri.parse(treeUri);
                String parentId = android.provider.DocumentsContract.getTreeDocumentId(tree);
                android.net.Uri parentUri = android.provider.DocumentsContract.buildDocumentUriUsingTree(tree, parentId);
                android.net.Uri created = android.provider.DocumentsContract.createDocument(
                        r, parentUri, "application/octet-stream", name);
                if (created != null) return created.toString();
            } catch (Throwable e) {}
            return "";
        }

        // Open a content:// document as a real, seekable file descriptor and hand
        // its ownership to native code (detachFd). Mode "r"/"w"/"rw"/"wt". The
        // native side wraps it in a QFile (AutoCloseHandle) and closes it. Lets
        // us read/write the picked file in place, with no copy to app storage.
        public static int openFd(String uri, String mode) {
            try {
                android.net.Uri u = android.net.Uri.parse(uri);
                android.os.ParcelFileDescriptor pfd =
                        s_activity.getContentResolver().openFileDescriptor(u, mode);
                if (pfd == null) return -1;
                return pfd.detachFd();
            } catch (Throwable e) { return -1; }
        }

        public static int openDevice() {
            return m_device.openDevice();
        }

        public static void closeDevice() {
            m_device.closeDevice();
        }

        public static int setSpeed(int speed) {
            return m_device.setSpeed(speed);
        }

        public static int read(int size, int total)
        {
            int rd = m_device.read(size, total);
            return rd;
        }

        public static int readStream(int maxSize, int mMethod)
        {
            return m_device.readStream(maxSize, mMethod);
        }

        public static int write(int size, int total)
        {
            m_device.write(size, total);
            return size;
        }

        public static boolean purge() {
            return m_device.purge();
        }

        public static boolean purgeTX() {
            return m_device.purgeTX();
        }

        public static boolean purgeRX() {
            return m_device.purgeRX();
        }

        public static void qLog(String msg) {
            if (debug) Log.i("USB", msg);
        }

        // System-bar + display-cutout insets (in physical px), packed as
        // (left<<48)|(top<<32)|(right<<16)|bottom. The C++ side reads this and
        // insets the Qt window so content isn't hidden under the status/nav bars
        // (targetSdk 35+ forces edge-to-edge; the surface is always full-screen).
        public static long systemBarInsets() {
            try {
                if (s_activity == null) return 0;
                android.view.View dv = s_activity.getWindow().getDecorView();
                android.view.WindowInsets wi = dv.getRootWindowInsets();
                if (wi == null) return 0;
                long l, t, r, b;
                if (Build.VERSION.SDK_INT >= 30) {
                    // Only the system bars, NOT the display cutout: we deliberately
                    // draw full-screen under the camera notch (immersive mode also
                    // hides the bars, so these are usually 0 anyway).
                    android.graphics.Insets in = wi.getInsets(
                            android.view.WindowInsets.Type.systemBars());
                    l = in.left; t = in.top; r = in.right; b = in.bottom;
                } else {
                    l = wi.getSystemWindowInsetLeft();
                    t = wi.getSystemWindowInsetTop();
                    r = wi.getSystemWindowInsetRight();
                    b = wi.getSystemWindowInsetBottom();
                }
                // In immersive fullscreen the ActionBar is pushed below the camera
                // cutout, so fold the cutout's TOP inset into the top offset (we
                // still draw under the cutout on the side/bottom edges).
                android.view.DisplayCutout dc = wi.getDisplayCutout();
                if (dc != null) t += dc.getSafeInsetTop();

                // The full-screen Qt surface is drawn behind the theme's ActionBar,
                // so the top offset must also clear the ActionBar, not just the
                // status-bar inset. Add the themed actionBarSize to the top value.
                android.util.TypedValue tv = new android.util.TypedValue();
                if (s_activity.getTheme().resolveAttribute(android.R.attr.actionBarSize, tv, true))
                    t += android.util.TypedValue.complexToDimensionPixelSize(
                            tv.data, s_activity.getResources().getDisplayMetrics());
                return (l << 48) | (t << 32) | (r << 16) | b;
            } catch (Throwable e) {
                return 0;
            }
        }

        public static int getModemStatus() {
            return m_device.getModemStatus();
        }

        public static int getSWCommandFrame() {
            return m_device.getSWCommandFrame();
        }

        public static int getHWCommandFrame(int mMethod) {
             return m_device.getHWCommandFrame(mMethod);
        }

        public static boolean isCommandAsserted(int mMethod) {
             return m_device.isCommandAsserted(mMethod);
        }

        public static void resetCommandLatch() {
             m_device.resetCommandLatch();
        }

        public static void armFrameResync() {
             m_device.armFrameResync();
        }
}
    class FileChooser implements Runnable
    {
        @Override
        public void run() {
            SerialActivity.s_activity.fileChooser();
        }
    }

    class DirChooser implements Runnable
    {
        @Override
        public void run() {
            SerialActivity.s_activity.dirChooser();
        }
    }

    class RegisterReceiverRunnable implements Runnable
    {
            // this method is called on Android Ui Thread
            @Override
            public void run() {
                    IntentFilter filter = new IntentFilter();
                    filter.addAction("com.android.example.USB_PERMISSION");
                    // this method must be called on Android Ui Thread
                    // API 33+ requires an explicit export flag for app-private receivers.
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                        SerialActivity.s_activity.registerReceiver(new USBReceiver(), filter, Context.RECEIVER_NOT_EXPORTED);
                    } else {
                        SerialActivity.s_activity.registerReceiver(new USBReceiver(), filter);
                    }
                    }
    }

    class USBReceiver extends BroadcastReceiver {
            @Override
            public void onReceive(Context context, Intent intent) {
            if (intent.getAction().equals("com.android.example.USB_PERMISSION"))
            synchronized (this) {
                UsbDevice device = (UsbDevice)intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
                if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
                    Log.v("USB", "Received permission result OK");
                    if(device != null){
                        Log.i("USB", "Device OK");
                    }
                    else {
                        Log.i("USB", "Permission denied for device " + device);
                        SerialActivity.s_activity.runOnUiThread(new Runnable() {
                            public void run() {
                                Toast.makeText(SerialActivity.s_activity, SerialActivity.s_activity.getResources().getString(R.string.sio2pc_no_permissions), Toast.LENGTH_LONG).show();
                            }
                        });
                    }
                }
                // Unblock openDevice() waiting on the permission result.
                if (SerialActivity.usbPermissionLatch != null)
                    SerialActivity.usbPermissionLatch.countDown();
            }
        }
    }
