package net.greblus;

import org.qtproject.example.AspeQt.R;
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
import android.os.Environment;
import android.content.pm.PackageManager;
import android.Manifest;
import android.net.Uri;
import android.provider.Settings;

public class SerialActivity extends QtActivity
{
        private static boolean debug = false;
        protected static byte rb[] = new byte [65535];
        protected static byte wb[] = new byte [65535];
        protected static byte t[] = new byte [1024];
        public static native void sendBufAddr(ByteBuffer rbuf, ByteBuffer wbuf);
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

        @Override
        protected void attachBaseContext(Context newBase) {
            // Wrap with the in-app language preference so Toasts/dialogs
            // (Java side) follow the user's choice from AspeQt Options
            // instead of always using the Android system locale.
            super.attachBaseContext(net.greblus.LocaleHelper.wrap(newBase));
        }

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
            requestRuntimePermissions();

            getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
            sendBufAddr(rbuf, wbuf);
        }

        private void requestRuntimePermissions() {
            java.util.ArrayList<String> needed = new java.util.ArrayList<String>();

            // Storage: legacy permissions on Android <= 10 (used by SimpleFileDialog
            // before scoped storage took over).
            if (Build.VERSION.SDK_INT >= 23 && Build.VERSION.SDK_INT <= 29) {
                if (checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE)
                        != PackageManager.PERMISSION_GRANTED) {
                    needed.add(Manifest.permission.WRITE_EXTERNAL_STORAGE);
                }
            }

            // Bluetooth runtime perms required on Android 12+ (API 31).
            if (Build.VERSION.SDK_INT >= 31) {
                if (checkSelfPermission("android.permission.BLUETOOTH_CONNECT")
                        != PackageManager.PERMISSION_GRANTED) {
                    needed.add("android.permission.BLUETOOTH_CONNECT");
                }
                if (checkSelfPermission("android.permission.BLUETOOTH_SCAN")
                        != PackageManager.PERMISSION_GRANTED) {
                    needed.add("android.permission.BLUETOOTH_SCAN");
                }
            }

            if (!needed.isEmpty()) {
                requestPermissions(needed.toArray(new String[0]), 1);
            }

            // Android 11+ (API 30): MANAGE_EXTERNAL_STORAGE — required so the
            // in-app file picker can browse anywhere on /sdcard (user folders,
            // Download, etc.). Android forbids granting this via the standard
            // runtime permission popup; the user must enable it in Settings.
            // Show a friendly explanation dialog with OK/Cancel before sending
            // them there.
            if (Build.VERSION.SDK_INT >= 30) {
                if (!Environment.isExternalStorageManager()) {
                    showAllFilesAccessDialog();
                }
            }
        }

        private void showAllFilesAccessDialog() {
            // Use getResources().getString(...) explicitly — matches the
            // existing working pattern (e.g. for bt_module_check toasts).
            // Qt for Android can wrap activity.getString() in a way that
            // doesn't fall through to Polish locale resources.
            android.content.res.Resources r = getResources();
            new android.app.AlertDialog.Builder(this)
                .setTitle(net.greblus.LocaleHelper.getString(this, R.string.perm_files_title))
                .setMessage(net.greblus.LocaleHelper.getString(this, R.string.perm_files_body))
                .setPositiveButton(net.greblus.LocaleHelper.getString(this, R.string.perm_files_ok), new android.content.DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(android.content.DialogInterface dialog, int which) {
                        try {
                            Intent i = new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION);
                            i.setData(Uri.parse("package:" + getPackageName()));
                            startActivity(i);
                        } catch (Exception e) {
                            try {
                                startActivity(new Intent(
                                    Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION));
                            } catch (Exception e2) {
                                Log.w("USB", "Cannot open all-files-access settings: " + e2);
                            }
                        }
                    }
                })
                .setNegativeButton(net.greblus.LocaleHelper.getString(this, R.string.perm_files_skip), new android.content.DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(android.content.DialogInterface dialog, int which) {
                        Toast.makeText(SerialActivity.this,
                            net.greblus.LocaleHelper.getString(SerialActivity.this, R.string.perm_files_skipped_toast),
                            Toast.LENGTH_LONG).show();
                    }
                })
                .setCancelable(false)
                .show();
        }

        @Override
        public void onPause() {
           m_chosen = "Cancelled";
           super.onPause();
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
                            if (m_chosen != "Cancelled") Toast.makeText(SerialActivity.this, net.greblus.LocaleHelper.getString(SerialActivity.this, R.string.file_chosen) +
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
                            if (m_chosen != "Cancelled") Toast.makeText(SerialActivity.this, net.greblus.LocaleHelper.getString(SerialActivity.this, R.string.dir_chosen) +
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

        public static int getModemStatus() {
            return m_device.getModemStatus();
        }

        public static int getSWCommandFrame() {
            return m_device.getSWCommandFrame();
        }

        public static int getHWCommandFrame(int mMethod) {
             return m_device.getHWCommandFrame(mMethod);
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
                    // From API 33 (Tiramisu) registering a non-protected receiver
                    // without an export flag throws SecurityException.
                    if (Build.VERSION.SDK_INT >= 33) {
                            SerialActivity.s_activity.registerReceiver(
                                new USBReceiver(), filter, Context.RECEIVER_NOT_EXPORTED);
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
                                Toast.makeText(SerialActivity.s_activity, net.greblus.LocaleHelper.getString(SerialActivity.s_activity, R.string.sio2pc_no_permissions), Toast.LENGTH_LONG).show();
                            }
                        });
                    }
                }
            }
        }
    }
