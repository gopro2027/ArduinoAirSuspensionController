package com.example.airsuspension;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.appwidget.AppWidgetManager;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.widget.RemoteViews;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.lifecycle.MutableLiveData;

import java.io.IOException;
import java.lang.reflect.Method;
import java.nio.charset.StandardCharsets;
import java.util.UUID;

//TODO: use this https://github.com/bauerjj/Android-Simple-Bluetooth-Example

public class AirSuspensionController {

    public String TAG = "AirSuspensionController";
    private static final UUID BT_MODULE_UUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB"); // "random" unique identifier
    private static final String BT_MAC = "00:14:03:05:59:F6";
    //private static UUID BT_MODULE_UUID;
    //private static final String BT_MAC = "00:14:03:05:59:EA";

    // #defines for identifying shared types between calling functions
    private final static int REQUEST_ENABLE_BT = 1; // used to identify adding bluetooth names
    public final static int MESSAGE_READ = 2; // used in bluetooth handler to identify message update
    private final static int CONNECTING_STATUS = 3; // used in bluetooth handler to identify message status

    @NonNull
    private BluetoothAdapter mBTAdapter;
    //private Set<BluetoothDevice> mPairedDevices;
    private Handler mHandler; // Our main handler that will receive callback notifications
    private ConnectedThread mConnectedThread; // bluetooth background worker thread to send and receive data
    private BluetoothSocket mBTSocket = null; // bi-directional client-to-client data path

    public long heartbeat = 0;

    public TextView mReadBuffer;
    public TextView mLogBuffer;

    Activity activity;

    void close() {
        try {
            mConnectedThread.cancel();
        } catch (Exception e) {
            //swallow bc it errors a lot
            //e.printStackTrace();
        }

    }

    interface UpdatePressure {
        void updatePressure(String fp, String rp, String fd, String rd, String tank);
    }

    interface BluetoothCommand {
        void command();
    }

    private UpdatePressure updatePressure = null;
    private UpdatePressure updatePressureProfile = null;
    private BluetoothCommand onConnectedCmd = null;

    public void setUpdatePressure(UpdatePressure updatePressure) {
        this.updatePressure = updatePressure;
    }

    public void setUpdatePressureProfile(UpdatePressure updatePressure) {
        this.updatePressureProfile = updatePressure;
    }

