package com.example.airsuspension;

import android.os.Bundle;
import android.text.method.ScrollingMovementMethod;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;

import com.example.airsuspension.databinding.FragmentBluetoothBinding;

public class BluetoothFragment extends Fragment {

    private FragmentBluetoothBinding binding;

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState
    ) {

        binding = FragmentBluetoothBinding.inflate(inflater, container, false);
        return binding.getRoot();

    }

    public void onViewCreated(@NonNull View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        getAirSuspensionController().mReadBuffer = (TextView) getActivity().findViewById(R.id.read_buffer);
        getAirSuspensionController().mLogBuffer = (TextView) getActivity().findViewById(R.id.log_buffer);
        getAirSuspensionController().mLogBuffer.setMovementMethod(new ScrollingMovementMethod());
        binding.enableButton.setOnClickListener(view1 -> {
            //Snackbar.make(view, "Replace with your own action", Snackbar.LENGTH_LONG)
            //        .setAction("Action", null).show();
            getAirSuspensionController().queBluetoothCommand(null);
        });

    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        binding = null;
    }

    public AirSuspensionController getAirSuspensionController() {
        return MainActivity.getAirSuspensionController(getActivity());
    }

}