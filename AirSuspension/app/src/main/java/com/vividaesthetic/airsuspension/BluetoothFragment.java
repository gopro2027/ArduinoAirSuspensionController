package com.vividaesthetic.airsuspension;

import android.app.AlertDialog;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.text.method.ScrollingMovementMethod;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.core.app.ActivityCompat;
import androidx.fragment.app.Fragment;

import com.vividaesthetic.airsuspension.databinding.FragmentBluetoothBinding;
import com.vividaesthetic.airsuspension.utils.PressureUnit;
import com.vividaesthetic.airsuspension.utils.SmplBTDevice;
import com.google.android.material.snackbar.Snackbar;

import java.util.ArrayList;
import java.util.List;
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
                    ((MainActivity) requireActivity()).log("paired device: " + deviceName + " at " + macAddress);
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

        binding.debugLogSwitch.setChecked(((MainActivity) requireActivity()).isLogVisible());
        binding.debugLogSwitch.setOnCheckedChangeListener((v,isChecked ) -> {
            ((MainActivity) requireActivity()).setLogVisible(isChecked);
        });

        binding.openFirmwareUpdateWebpage.setOnClickListener((v) -> {

            DialogInterface.OnClickListener dialogClickListener1 = (dialog, which) -> {
                switch (which){
                    case DialogInterface.BUTTON_POSITIVE:
                        //Yes button clicked
                        openUpdateWebBrowser();
                        break;

                    case DialogInterface.BUTTON_NEGATIVE:
                        //No button clicked
                        break;
                }
            };


            DialogInterface.OnClickListener dialogClickListener = (dialog, which) -> {
                switch (which){
                    case DialogInterface.BUTTON_POSITIVE:
                        //Yes button clicked
                        getAirSuspensionController().startWebService();
                        AlertDialog.Builder builder = new AlertDialog.Builder(getContext());
                        builder.setMessage("Network starting, please connect to the OASMAN-XXXXX wifi network now using the password `"+((MainActivity) requireActivity()).getBTPassword()+"`. Then click yes to proceed to the web page to upload the update file.").setPositiveButton("Yes", dialogClickListener1)
                                .setNegativeButton("No", dialogClickListener1).show();
                        break;

                    case DialogInterface.BUTTON_NEGATIVE:
                        //No button clicked
                        break;
                }
            };

            AlertDialog.Builder builder = new AlertDialog.Builder(getContext());
            builder.setMessage("Do you want to start the update web service?").setPositiveButton("Yes", dialogClickListener)
                    .setNegativeButton("No", dialogClickListener).show();
        });
    }

    public void openUpdateWebBrowser() {
        String url = "http://oasman.local";
        Intent i = new Intent(Intent.ACTION_VIEW);
        i.setData(Uri.parse(url));
        startActivity(i);
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