    AirSuspensionController(Activity activity) {

        // Ask for location permission if not already allowed
        if (activity != null) {
            if (ContextCompat.checkSelfPermission(activity, Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED)
                ActivityCompat.requestPermissions(activity, new String[]{Manifest.permission.ACCESS_COARSE_LOCATION}, 1);

        }

        //BT_MODULE_UUID = UUID.randomUUID();

        mBTAdapter = BluetoothAdapter.getDefaultAdapter();
        if (mBTAdapter == null) {
            appendToLog("Bluetooth unavailable on this device (null)");
        } else {

        }
        this.activity = activity;

        mHandler = new Handler(Looper.getMainLooper()) {
            @Override
            public void handleMessage(Message msg) {
                if (msg.what == MESSAGE_READ) {
                    String readMessage = new String((byte[]) msg.obj, StandardCharsets.UTF_8).substring(0, msg.arg1 - 1);//-1 bc all messages are terminated by a \n

                    String messages[] = readMessage.split("\n");

                    for (String message : messages) {

                        String recv_password = "56347893";

                        if (message.startsWith(recv_password)) {

                            message = message.substring(recv_password.length());

                            if (mReadBuffer != null) {
                                mReadBuffer.setText(message);
                            }

                            Log.i("Received", message);
                            heartbeat = System.currentTimeMillis();


                            //if (arr.length == 1) {


                            if (message.startsWith("NOTIF")) {
                                message = message.substring("NOTIF".length());
                                toast(message);
                            } else if (message.startsWith("PROF")) {
                                message = message.substring("PROF".length());
                                String[] arr = message.split("\\|");
                                if (arr.length == 4) {
                                    String fp = arr[0];
                                    String rp = arr[1];
                                    String fd = arr[2];
                                    String rd = arr[3];

                                    if (updatePressureProfile != null)
                                        updatePressureProfile.updatePressure(fp, rp, fd, rd, null);// no tank pressure
                                }

                            } else if (message.startsWith("PRES")) {
                                message = message.substring("PRES".length());

                                String[] arr = message.split("\\|");

                                //}
                                if (arr.length == 5) {

                                    if (onConnectedCmd != null) {
                                        onConnectedCmd.command();
                                        onConnectedCmd = null;
                                    }

                                    String fp = arr[0];
                                    String rp = arr[1];
                                    String fd = arr[2];
                                    String rd = arr[3];
                                    String tank = arr[4];

                                    try {

                                        AppWidgetManager appWidgetManager = AppWidgetManager.getInstance(activity);
                                        RemoteViews remoteViews = new RemoteViews(((Context) activity).getPackageName(), R.layout.pressure_widget);
                                        ComponentName thisWidget = new ComponentName(activity, PressureWidget.class);
                                        remoteViews.setTextViewText(R.id.pressure_fp, fp);
                                        remoteViews.setTextViewText(R.id.pressure_rp, rp);
                                        remoteViews.setTextViewText(R.id.pressure_fd, fd);
                                        remoteViews.setTextViewText(R.id.pressure_rd, rd);
                                        remoteViews.setTextViewText(R.id.pressure_tank, tank);

                                        if (updatePressure != null)
                                            updatePressure.updatePressure(fp, rp, fd, rd, tank);

                                        appWidgetManager.updateAppWidget(thisWidget, remoteViews);


                                    } catch (Exception e) {
                                        //e.printStackTrace();
                                    }
                                }
                            }
                        }
                    }
                }

                if (msg.what == CONNECTING_STATUS) {
                    char[] sConnected;
                    if (msg.arg1 == 1)
                        //mBluetoothStatus.setText(getString(R.string.BTConnected) + msg.obj);
                        toast("Connection to: " + msg.obj);
                    else {
                        // Make it post to the main thread so that the previous thread (the one that will be setting btConnectTryingRunning to false) will complete
                        /*new Handler(Looper.getMainLooper()).postDelayed(()-> {



                            if (btConnectTryingRunning) {
                                toast("Can't retry, thread is already active :(");
                            } else {
                                int timeoutMS = (5 * 60 * 1000);// 5 minutes
                                if (lastBluetoothRequestTime + timeoutMS > System.currentTimeMillis()) {
                                    toast("retrying connection...");

                                    //restart bluetooth connect after 100ms
                                    new Handler(Looper.getMainLooper()).postDelayed(
                                            () -> btStartThread(BT_MAC)
                                            , 250);

                                } else {
                                    toast("Connection timeout!");
                                }
                            }
                        }, 250);*/


                    }
                }
            }
        };
    }

    public void airUp() {
        bluetoothOn(() -> {
            if (mConnectedThread != null) { //First check to make sure thread created
                mConnectedThread.write("AIRUP\n");
                toast("Sent air up");
            }
        });
    }

    public void airOut() {
        bluetoothOn(() -> {
            if (mConnectedThread != null) { //First check to make sure thread created
                mConnectedThread.write("AIROUT\n");
                toast("Sent air out");
            }
        });
    }

    public void airSm(int psi) {
        bluetoothOn(() -> {
            if (mConnectedThread != null) { //First check to make sure thread created
                mConnectedThread.write("AIRSM" + psi + "\n");
                toast("Adding this much pressure: " + psi);
            }
        });
    }

    /*
    disabled now, 1 is hardcoded default profile
    public void setBaseProfile(int profileNum) {
        bluetoothOn(() -> {
            if (mConnectedThread != null) { //First check to make sure thread created
                mConnectedThread.write("PRBOF" + profileNum + "\n");
                toast("Set base profile (loaded on car start) to " + (profileNum+1));
            }
        });
    }*/

    public void saveToProfile(int profileNum) {
        bluetoothOn(() -> {
            if (mConnectedThread != null) { //First check to make sure thread created
                mConnectedThread.write("SPROF" + profileNum + "\n");
                toast("Saved to profile " + (profileNum+1));
            }
        });
    }

    public void readProfile(int profileNum) {
        bluetoothOn(() -> {
            if (mConnectedThread != null) { //First check to make sure thread created
                mConnectedThread.write("PROFR" + profileNum + "\n");
                toast("Loaded profile " + (profileNum+1));
            }
        });
    }

    public void quickAirUp(int profileNum) {
        bluetoothOn(() -> {
            if (mConnectedThread != null) { //First check to make sure thread created
                mConnectedThread.write("AUQ" + profileNum + "\n");
                toast("Airing up on profile " + (profileNum+1));
            }
        });
    }

    public void setFrontPressureD(int pressure) {
        bluetoothOn(() -> {
            if (mConnectedThread != null) { //First check to make sure thread created
                mConnectedThread.write("AIRHEIGHTC" + pressure + "\n");
                toast("Set front driver pressure");
            }
        });
    }

    public void setFrontPressureP(int pressure) {
        bluetoothOn(() -> {
            if (mConnectedThread != null) { //First check to make sure thread created
                mConnectedThread.write("AIRHEIGHTA" + pressure + "\n");
                toast("Set front passenger pressure");
            }
        });
    }

    public void setRearPressureD(int pressure) {
        bluetoothOn(() -> {
            if (mConnectedThread != null) { //First check to make sure thread created
                mConnectedThread.write("AIRHEIGHTD" + pressure + "\n");
                toast("Set rear driver pressure");
            }
        });
    }

    public void setRearPressureP(int pressure) {
        bluetoothOn(() -> {
            if (mConnectedThread != null) { //First check to make sure thread created
                mConnectedThread.write("AIRHEIGHTB" + pressure + "\n");
                toast("Set rear passenger pressure");
            }
        });
    }

    public void setRiseOnStart(boolean riseOnStart) {
        bluetoothOn(() -> {
            if (mConnectedThread != null) { //First check to make sure thread created
                mConnectedThread.write("RISEONSTART" + (riseOnStart ? "1" : "0") + "\n");
                toast("Set rise on start " + riseOnStart);
            }
        });
    }

    public void setRaiseOnPressureSet(boolean raiseOnPressureSet) {
        bluetoothOn(() -> {
            if (mConnectedThread != null) { //First check to make sure thread created
                mConnectedThread.write("ROPS" + (raiseOnPressureSet ? "1" : "0") + "\n");
                toast("Set raise on pressure set " + raiseOnPressureSet);
            }
        });
    }

    public void testSolenoid(int pinNum) {
        bluetoothOn(() -> {
            if (mConnectedThread != null) { //First check to make sure thread created
                if (pinNum >= 6 && pinNum <= 13) {
                    mConnectedThread.write("TESTSOL" + pinNum + "\n");
                    toast("Tested solenoid on pin " + pinNum);
                } else {
                    toast("Please use a pin between 6 and 13");
                }
            }
        });
    }

    //boolean forceReconnect = false;
    public void forceReconnect() {
        if (!btConnectTryingRunning) {
            toast("Loop detected timeout! Reconnecting.");
            //forceReconnect = true;
            bluetoothOn();
        }
    }
    public void forceReconnectLoop() {
        if (System.currentTimeMillis() - heartbeat > 10000) {
            forceReconnect();
        }
    }

    @SuppressLint("MissingPermission")
    public void bluetoothOn(BluetoothCommand cmd) {
        onConnectedCmd = cmd; // cue the command to run right after the next time we receive a heartbeat
        //if (System.currentTimeMillis() - heartbeat < 10000) {//30 seconds timeout eek
        //    //toast("Socket is already connected!");
        //    appendToLog("socket already connected");
        //    return;
        //}

        //bluetoothOn();
    }

    public void bluetoothOn() {
        if (mBTAdapter != null) {
            if (!mBTAdapter.isEnabled() && activity.getIntent() != null) {
                appendToLog("bt adapter not enabled... sending intent");
                if (activity != null) {
                    Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
                    activity.startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT);
                } else {
                    //no activity
                    return;//this would be the widget if it's null idk
                }
                //toast("Bluetooth turned on");
            } else {
                //toast("Bluetooth is already on");
                //appendToLog("bt adapter enabled");
            }
        } else {
            //appendToLog("bt adapter null");
        }

        //discover();
        lastBluetoothRequestTime = System.currentTimeMillis();
        btStartThread(BT_MAC);
    }

