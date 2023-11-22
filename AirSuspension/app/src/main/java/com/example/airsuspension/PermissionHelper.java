package com.example.airsuspension;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Build;
import android.util.Log;
import android.widget.Toast;

import androidx.annotation.RequiresApi;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;


public class PermissionHelper {
    // Defining Permission codes.
    // We can give any value
    // but unique for each permission.
    //private static final int BLUETOOTH_PERMISSION_CODE = 100;
    //private static final int BLUETOOTH_ADMIN_PERMISSION_CODE = 101;
    //private static final int BLUETOOTH_CONNECT_PERMISSION_CODE = 102;
    //private static final int BLUETOOTH_SCAN_PERMISSION_CODE = 103;
    //private static final int ACCESS_FINE_LOCATION = 104;
    //private static final int ACCESS_COARSE_LOCATION = 105;
    //private static final int BLUETOOTH_ADVERTISE = 106;
    MainActivity mainActivity;
    private int curCode = -1;
    private final String[] perms = new String[] {
            Manifest.permission.ACCESS_COARSE_LOCATION,
            //Manifest.permission.ACCESS_FINE_LOCATION,
            Manifest.permission.BLUETOOTH,
            Manifest.permission.BLUETOOTH_ADMIN
            //Manifest.permission.BLUETOOTH_CONNECT
    };


    protected PermissionHelper(MainActivity mainActivity)
    {
        this.mainActivity = mainActivity;
        //checkPermission(Manifest.permission.ACCESS_FINE_LOCATION, ACCESS_FINE_LOCATION);
        //checkPermission(Manifest.permission.ACCESS_COARSE_LOCATION, ACCESS_COARSE_LOCATION);
        //checkPermission(Manifest.permission.BLUETOOTH_ADVERTISE, BLUETOOTH_ADVERTISE);
        //checkPermission(Manifest.permission.BLUETOOTH, BLUETOOTH_PERMISSION_CODE);
        //checkPermission(Manifest.permission.BLUETOOTH_ADMIN, BLUETOOTH_ADMIN_PERMISSION_CODE);
        //checkPermission(Manifest.permission.BLUETOOTH_CONNECT, BLUETOOTH_CONNECT_PERMISSION_CODE);
        //checkPermission(Manifest.permission.BLUETOOTH_SCAN, BLUETOOTH_SCAN_PERMISSION_CODE);
        //nextPermission();
    }

    public void retryLastPermission() {
        curCode--;
    }
    public void start() {
        nextPermission();

        //simply requesting all perms at once sadly does not work as expected
        //if (ContextCompat.checkSelfPermission(mainActivity, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
        //    ActivityCompat.requestPermissions(mainActivity, perms, 2027);
        //}
    }
    public String currentPermissionCheck() {
        if (curCode < 0) {
            //bruh stupid shit i swear
            return "lmao nothing";
        }
        return perms[curCode];
    }
    public void nextPermission() {
        if (curCode < perms.length-1) {//my code is stupid
            curCode++;
            Log.i("MYINFO","checking next permission ");
            checkPermission(perms[curCode], curCode+100);
        } else {
            Log.i("MYINFO","finished Permissions ");
        }
    }

    // Function to check and request permission.
    public void checkPermission(String permission, int requestCode)
    {
        if (ContextCompat.checkSelfPermission(mainActivity, permission) == PackageManager.PERMISSION_GRANTED) {
            //Toast.makeText(mainActivity, "Granted "+permission, Toast.LENGTH_SHORT).show();
            Log.i("MYINFO","Granted "+permission);
            nextPermission();
        } else {

            //Toast.makeText(mainActivity, "Requesting "+permission, Toast.LENGTH_SHORT).show();
            // Requesting the permission
            Log.i("MYINFO","Requesting "+permission);
            ActivityCompat.requestPermissions(mainActivity, new String[] { permission }, requestCode);
        }

    }



}
