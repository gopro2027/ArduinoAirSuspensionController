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

        binding.buttonAirup.setOnClickListener((v) -> getAirSuspensionController().airUp());

        binding.buttonAirout.setOnClickListener((v) -> getAirSuspensionController().airOut()
        );

        binding.buttonSetfrontpressureD.setOnClickListener((v) -> {
                try {
                    getAirSuspensionController().setFrontPressureD(Integer.parseInt(binding.edittextSetfrontpressureD.getText().toString()));
                } catch (Exception e) {
                    getAirSuspensionController().toast("Please input a valid number!");
                }
        });

        binding.buttonSetfrontpressureP.setOnClickListener((v) -> {
                try {
                    getAirSuspensionController().setFrontPressureP(Integer.parseInt(binding.edittextSetfrontpressureP.getText().toString()));
                } catch (Exception e) {
                    getAirSuspensionController().toast("Please input a valid number!");
                }
            }
        );

        binding.buttonSetrearpressureD.setOnClickListener((v) -> {
                try {
                    getAirSuspensionController().setRearPressureD(Integer.parseInt(binding.edittextSetrearpressureD.getText().toString()));
                } catch (Exception e) {
                    getAirSuspensionController().toast("Please input a valid number!");
                }

        });

        binding.buttonSetrearpressureP.setOnClickListener((v) -> {
                try {
                    getAirSuspensionController().setRearPressureP(Integer.parseInt(binding.edittextSetrearpressureP.getText().toString()));
                } catch (Exception e) {
                    getAirSuspensionController().toast("Please input a valid number!");
                }
            }
        );

        binding.profileNum.setMinValue(1);
        binding.profileNum.setMaxValue(4);
        binding.buttonLoadProfile.setOnClickListener((v) -> getAirSuspensionController().readProfile(binding.profileNum.getValue()-1));
        binding.buttonSaveProfile.setOnClickListener((v) -> getAirSuspensionController().saveToProfile(binding.profileNum.getValue()-1));
        binding.buttonDefaultProfile.setOnClickListener((v) -> getAirSuspensionController().setBaseProfile(binding.profileNum.getValue()-1));
        /*
        binding.buttonLoad1.setOnClickListener((v) -> getAirSuspensionController().readProfile(0));
        binding.buttonLoad2.setOnClickListener((v) -> getAirSuspensionController().readProfile(1));
        binding.buttonLoad3.setOnClickListener((v) -> getAirSuspensionController().readProfile(2));

        binding.buttonSave1.setOnClickListener((v) -> getAirSuspensionController().saveToProfile(0));
        binding.buttonSave2.setOnClickListener((v) -> getAirSuspensionController().saveToProfile(1));
        binding.buttonSave3.setOnClickListener((v) -> getAirSuspensionController().saveToProfile(2));

        binding.buttonBp1.setOnClickListener((v) -> getAirSuspensionController().setBaseProfile(0));
        binding.buttonBp2.setOnClickListener((v) -> getAirSuspensionController().setBaseProfile(1));
        binding.buttonBp3.setOnClickListener((v) -> getAirSuspensionController().setBaseProfile(2));
*/

        binding.buttonTestsol.setOnClickListener((v) -> {
                try {
                    getAirSuspensionController().testSolenoid(Integer.parseInt(binding.edittextTestsol.getText().toString()));
                } catch (Exception e) {
                    getAirSuspensionController().toast("Please input a valid number!");
                }
        });

        binding.buttonSetriseonstartenabled.setOnClickListener(v -> {
            getAirSuspensionController().setRiseOnStart(true);
        });

        binding.buttonSetriseonstartdisabled.setOnClickListener(v -> {
            getAirSuspensionController().setRiseOnStart(false);
        });

        getAirSuspensionController().setUpdatePressure((fp, rp,  fd,  rd,  tank) -> {
            if (binding != null) {
                binding.pressureFp.setText(fp);
                binding.pressureRp.setText(rp);
                binding.pressureFd.setText(fd);
                binding.pressureRd.setText(rd);
                binding.pressureTank.setText(tank);
            }
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