    private Toast previousToast;
    private StringBuffer toastLog = new StringBuffer();

    public void appendToLog(String text) {
        toastLog.append(text+"\n");
        if (mLogBuffer != null) {
            mLogBuffer.setText(toastLog.toString());
        }
        if (toastLog.toString().length() > 1000) {
            toastLog = new StringBuffer();//gross it's sooo long
        }
    }

    public void toast(String text) {
        appendToLog(text);
        try {
            Log.i(TAG, "Toast said: " + text);
            if (previousToast != null) {
                previousToast.cancel();
            }
            if (activity != null) {
                previousToast = Toast.makeText(activity.getApplicationContext(), text, Toast.LENGTH_SHORT);
                previousToast.show();
            }
        } catch (Exception e) {}
    }


    // Enter here after user selects "yes" or "no" to enabling radio

    protected void onActivityResult(int requestCode, int resultCode, Intent Data) {
        // Check which request we're responding to
        if (requestCode == REQUEST_ENABLE_BT) {
            // Make sure the request was successful
            if (resultCode == Activity.RESULT_OK) {
                // The user picked a contact.
                // The Intent's data Uri identifies which contact was selected.
                //mBTAdapter.enable();
                toast("status: Enabled");
            } else
                toast("status: Disabled");
        }
    }

