package com.example.airsuspension.utils;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Build;
import android.util.Log;
import android.widget.Toast;

import androidx.annotation.RequiresApi;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import com.example.airsuspension.MainActivity;


public class PermissionHelper {
    MainActivity mainActivity;
    private int curCode = -1;
    private final String[] perms = new String[] {
            Manifest.permission.ACCESS_COARSE_LOCATION,
            Manifest.permission.BLUETOOTH,
            Manifest.permission.BLUETOOTH_ADMIN
    };


    public PermissionHelper(MainActivity mainActivity)
    {
        this.mainActivity = mainActivity;
    }

    public void retryLastPermission() {
        curCode--;
    }
    public void start() {
        nextPermission();
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
            Log.i("MYINFO","Granted "+permission);
            nextPermission();
        } else {

            // Requesting the permission
            Log.i("MYINFO","Requesting "+permission);
            ActivityCompat.requestPermissions(mainActivity, new String[] { permission }, requestCode);
        }

    }



}
