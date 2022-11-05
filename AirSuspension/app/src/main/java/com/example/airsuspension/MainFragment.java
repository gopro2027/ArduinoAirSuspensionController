package com.example.airsuspension;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import androidx.navigation.fragment.NavHostFragment;

import com.example.airsuspension.databinding.FragmentMainBinding;

public class MainFragment extends Fragment {

    private FragmentMainBinding binding;

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState
    ) {


        binding = FragmentMainBinding.inflate(inflater, container, false);
        return binding.getRoot();

    }

    public void onViewCreated(@NonNull View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        binding.buttonAirup.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                //NavHostFragment.findNavController(MainFragment.this)
                //        .navigate(R.id.action_FirstFragment_to_SecondFragment);
                getAirSuspensionController().airUp();
            }
        });

        binding.buttonAirout.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                getAirSuspensionController().airOut();
            }
        });


        binding.buttonSetfrontpressure.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                try {
                    getAirSuspensionController().setFrontPressure(Integer.parseInt(binding.edittextSetfrontpressure.getText().toString()));
                } catch (Exception e) {
                    getAirSuspensionController().toast("Please input a valid number!");
                }
            }
        });

        binding.buttonSetrearpressure.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                try {
                    getAirSuspensionController().setRearPressure(Integer.parseInt(binding.edittextSetrearpressure.getText().toString()));
                } catch (Exception e) {
                    getAirSuspensionController().toast("Please input a valid number!");
                }
            }
        });


        binding.buttonTestsol.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                try {
                    getAirSuspensionController().testSolenoid(Integer.parseInt(binding.edittextTestsol.getText().toString()));
                } catch (Exception e) {
                    getAirSuspensionController().toast("Please input a valid number!");
                }
            }
        });

        binding.buttonSetriseonstartenabled.setOnClickListener(v -> {
            getAirSuspensionController().setRiseOnStart(true);
        });

        binding.buttonSetriseonstartdisabled.setOnClickListener(v -> {
            getAirSuspensionController().setRiseOnStart(false);
        });

        getAirSuspensionController().setUpdatePressure((fp, rp,  fd,  rd,  tank) -> {
            binding.pressureFp.setText(fp);
            binding.pressureRp.setText(rp);
            binding.pressureFd.setText(fd);
            binding.pressureRd.setText(rd);
            binding.pressureTank.setText(tank);
        });
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        binding = null;
    }

    public AirSuspensionController getAirSuspensionController() {
        return ((MainActivity) getActivity()).getAirSuspensionController();
    }

}