    @SuppressLint("MissingPermission")
    private BluetoothSocket createBluetoothSocket(BluetoothDevice device) throws IOException {
        try {
            //final Method m = device.getClass().getMethod("createInsecureRfcommSocketToServiceRecord", UUID.class);
            //return (BluetoothSocket) m.invoke(device, BT_MODULE_UUID);
            return device.createInsecureRfcommSocketToServiceRecord(BT_MODULE_UUID);
        } catch (Exception e) {
            e.printStackTrace();
            Log.e(TAG, "Could not create Insecure RFComm Connection", e);
        }
        return device.createRfcommSocketToServiceRecord(BT_MODULE_UUID);
    }

    /*@SuppressLint("MissingPermission")
    private BluetoothSocket createBluetoothSocket2(BluetoothDevice device) throws IOException {
        try {
            Method createMethod = device.getClass().getMethod("createInsecureRfcommSocket", new Class[] { int.class });
            return (BluetoothSocket)createMethod.invoke(device, 1);
        } catch (Exception e) {
            e.printStackTrace();
            toast("Could not create Insecure RFComm Connection 2");
        }
        return device.createRfcommSocketToServiceRecord(BT_MODULE_UUID);
    }*/

    public boolean btConnectTryingRunning = false;
    private long lastBluetoothRequestTime = 0;

    public void btStartThread(String address) {
        //final boolean stop[] = {false, false};
        //toast("Connection Attempting...");
        if (mBTAdapter == null || !mBTAdapter.isEnabled()) {
            //toast("BT not on!");
            return;
        }

        if (!btConnectTryingRunning) {
            // Spawn a new thread to avoid blocking the GUI one
            Thread t = new Thread() {
                @SuppressLint("MissingPermission")
                @Override
                public void run() {
                    btConnectTryingRunning = true;
                    boolean fail = false;

                    //if it's open already... close it
                    try {
                        mBTSocket.close();
                    } catch (Exception e) {
                        e.printStackTrace();
                    }

                    //mBTAdapter.cancelDiscovery();//permissions error

                    BluetoothDevice device = mBTAdapter.getRemoteDevice(address);
                    appendToLog("device created");
                    /*try {
                        mBTSocket = createBluetoothSocket(device);
                        appendToLog("socket created");
                    } catch (Exception e) {
                        fail = true;
                        toast("Error creating socket");
                    }
                    // Establish the Bluetooth socket connection.
                    try {
                        mBTSocket.connect();
                        appendToLog("socket connected");
                    } catch (Exception e) {*/

                        try {
                            //try version 2
                            //try {
                                mBTSocket = createBluetoothSocket(device);
                                appendToLog("socket connected ");
                            //} catch (Exception e2) {
                            //    fail = true;
                            //    toast("Error creating socket");
                            //}
                            mBTSocket.connect();


                        } catch (Exception e2) {

                            //cut short if socket failed but we are waiting
                            //if (stop[0] == true) {
                            //    return;
                            //}
                            fail = true;
                            try {
                                mBTSocket.close();
                                mHandler.obtainMessage(CONNECTING_STATUS, -1, -1)
                                        .sendToTarget();
                                appendToLog("ah shoot we failed both connections");
                            } catch (Exception e3) {
                                //insert code to deal with this
                                toast("Error connecting socket");
                            }
                        }
                    //}
                    if (!fail) {
                        //appendToLog("I think I connected!");
                        mConnectedThread = new ConnectedThread(mBTSocket, mHandler);
                        mConnectedThread.start();
                        mHandler.obtainMessage(CONNECTING_STATUS, 1, -1, "Specified MAC")
                                .sendToTarget();
                        heartbeat = System.currentTimeMillis() - 4000;//make it 6 seconds wait time I guess lmao idk :( I wish it didn't think it was connected when it wasn't because then I could just set this to the current time and it would be great 100% of the time
                    } else {
                        appendToLog("Definitely failed connection");
                    }
                    //stop[1] = true;
                    btConnectTryingRunning = false;
                }
            };
            t.start();

            // Connection timeout (making it quicker)
            // JK it seems as though the socket just glitches out if we try to end it early, and causes a failure in the code above when running mBTSocket.connect();
            /*new Handler(Looper.getMainLooper()).postDelayed(
                    () -> {
                        if (btConnectTryingRunning && stop[1] == false) {
                            if (mBTSocket != null) {
                                toast("Cancelled early");
                                try {
                                    mBTSocket.close();
                                } catch (IOException e) {
                                    throw new RuntimeException(e);
                                }
                                stop[0] = true;
                                toast("stop[0] = true;");
                                btConnectTryingRunning = false;
                                mHandler.obtainMessage(CONNECTING_STATUS, -1, -1).sendToTarget();
                            }
                        }
                    }
                    , 2500);*/

        } else {
            toast("Connection thread already running...");
        }
    }
}
