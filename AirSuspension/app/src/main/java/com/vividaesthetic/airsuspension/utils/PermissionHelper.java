package com.vividaesthetic.airsuspension.utils;

import android.Manifest;
import android.content.pm.PackageManager;

import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import com.vividaesthetic.airsuspension.MainActivity;


/**
 * Permission setup for version before api 31, which includes android 10 devices
 */
public class PermissionHelper {
    MainActivity mainActivity;
    private int curCode = -1;
    private final String[] perms = new String[] {
            //Next 4 are for the large "Allow Air Suspension to access this device's approximate location?" check (galaxy s23)
            Manifest.permission.ACCESS_COARSE_LOCATION,
            Manifest.permission.ACCESS_FINE_LOCATION,
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

        mainActivity.logWithToastQuick( "inside permissions helper start!");

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
            mainActivity.logWithToastQuick("checking next permission ");
            checkPermission(perms[curCode], curCode+100);
        } else {
            mainActivity.logWithToastQuick("finished Permissions ");
        }
    }

    // Function to check and request permission.
    public void checkPermission(String permission, int requestCode)
    {
        if (ContextCompat.checkSelfPermission(mainActivity, permission) == PackageManager.PERMISSION_GRANTED) {
            mainActivity.logWithToastQuick("Granted "+permission);
            nextPermission();
        } else {

            // Requesting the permission
            mainActivity.logWithToastQuick("Requesting "+permission);
            ActivityCompat.requestPermissions(mainActivity, new String[] { permission }, requestCode);
        }

    }



}
