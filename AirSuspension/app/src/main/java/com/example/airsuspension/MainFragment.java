package com.example.airsuspension;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.util.Log;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.widget.EditText;
import android.widget.NumberPicker;

import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import androidx.navigation.fragment.NavHostFragment;

import com.example.airsuspension.databinding.FragmentMainBinding;

import java.lang.reflect.Field;

public class MainFragment extends Fragment {

    private FragmentMainBinding binding;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        final View rootView = requireActivity().getWindow().getDecorView().getRootView();
        rootView.getViewTreeObserver().addOnGlobalLayoutListener(
                () -> {

                    int height = binding.getRoot().getHeight();
                    int width = binding.getRoot().getWidth();
                    int padding = width/8;
                    Log.i("MainFragment",padding+" padding");
                    binding.corvetteImg.getLayoutParams().height = height;
                    binding.corvetteImg.setImageResource(R.drawable.corvette_gray_untrimmed); // height doesn't update until I set the drawable
                    setLeftPadding(binding.pressureFd, padding);
                    setRightPadding(binding.pressureFp, padding);
                    setLeftPadding(binding.pressureRd, padding);
                    setRightPadding(binding.pressureRp, padding);

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

    public void onViewCreated(@NonNull View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);



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
                int temp = value * numberPickerIncrement;
                return "" + temp;
            }
        };
        binding.numberpickerSetfrontpressureD.setFormatter(formatter);
        binding.numberpickerSetfrontpressureP.setFormatter(formatter);
        binding.numberpickerSetrearpressureD.setFormatter(formatter);
        binding.numberpickerSetrearpressureP.setFormatter(formatter);



        binding.buttonSetfrontpressureD.setOnClickListener((v) -> {
            getAirSuspensionController().setFrontPressureD(binding.numberpickerSetfrontpressureD.getValue()*numberPickerIncrement);
        });

        binding.buttonSetfrontpressureP.setOnClickListener((v) -> {
            getAirSuspensionController().setFrontPressureP(binding.numberpickerSetfrontpressureP.getValue()*numberPickerIncrement);
        });

        binding.buttonSetrearpressureD.setOnClickListener((v) -> {
            getAirSuspensionController().setRearPressureD(binding.numberpickerSetrearpressureD.getValue()*numberPickerIncrement);
        });

        binding.buttonSetrearpressureP.setOnClickListener((v) -> {
            getAirSuspensionController().setRearPressureP(binding.numberpickerSetrearpressureP.getValue()*numberPickerIncrement);
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
        //binding.buttonDefaultProfile.setOnClickListener((v) -> getAirSuspensionController().setBaseProfile(binding.profileNum.getValue() - 1));
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

        /*binding.buttonTestsol.setOnClickListener((v) -> {
                try {
                    getAirSuspensionController().testSolenoid(Integer.parseInt(binding.edittextTestsol.getText().toString()));
                } catch (Exception e) {
                    getAirSuspensionController().toast("Please input a valid number!");
                }
        });*/

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

        getAirSuspensionController().setUpdatePressure((fp, rp, fd, rd, tank) -> {
            if (binding != null) {
                binding.pressureFp.setText(fp);
                binding.pressureRp.setText(rp);
                binding.pressureFd.setText(fd);
                binding.pressureRd.setText(rd);
                binding.pressureTank.setText(tank);
            }
        });

        getAirSuspensionController().setUpdatePressureProfile((fp, rp, fd, rd, tank) -> {
            if (binding != null) {
                try {
                    int _fp = Integer.parseInt(fp) / numberPickerIncrement;
                    int _rp = Integer.parseInt(rp) / numberPickerIncrement;
                    int _fd = Integer.parseInt(fd) / numberPickerIncrement;
                    int _rd = Integer.parseInt(rd) / numberPickerIncrement;
                    binding.numberpickerSetfrontpressureP.setValue(_fp);
                    binding.numberpickerSetrearpressureP.setValue(_rp);
                    binding.numberpickerSetfrontpressureD.setValue(_fd);
                    binding.numberpickerSetrearpressureD.setValue(_rd);
                    fixNumberPicker(binding.numberpickerSetfrontpressureP);
                    fixNumberPicker(binding.numberpickerSetrearpressureP);
                    fixNumberPicker(binding.numberpickerSetfrontpressureD);
                    fixNumberPicker(binding.numberpickerSetrearpressureD);
                } catch (Exception e){getAirSuspensionController().toast("Could not update values! Inproper data received");}
            }
        });
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

    /*@Override
    public void onResume() {
        super.onResume();
        Log.i("MainFragment","onResume");
        getAirSuspensionController().bluetoothOn(null);
    }*/

    @Override
    public void onStart() {
        super.onStart();
        //getAirSuspensionController().bluetoothOn(null);
        getAirSuspensionController().readProfile(0);
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