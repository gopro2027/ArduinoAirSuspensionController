package com.example.airsuspension;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.util.Pair;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Spinner;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.core.app.ActivityCompat;
import androidx.fragment.app.Fragment;

import com.example.airsuspension.databinding.FragmentBluetoothBinding;
import com.example.airsuspension.utils.PressureUnit;
import com.example.airsuspension.utils.SmplBTDevice;
import com.google.android.material.snackbar.Snackbar;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.stream.Collectors;

public class BluetoothFragment extends Fragment {

    private FragmentBluetoothBinding binding;
    private String LOGTAG = "BluetoothFragment";
    private ArrayAdapter<String> adapterbt = null;

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState
    ) {

        binding = FragmentBluetoothBinding.inflate(inflater, container, false);
        return binding.getRoot();

    }


    private final List<SmplBTDevice> btDevices = new ArrayList<>();

    void addBTDevice(SmplBTDevice device) {
        if (!btDevices.contains(device)) {
            btDevices.add(device);
            if (adapterbt != null) {
                adapterbt.add(device.toString());
                adapterbt.notifyDataSetChanged();
            }
        }
    }

    public void onViewCreated(@NonNull View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        MainActivity.onReveivedBluetoothDevice = (device) -> {
            addBTDevice(device);
        };

        getAirSuspensionController().mReadBuffer = (TextView) getActivity().findViewById(R.id.read_buffer);
        getAirSuspensionController().mLogBuffer = (TextView) getActivity().findViewById(R.id.log_buffer);
        getAirSuspensionController().mLogBuffer.setMovementMethod(new ScrollingMovementMethod());
        binding.findBTDevicesBtn.setOnClickListener(view1 -> {
            if (Build.VERSION.SDK_INT < Build.VERSION_CODES.S || ActivityCompat.checkSelfPermission(requireContext(), android.Manifest.permission.BLUETOOTH_SCAN) == PackageManager.PERMISSION_GRANTED) {
                Snackbar.make(view, "Adding devices to dropdown device list!", Snackbar.LENGTH_LONG).show();
                BluetoothAdapter btAdapter = BluetoothAdapter.getDefaultAdapter();
                if (btAdapter != null) {
                    // if already scanning ... cancel
                    if (btAdapter.isDiscovering()) {
                        btAdapter.cancelDiscovery();
                    }
                    btAdapter.startDiscovery();
                } else {
                    Snackbar.make(view, "ERROR: You do not have bluetooth!", Snackbar.LENGTH_LONG).show();
                }
            } else {
                Snackbar.make(view, "ERROR: You have not enabled this permission!", Snackbar.LENGTH_LONG).show();
            }
        });

        binding.btPassword.setText(((MainActivity) requireActivity()).getBTPassword());
        binding.btPasswordButton.setOnClickListener(v -> {
            ((MainActivity) requireActivity()).saveBTPassword(binding.btPassword.getText().toString());
        });

        String[] items = new String[]{"PSI", "BAR", "PASCAL"};
        ArrayAdapter<String> adapter = new ArrayAdapter<>(requireContext(), R.layout.spinner_dropdown_item, items);
        binding.preferenceDropdown.setAdapter(adapter);
        binding.preferenceDropdown.setSelection(((MainActivity) requireActivity()).preferredUnit.getId());

        binding.preferenceDropdown.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView,
                                       int position, long id) {
                ((MainActivity) requireActivity()).savePressureUnit(PressureUnit.Unit.fromId(position));
            }

            @Override
            public void onNothingSelected(AdapterView<?> parentView) {
                // your code here
            }
        });

        // Setup connected devices dropdown list
        ((MainActivity)requireActivity()).enableBT(() -> {
            //if (ActivityCompat.checkSelfPermission(requireContext(), android.Manifest.permission.BLUETOOTH_CONNECT) == PackageManager.PERMISSION_GRANTED) {

                int selectedIndex = 0;

                //add a default "none selected"
                addBTDevice(new SmplBTDevice("", "-- none --"));

                //check our selected saved device and add it
                SmplBTDevice savedDevice = ((MainActivity) requireActivity()).getBT();
                if (!savedDevice.getMac().isEmpty()) {
                    addBTDevice(savedDevice);
                    selectedIndex = 1;
                }

                //check all of the saved devices and add them
            if (BluetoothAdapter.getDefaultAdapter() != null) {
                Set<BluetoothDevice> pairedDevices = BluetoothAdapter.getDefaultAdapter().getBondedDevices();
                for (BluetoothDevice d : pairedDevices) {
                    String deviceName = d.getName();
                    String macAddress = d.getAddress();
                    ((MainActivity) requireActivity()).logWithToastQuick("paired device: " + deviceName + " at " + macAddress);
                    addBTDevice(new SmplBTDevice(macAddress, deviceName));
                }
            }

                adapterbt = new ArrayAdapter<>(requireContext(), R.layout.spinner_dropdown_item, btDevices.stream().map(SmplBTDevice::toString).collect(Collectors.toList()));
                binding.btDropdown.setAdapter(adapterbt);
                binding.btDropdown.setSelection(selectedIndex);

                binding.btDropdown.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
                    @Override
                    public void onItemSelected(AdapterView<?> parentView, View selectedItemView,
                                               int position, long id) {
                        ((MainActivity) requireActivity()).saveBT(btDevices.get(position));
                    }

                    @Override
                    public void onNothingSelected(AdapterView<?> parentView) {
                        // your code here
                    }
                });
            //}



        });
    }


    @Override
    public void onDestroyView() {
        super.onDestroyView();
        BluetoothAdapter btAdapter = BluetoothAdapter.getDefaultAdapter();
        if (btAdapter != null) {
            if (Build.VERSION.SDK_INT < Build.VERSION_CODES.S || ActivityCompat.checkSelfPermission(requireContext(), android.Manifest.permission.BLUETOOTH_SCAN) == PackageManager.PERMISSION_GRANTED) {
                if (btAdapter.isDiscovering()) {
                    btAdapter.cancelDiscovery();
                }
            }
        }
        binding = null;
    }

    public AirSuspensionController getAirSuspensionController() {
        return MainActivity.getAirSuspensionController((MainActivity) getActivity());
    }

}