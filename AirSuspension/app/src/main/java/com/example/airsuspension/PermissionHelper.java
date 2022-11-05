package com.example.airsuspension;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Build;
import android.widget.Toast;

import androidx.annotation.RequiresApi;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

public class PermissionHelper {
    // Defining Permission codes.
    // We can give any value
    // but unique for each permission.
    private static final int BLUETOOTH_PERMISSION_CODE = 100;
    private static final int BLUETOOTH_ADMIN_PERMISSION_CODE = 101;
    private static final int BLUETOOTH_CONNECT_PERMISSION_CODE = 102;
    private static final int BLUETOOTH_SCAN_PERMISSION_CODE = 103;
    MainActivity mainActivity;

    @RequiresApi(api = Build.VERSION_CODES.S)
    protected PermissionHelper(MainActivity mainActivity)
    {
        this.mainActivity = mainActivity;
        checkPermission(Manifest.permission.BLUETOOTH, BLUETOOTH_PERMISSION_CODE);
        checkPermission(Manifest.permission.BLUETOOTH_ADMIN, BLUETOOTH_ADMIN_PERMISSION_CODE);
        checkPermission(Manifest.permission.BLUETOOTH_CONNECT, BLUETOOTH_CONNECT_PERMISSION_CODE);
        checkPermission(Manifest.permission.BLUETOOTH_SCAN, BLUETOOTH_SCAN_PERMISSION_CODE);
    }

    // Function to check and request permission.
    public void checkPermission(String permission, int requestCode)
    {
        if (ContextCompat.checkSelfPermission(mainActivity, permission) == PackageManager.PERMISSION_DENIED) {

            // Requesting the permission
            ActivityCompat.requestPermissions(mainActivity, new String[] { permission }, requestCode);
        }
        else {
            //Toast.makeText(mainActivity, "Permission already granted", Toast.LENGTH_SHORT).show();
        }
    }

}
