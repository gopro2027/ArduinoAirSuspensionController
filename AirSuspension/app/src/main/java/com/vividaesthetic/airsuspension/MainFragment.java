package com.vividaesthetic.airsuspension;

import android.app.AlertDialog;

import android.os.Handler;
import android.content.DialogInterface;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.NumberPicker;

import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;

import com.vividaesthetic.airsuspension.databinding.FragmentMainBinding;
import com.vividaesthetic.airsuspension.utils.FourPressure;
import com.vividaesthetic.airsuspension.utils.PressureUnit;

import java.lang.reflect.Field;

public class MainFragment extends Fragment {

    private FragmentMainBinding binding;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        final View rootView = requireActivity().getWindow().getDecorView().getRootView();
        rootView.getViewTreeObserver().addOnGlobalLayoutListener(
                () -> {

                    if (binding != null) {
                        int height = binding.getRoot().getHeight();
                        int width = binding.getRoot().getWidth();
                        int padding = width / 8;
                        binding.corvetteImg.getLayoutParams().height = height;
                        binding.corvetteImg.setImageResource(R.drawable.corvette_gray_untrimmed); // height doesn't update until I set the drawable
                        setLeftPadding(binding.pressureFd, padding);
                        setRightPadding(binding.pressureFp, padding);
                        setLeftPadding(binding.pressureRd, padding);
                        setRightPadding(binding.pressureRp, padding);
                    }

                });
    }

    public void setLeftPadding(View view, int padding) {
        view.setPadding(padding, view.getPaddingTop(), view.getPaddingRight(), view.getPaddingBottom());
    }

    public void setRightPadding(View view, int padding) {
        view.setPadding(view.getPaddingLeft(), view.getPaddingTop(), padding, view.getPaddingBottom());
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState
    ) {
        binding = FragmentMainBinding.inflate(inflater, container, false);
        return binding.getRoot();
    }


    private Handler handler = new Handler();

    // Define the code block to be executed
    private Runnable forceReconnectThread = new Runnable() {
        @Override
        public void run() {
            // Insert custom code here
            getAirSuspensionController().forceReconnectLoop();
            // Repeat every 2 seconds
            handler.postDelayed(forceReconnectThread, 2500);
        }
    };

    public void onViewCreated(@NonNull View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);


        handler.post(forceReconnectThread);


        binding.buttonAirup.setOnClickListener((v) -> getAirSuspensionController().airUp());

        binding.buttonAirout.setOnClickListener((v) -> {
            DialogInterface.OnClickListener dialogClickListener = (dialog, which) -> {
                switch (which){
                    case DialogInterface.BUTTON_POSITIVE:
                        //Yes button clicked
                        getAirSuspensionController().airOut();
                        break;

                    case DialogInterface.BUTTON_NEGATIVE:
                        //No button clicked
                        break;
                }
            };

            AlertDialog.Builder builder = new AlertDialog.Builder(getContext());
            builder.setMessage("Are you sure you want to air out?").setPositiveButton("Yes", dialogClickListener)
                    .setNegativeButton("No", dialogClickListener).show();
        });

        binding.buttonAirupsm.setOnClickListener((v) -> getAirSuspensionController().airSm(10));

        binding.buttonAiroutsm.setOnClickListener((v) -> getAirSuspensionController().airSm(-10));

        final int numberPickerIncrement = 5;

        binding.numberpickerSetfrontpressureD.setMinValue(20/numberPickerIncrement);
        binding.numberpickerSetfrontpressureD.setMaxValue(200/numberPickerIncrement);
        binding.numberpickerSetfrontpressureP.setMinValue(20/numberPickerIncrement);
        binding.numberpickerSetfrontpressureP.setMaxValue(200/numberPickerIncrement);
        binding.numberpickerSetrearpressureD.setMinValue(20/numberPickerIncrement);
        binding.numberpickerSetrearpressureD.setMaxValue(200/numberPickerIncrement);
        binding.numberpickerSetrearpressureP.setMinValue(20/numberPickerIncrement);
        binding.numberpickerSetrearpressureP.setMaxValue(200/numberPickerIncrement);

        NumberPicker.Formatter formatter = new NumberPicker.Formatter() {
            @Override
            public String format(int value) {
                int psi = value * numberPickerIncrement;
                return PressureUnit.convertValueFromBaseUnitToDisplay(psi+"",((MainActivity)requireActivity()).preferredUnit);
            }
        };
        binding.numberpickerSetfrontpressureD.setFormatter(formatter);
        binding.numberpickerSetfrontpressureP.setFormatter(formatter);
        binding.numberpickerSetrearpressureD.setFormatter(formatter);
        binding.numberpickerSetrearpressureP.setFormatter(formatter);



        binding.buttonSetfrontpressureD.setOnClickListener((v) -> {
            getAirSuspensionController().setFrontPressureD(PressureUnit.ofPreferredUnit(((MainActivity) requireActivity()), binding.numberpickerSetfrontpressureD.getValue()*numberPickerIncrement));
        });

        binding.buttonSetfrontpressureP.setOnClickListener((v) -> {
            getAirSuspensionController().setFrontPressureP(PressureUnit.ofPreferredUnit(((MainActivity) requireActivity()), binding.numberpickerSetfrontpressureP.getValue()*numberPickerIncrement));
        });

        binding.buttonSetrearpressureD.setOnClickListener((v) -> {
            getAirSuspensionController().setRearPressureD(PressureUnit.ofPreferredUnit(((MainActivity) requireActivity()), binding.numberpickerSetrearpressureD.getValue()*numberPickerIncrement));
        });

        binding.buttonSetrearpressureP.setOnClickListener((v) -> {
            getAirSuspensionController().setRearPressureP(PressureUnit.ofPreferredUnit(((MainActivity) requireActivity()), binding.numberpickerSetrearpressureP.getValue()*numberPickerIncrement));
        });

        binding.buttonSetProfile1.setOnClickListener((v) -> {
            getAirSuspensionController().quickAirUp(0);
        });
        binding.buttonSetProfile2.setOnClickListener((v) -> {
            getAirSuspensionController().quickAirUp(1);
        });
        binding.buttonSetProfile3.setOnClickListener((v) -> {
            getAirSuspensionController().quickAirUp(2);
        });
        binding.buttonSetProfile4.setOnClickListener((v) -> {
            getAirSuspensionController().quickAirUp(3);
        });

        binding.profileNum.setMinValue(1);
        binding.profileNum.setMaxValue(4);
        binding.buttonLoadProfile.setOnClickListener((v) -> getAirSuspensionController().readProfile(binding.profileNum.getValue() - 1));
        binding.buttonSaveProfile.setOnClickListener((v) -> getAirSuspensionController().saveToProfile(binding.profileNum.getValue() - 1));

        binding.buttonSetriseonstartenabled.setOnClickListener(v -> {
            getAirSuspensionController().setRiseOnStart(true);
        });

        binding.buttonSetriseonstartdisabled.setOnClickListener(v -> {
            getAirSuspensionController().setRiseOnStart(false);
        });

        binding.buttonSetraiseonpressuresetenabled.setOnClickListener(v -> {
            getAirSuspensionController().setRaiseOnPressureSet(true);
        });

        binding.buttonSetraiseonpressuresetdisabled.setOnClickListener(v -> {
            getAirSuspensionController().setRaiseOnPressureSet(false);
        });

        binding.buttonMaintainpressureenabled.setOnClickListener(v -> {
            getAirSuspensionController().setMaintainPressure(true);
        });

        binding.buttonMaintainpressuredisabled.setOnClickListener(v -> {
            getAirSuspensionController().setMaintainPressure(false);
        });

        getAirSuspensionController().setUpdatePressure((fp, rp, fd, rd, tank) -> {
            if (binding != null) {
                binding.pressureFp.setText(fp);
                binding.pressureRp.setText(rp);
                binding.pressureFd.setText(fd);
                binding.pressureRd.setText(rd);
                binding.pressureTank.setText(tank);
            }
        });

        // This will always be received in PSI as only the frontend values will be updated to what the user sees
        getAirSuspensionController().setUpdatePressureProfile((fp, rp, fd, rd, tank) -> {
            if (binding != null) {
                try {
                    FourPressure press = new FourPressure(fp,rp,fd,rd);
                    binding.numberpickerSetfrontpressureP.setValue(press.fp / numberPickerIncrement);
                    binding.numberpickerSetrearpressureP.setValue(press.rp / numberPickerIncrement);
                    binding.numberpickerSetfrontpressureD.setValue(press.fd / numberPickerIncrement);
                    binding.numberpickerSetrearpressureD.setValue(press.rd / numberPickerIncrement);
                    fixNumberPicker(binding.numberpickerSetfrontpressureP);
                    fixNumberPicker(binding.numberpickerSetrearpressureP);
                    fixNumberPicker(binding.numberpickerSetfrontpressureD);
                    fixNumberPicker(binding.numberpickerSetrearpressureD);
                } catch (Exception e){((MainActivity)requireActivity()).log("Could not update values! Improper data received");}
            }
        });

        fixNumberPicker(binding.numberpickerSetfrontpressureP);
        fixNumberPicker(binding.numberpickerSetrearpressureP);
        fixNumberPicker(binding.numberpickerSetfrontpressureD);
        fixNumberPicker(binding.numberpickerSetrearpressureD);
    }

    // Used to fix a bug where when you set a number pickers value and it hasn't visually loaded yet it won't show anything. With a default value of 25, this will occur when setting anything 45 and above
    private void fixNumberPicker(NumberPicker picker) {
        try {
            Field field = NumberPicker.class.getDeclaredField("mInputText");
            field.setAccessible(true);
            EditText inputText = (EditText) field.get(picker);
            inputText.setVisibility(View.INVISIBLE);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @Override
    public void onStart() {
        super.onStart();
        getAirSuspensionController().readProfile(0);
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        binding = null;
    }

    public AirSuspensionController getAirSuspensionController() {
        return MainActivity.getAirSuspensionController((MainActivity)getActivity());
    }


}