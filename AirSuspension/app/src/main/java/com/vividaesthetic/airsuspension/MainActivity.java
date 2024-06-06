package com.vividaesthetic.airsuspension;

import android.annotation.SuppressLint;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.appcompat.app.AppCompatActivity;

import android.util.Log;

import androidx.navigation.NavController;
import androidx.navigation.NavDestination;
import androidx.navigation.Navigation;
import androidx.navigation.ui.AppBarConfiguration;
import androidx.navigation.ui.NavigationUI;

import com.vividaesthetic.airsuspension.databinding.ActivityMainBinding;
import com.vividaesthetic.airsuspension.utils.PermissionHelper;
import com.vividaesthetic.airsuspension.utils.PressureUnit;
import com.vividaesthetic.airsuspension.utils.SmplBTDevice;

import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import android.Manifest;

public class MainActivity extends AppCompatActivity {

    private final String PREFS_NAME = "vividaesthetic_airsuspension_preferences";
    private AppBarConfiguration appBarConfiguration;
    public ActivityMainBinding binding;
    public static AirSuspensionController airSuspensionController;

    private PermissionHelper permissionHelper;
    public PressureUnit.Unit preferredUnit = PressureUnit.Unit.PSI;


    private final List<String> errorLog = new ArrayList<>();
    private static final int MAX_ERROR_LINES = 70;
    private TextView logTextView;


    // References for bluetooth
    // https://stackoverflow.com/questions/67722950/android-12-new-bluetooth-permissions
    // https://stackoverflow.com/questions/74647915/issue-with-need-android-permission-bluetooth-scan-permission-for-attributionsour
    AirSuspensionController.BluetoothCommand btrunUponEnable = null;
    ActivityResultLauncher<Intent> requestBluetooth = registerForActivityResult(new ActivityResultContracts.StartActivityForResult(), result -> {
//        if (result.getResultCode() == Activity.RESULT_OK) {
//            if (btrunUponEnable != null) {
//                btrunUponEnable.command();
//                btrunUponEnable = null;
//            }
//        }
    });

    ActivityResultLauncher<String[]> requestMultiplePermissions = registerForActivityResult(new ActivityResultContracts.RequestMultiplePermissions(), permissions -> {
        boolean val = true;
        for (Map.Entry<String, Boolean> perm : permissions.entrySet()) {
            val = val && perm.getValue();
        }
        if (val) {
            if (btrunUponEnable != null) {
                btrunUponEnable.command();
                btrunUponEnable = null;
            }
        } else {
            enableBT(btrunUponEnable);
        }
    });

