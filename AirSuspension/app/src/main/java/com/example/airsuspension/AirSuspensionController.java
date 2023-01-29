package com.example.airsuspension;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.appwidget.AppWidgetManager;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.widget.ArrayAdapter;
import android.widget.RemoteViews;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import java.io.IOException;
import java.lang.reflect.Method;
import java.nio.charset.StandardCharsets;
import java.util.Set;
import java.util.UUID;

//TODO: use this https://github.com/bauerjj/Android-Simple-Bluetooth-Example

public class AirSuspensionController {

    public String TAG = "AirSuspensionController";

    private static final UUID BT_MODULE_UUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB"); // "random" unique identifier
    private static final String BT_MAC = "00:14:03:05:59:F6";
    //private static final String BT_MAC = "00:14:03:05:59:EA";

    // #defines for identifying shared types between calling functions
    private final static int REQUEST_ENABLE_BT = 1; // used to identify adding bluetooth names
    public final static int MESSAGE_READ = 2; // used in bluetooth handler to identify message update
    private final static int CONNECTING_STATUS = 3; // used in bluetooth handler to identify message status

    @NonNull
    private BluetoothAdapter mBTAdapter;
    private Set<BluetoothDevice> mPairedDevices;
    private Handler mHandler; // Our main handler that will receive callback notifications
    private ConnectedThread mConnectedThread; // bluetooth background worker thread to send and receive data
    private BluetoothSocket mBTSocket = null; // bi-directional client-to-client data path

    private long heartbeat = 0;

    public TextView mReadBuffer;

    Activity activity;

    void close() {
        try {
            mConnectedThread.cancel();
        } catch (Exception e) {
            e.printStackTrace();
        }

    }

    interface UpdatePressure {
        void updatePressure(String fp, String rp, String fd, String rd, String tank);
    }

    interface BluetoothCommand {
        void command();
    }

    private UpdatePressure updatePressure = null;
    private BluetoothCommand onConnectedCmd = null;

    public void setUpdatePressure(UpdatePressure updatePressure) {
        this.updatePressure = updatePressure;
    }

    AirSuspensionController(Activity activity) {

        // Ask for location permission if not already allowed
        if (activity != null) {
            if (ContextCompat.checkSelfPermission(activity, Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED)
                ActivityCompat.requestPermissions(activity, new String[]{Manifest.permission.ACCESS_COARSE_LOCATION}, 1);

        }

        mBTAdapter = BluetoothAdapter.getDefaultAdapter();
        if (mBTAdapter == null) {
            Log.e(TAG, "Bluetooth unavailable on this device");
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

                            String[] arr = message.split("\\|");
                            if (arr.length == 1) {


                                String cmd_key = "NOTIF";
                                if (message.startsWith(cmd_key)) {
                                    message = message.substring(cmd_key.length());
                                    toast(message);
                                }


                            }
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

                if (msg.what == CONNECTING_STATUS) {
                    char[] sConnected;
                    if (msg.arg1 == 1)
                        //mBluetoothStatus.setText(getString(R.string.BTConnected) + msg.obj);
                        toast("Connection to: " + msg.obj);
                    else
                        //mBluetoothStatus.setText(getString(R.string.BTconnFail));
                        toast("Connection fail");
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

    public void setBaseProfile(int profileNum) {
        bluetoothOn(() -> {
            if (mConnectedThread != null) { //First check to make sure thread created
                mConnectedThread.write("PRBOF" + profileNum + "\n");
                toast("Set base profile (loaded on car start) to "+profileNum);
            }
        });
    }

    public void saveToProfile(int profileNum) {
        bluetoothOn(() -> {
            if (mConnectedThread != null) { //First check to make sure thread created
                mConnectedThread.write("SPROF" + profileNum + "\n");
                toast("Saved to profile "+profileNum);
            }
        });
    }

    public void readProfile(int profileNum) {
        bluetoothOn(() -> {
            if (mConnectedThread != null) { //First check to make sure thread created
                mConnectedThread.write("PROFR" + profileNum + "\n");
                toast("Loaded profile "+profileNum);
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
                toast("Set rise on start");
            }
        });
    }

    public void setRaiseOnPressureSet(boolean riseOnStart) {
        bluetoothOn(() -> {
            if (mConnectedThread != null) { //First check to make sure thread created
                mConnectedThread.write("ROPS" + (riseOnStart ? "1" : "0") + "\n");
                toast("Set rise on start");
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

    @SuppressLint("MissingPermission")
    public void bluetoothOn(BluetoothCommand cmd) {
        onConnectedCmd = cmd;
        if (System.currentTimeMillis() - heartbeat < 5000) {
            //toast("Socket is already connected!");
            return;
        }

        if (!mBTAdapter.isEnabled()) {
            Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            if (activity != null) {
                activity.startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT);
            } else {
                //no activity
                return;//this would be the widget if it's null idk
            }
            //toast("Bluetooth turned on");
        } else {
            //toast("Bluetooth is already on");
        }

        //discover();
        btStartThread(BT_MAC);
    }

    public void toast(String text) {
        if (activity != null)
            Toast.makeText(activity.getApplicationContext(), text, Toast.LENGTH_SHORT).show();
        Log.i(TAG, "Toast said: "+text);
    }


    // Enter here after user selects "yes" or "no" to enabling radio

    protected void onActivityResult(int requestCode, int resultCode, Intent Data) {
        // Check which request we're responding to
        if (requestCode == REQUEST_ENABLE_BT) {
            // Make sure the request was successful
            if (resultCode == Activity.RESULT_OK) {
                // The user picked a contact.
                // The Intent's data Uri identifies which contact was selected.
                toast("status: Enabled");
            } else
                toast("status: Disabled");
        }
    }

    @SuppressLint("MissingPermission")
    private BluetoothSocket createBluetoothSocket(BluetoothDevice device) throws IOException {
        try {
            final Method m = device.getClass().getMethod("createInsecureRfcommSocketToServiceRecord", UUID.class);
            return (BluetoothSocket) m.invoke(device, BT_MODULE_UUID);
        } catch (Exception e) {
            e.printStackTrace();
            Log.e(TAG, "Could not create Insecure RFComm Connection", e);
        }
        return device.createRfcommSocketToServiceRecord(BT_MODULE_UUID);
    }

    public void btStartThread(String address) {
        if (!mBTAdapter.isEnabled()) {
            toast("BT not on!");
            return;
        }

        // Spawn a new thread to avoid blocking the GUI one
        new Thread() {
            @SuppressLint("MissingPermission")
            @Override
            public void run() {
                boolean fail = false;

                BluetoothDevice device = mBTAdapter.getRemoteDevice(address);

                try {
                    mBTSocket = createBluetoothSocket(device);
                } catch (IOException e) {
                    fail = true;
                    toast("Error creating socket");
                }
                // Establish the Bluetooth socket connection.
                try {
                    mBTSocket.connect();
                } catch (IOException e) {
                    e.printStackTrace();
                    try {
                        fail = true;
                        mBTSocket.close();
                        mHandler.obtainMessage(CONNECTING_STATUS, -1, -1)
                                .sendToTarget();
                    } catch (IOException e2) {
                        //insert code to deal with this
                        toast("Error creating socket");
                    }
                }
                if (!fail) {
                    mConnectedThread = new ConnectedThread(mBTSocket, mHandler);
                    mConnectedThread.start();

                    mHandler.obtainMessage(CONNECTING_STATUS, 1, -1, "Specified MAC")
                            .sendToTarget();
                }
            }
        }.start();

    }
}
