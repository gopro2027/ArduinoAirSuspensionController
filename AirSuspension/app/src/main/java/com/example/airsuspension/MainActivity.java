package com.example.airsuspension;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.os.Bundle;

import androidx.appcompat.app.AppCompatActivity;

import android.util.Log;

import androidx.navigation.NavController;
import androidx.navigation.NavDestination;
import androidx.navigation.Navigation;
import androidx.navigation.ui.AppBarConfiguration;
import androidx.navigation.ui.NavigationUI;

import com.example.airsuspension.databinding.ActivityMainBinding;
import com.example.airsuspension.utils.PermissionHelper;
import com.example.airsuspension.utils.PressureUnit;

import android.view.Menu;
import android.view.MenuItem;
import android.widget.Toast;

import java.lang.reflect.ParameterizedType;

public class MainActivity extends AppCompatActivity {

    private final String PREFS_NAME = "vividaesthetic_airsuspension_preferences";
    private AppBarConfiguration appBarConfiguration;
    public ActivityMainBinding binding;
    public static AirSuspensionController airSuspensionController;

    private PermissionHelper permissionHelper;
    public PressureUnit.Unit preferredUnit = PressureUnit.Unit.PSI;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        preferredUnit = getSavedPressureUnit();

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        permissionHelper = new PermissionHelper(this);
        permissionHelper.start();

        setSupportActionBar(binding.toolbar);

        NavController navController = Navigation.findNavController(this, R.id.nav_host_fragment_content_main);
        appBarConfiguration = new AppBarConfiguration.Builder(navController.getGraph()).build();
        NavigationUI.setupActionBarWithNavController(this, navController, appBarConfiguration);
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
    protected void onActivityResult(int requestCode, int resultCode, Intent Data){
        getAirSuspensionController(this).onActivityResult(requestCode,resultCode,Data);
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions,
                                           int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (permissions.length >= 1) {
            if (permissions[0].equalsIgnoreCase(permissionHelper.currentPermissionCheck())) {
                if (grantResults[0] == PackageManager.PERMISSION_DENIED) {
                    permissionHelper.retryLastPermission();
                    Log.i("MYINFO", "Denied gonna retry " + permissions[0]);
                    Toast.makeText(this, "Denied " + permissions[0], Toast.LENGTH_SHORT).show();
                }
                permissionHelper.nextPermission();
            }
        }
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

    private int getPreferenceInt(String name) {
        SharedPreferences settings = getSharedPreferences(PREFS_NAME, 0);
        return settings.getInt(name, -1);
    }
    private void setPreferenceInt(String name, int value) {
        SharedPreferences settings = getSharedPreferences(PREFS_NAME, 0);
        SharedPreferences.Editor editor = settings.edit();
        editor.putInt(name, value);
        editor.apply();
    }

    public void savePressureUnit(PressureUnit.Unit unit) {
        this.preferredUnit = unit;
        setPreferenceInt("unit",unit.getId());
    }
    public PressureUnit.Unit getSavedPressureUnit() {
        try {
            return PressureUnit.Unit.fromId(getPreferenceInt("unit"));
        } catch (Exception e) {
            return PressureUnit.Unit.PSI;
        }
    }
}