    // Bluetooth Permissions crap
    public void enableBT(AirSuspensionController.BluetoothCommand runUponEnable) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            btrunUponEnable = runUponEnable;
            // This is required for the newer devices!
            requestMultiplePermissions.launch(new String[]{
                    //First 2 are necessary bt permssions for bt, "Allow Air Suspension to find, connect to, and determine the relative position of nearby devices?" (tested on galaxy s23)
                    Manifest.permission.BLUETOOTH_SCAN,
                    Manifest.permission.BLUETOOTH_CONNECT,
                    //Next 4 are for the large "Allow Air Suspension to access this device's approximate location?" check (tested on galaxy s23)
                    Manifest.permission.ACCESS_COARSE_LOCATION,
                    Manifest.permission.ACCESS_FINE_LOCATION, // apparently my s23 wants this permission in order to search for available bt devices
                    Manifest.permission.BLUETOOTH,
                    Manifest.permission.BLUETOOTH_ADMIN});
        } else {
            //android 10, ect, other lower versions
            Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            requestBluetooth.launch(enableBtIntent);
            runUponEnable.command();// the 2 lines above don't actually seem to act correctly, so just calling it down here instead of on successfull callback
        }
    }

    interface OnReveivedBluetoothDevice {
        void receive(SmplBTDevice device);
    }

    public static OnReveivedBluetoothDevice onReveivedBluetoothDevice = null;

    // Bluetooth discovery
    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();

            if (BluetoothAdapter.ACTION_DISCOVERY_STARTED.equals(action)) {
                //discovery starts, we can show progress dialog or perform other tasks
                log("BT EVENT Started");
            } else if (BluetoothAdapter.ACTION_DISCOVERY_FINISHED.equals(action)) {
                //discovery finishes, dismiss progress dialog
                log("BT EVENT Finished");
            } else if (BluetoothDevice.ACTION_FOUND.equals(action)) {
                //bluetooth device found
                BluetoothDevice device = (BluetoothDevice) intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
                SmplBTDevice smplDevice = new SmplBTDevice(device.getAddress(), device.getName());
                log("BT EVENT " + smplDevice.toString());
                if (onReveivedBluetoothDevice != null) {
                    onReveivedBluetoothDevice.receive(smplDevice);
                }

                String deviceName = device.getName();
                String macAddress = device.getAddress();
                log("found device: " + deviceName + " at " + macAddress);

            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        IntentFilter filter = new IntentFilter();

        filter.addAction(BluetoothDevice.ACTION_FOUND);
        filter.addAction(BluetoothAdapter.ACTION_DISCOVERY_STARTED);
        filter.addAction(BluetoothAdapter.ACTION_DISCOVERY_FINISHED);

        registerReceiver(mReceiver, filter);


        enableBT(() -> {});

        preferredUnit = getSavedPressureUnit();
        AirSuspensionController.BT_DEVICE = getBT();

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());
        logTextView = findViewById(R.id.debug_log_overlay);

        permissionHelper = new PermissionHelper(this);
        permissionHelper.start();

        setSupportActionBar(binding.toolbar);

        NavController navController = Navigation.findNavController(this, R.id.nav_host_fragment_content_main);
        appBarConfiguration = new AppBarConfiguration.Builder(navController.getGraph()).build();
        NavigationUI.setupActionBarWithNavController(this, navController, appBarConfiguration);
    }



    @Override
    protected void onDestroy() {
        super.onDestroy();
        unregisterReceiver(mReceiver);
    }

    public static AirSuspensionController getAirSuspensionController(MainActivity activity) {
        if (airSuspensionController == null || (airSuspensionController.activity == null && activity != null)) {
            if (airSuspensionController != null) {
                airSuspensionController.close();
            }
            airSuspensionController = new AirSuspensionController(activity);
        }
        return airSuspensionController;
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_bluetooth) {
            NavController navController = Navigation.findNavController(this, R.id.nav_host_fragment_content_main);
            NavDestination destination = navController.getCurrentDestination();
            if (destination.getId() != R.id.BluetoothFragment) {
                navController.navigate(R.id.action_to_BluetoothFragment);
            }
        }

        return super.onOptionsItemSelected(item);
    }

    @Override
    public boolean onSupportNavigateUp() {
        NavController navController = Navigation.findNavController(this, R.id.nav_host_fragment_content_main);
        return NavigationUI.navigateUp(navController, appBarConfiguration)
                || super.onSupportNavigateUp();
    }

    @SuppressLint("MissingSuperCall")
    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent Data) {
        getAirSuspensionController(this).onActivityResult(requestCode, resultCode, Data);
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions,
                                           int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (permissions.length >= 1) {
            if (permissions[0].equalsIgnoreCase(permissionHelper.currentPermissionCheck())) {
                if (grantResults[0] == PackageManager.PERMISSION_DENIED) {
                    permissionHelper.retryLastPermission();
                    log("Denied gonna retry " + permissions[0]);
                    Toast.makeText(this, "Denied " + permissions[0], Toast.LENGTH_SHORT).show();
                }
                permissionHelper.nextPermission();
            }
        }
    }

    private void updateLog() {
        String log = "";
        for (String str : errorLog) {
            log = str + "\n" + log;
        }
        if (logTextView != null) {
            logTextView.setText(log);
        }
    }

    public boolean isLogVisible() {
        return logTextView.getVisibility() == View.VISIBLE;
    }
    public void setLogVisible(boolean enabled) {
        if (logTextView != null) {
            logTextView.setVisibility(enabled ? View.VISIBLE : View.GONE);
        }
    }

    public void log(String text) {
        if (text.length() > 0) {
            Log.i("OASMan",text);
            errorLog.add(text) ;
        }
        // remove the first line if log is too large
        if (errorLog.size() >= MAX_ERROR_LINES) {
            errorLog.remove(0);
        }
        updateLog();
    }

    private Toast previousToast;

    public void toast(String text) {
        try {
            log("Toast said: " + text);
            if (previousToast != null) {
                previousToast.cancel();
            }
            previousToast = Toast.makeText(getApplicationContext(), text, Toast.LENGTH_SHORT);
            previousToast.show();
        } catch (Exception e) {}
    }

    public boolean getPreferenceBool(String name) {
        SharedPreferences settings = getSharedPreferences(PREFS_NAME, 0);
        return settings.getBoolean(name, false);
    }

    public void setPreferenceBool(String name, boolean value) {
        SharedPreferences settings = getSharedPreferences(PREFS_NAME, 0);
        SharedPreferences.Editor editor = settings.edit();
        editor.putBoolean(name, value);
        editor.apply();
    }

    private int getPreferenceInt(String name, int defValue) {
        SharedPreferences settings = getSharedPreferences(PREFS_NAME, 0);
        return settings.getInt(name, defValue);
    }
    private int getPreferenceInt(String name) {return getPreferenceInt(name, -1);}

    private void setPreferenceInt(String name, int value) {
        SharedPreferences settings = getSharedPreferences(PREFS_NAME, 0);
        SharedPreferences.Editor editor = settings.edit();
        editor.putInt(name, value);
        editor.apply();
    }

    private String getPreferenceString(String name, String defValue) {
        SharedPreferences settings = getSharedPreferences(PREFS_NAME, 0);
        return settings.getString(name, defValue);
    }
    private String getPreferenceString(String name) {return getPreferenceString(name, "");}

    private void setPreferenceString(String name, String value) {
        SharedPreferences settings = getSharedPreferences(PREFS_NAME, 0);
        SharedPreferences.Editor editor = settings.edit();
        editor.putString(name, value);
        editor.apply();
    }

    public void savePressureUnit(PressureUnit.Unit unit) {
        this.preferredUnit = unit;
        setPreferenceInt("unit", unit.getId());
    }

    public PressureUnit.Unit getSavedPressureUnit() {
        try {
            return PressureUnit.Unit.fromId(getPreferenceInt("unit"));
        } catch (Exception e) {
            return PressureUnit.Unit.PSI;
        }
    }

    public void saveBT(SmplBTDevice device) {
        AirSuspensionController.BT_DEVICE = device;
        setPreferenceString("btmac", device.getMac());
        setPreferenceString("btname", device.getName());
    }

    public SmplBTDevice getBT() {
        return new SmplBTDevice(getPreferenceString("btmac"),getPreferenceString("btname"));//""00:14:03:05:59:F6"
    }

    public void saveBTPassword(String password) {
        setPreferenceString("btpassword", password);
    }

    public String getBTPassword() {
        return getPreferenceString("btpassword", "12345678"); // PASSWORD on arduino
    }
}