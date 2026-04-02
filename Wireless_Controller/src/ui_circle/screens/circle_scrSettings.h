#pragma once

#ifdef SCREEN_MODE_CIRCLE

#include "ui/components/Scr.h"
#include "ui/components/option.h"
#include "ui/components/radioOption.h"

#include <vector>
#include <iostream>
#include "NimBLEDevice.h"

#include <directdownload.h>

#include "bt/ble.h"

#include "waveshare/waveshare.h"
#include "waveshare/board_driver_util.h"

#include "device_lib_exports.h"

class ScrSettings : public Scr {
    using Scr::Scr;

public:
    void updateUpdateButtonVisbility();
    lv_obj_t *pages[9];
    lv_obj_t *ui_qrcode;
    Option *ui_s1;
    Option *ui_s2;
    Option *ui_s3;
    Option *ui_ebrakeStatus;
    Option *ui_rebootbutton;
    Option *ui_aiReady;
    Option *ui_aiPercentage;
    Option *ui_aiEnabled;
    Option *ui_maintainprssure;
    Option *ui_riseonstart;
#if ENABLE_AIR_OUT_ON_SHUTOFF
    Option *ui_airoutonshutoff;
#endif
    Option *ui_safetymode;
    RadioOption *ui_heightsensormode;
    Option *ui_heightInvertFP;
    Option *ui_heightInvertRP;
    Option *ui_heightInvertFD;
    Option *ui_heightInvertRD;
    Option *ui_config1;
    Option *ui_config2;
    Option *ui_config3;
    Option *ui_config4;
    Option *ui_config5;
    Option *ui_config6;
    Option *ui_updateBtn;
    Option *ui_manifoldUpdateStatus;
    Option *ui_mac;
    Option *ui_volts;
    Option *ui_brightnessSlider;
    Option *ui_screenRotation;
    RadioOption *ui_themePreset;
    Option *ui_rfbuttonA;
    Option *ui_rfbuttonB;
    Option *ui_rfbuttonC;
    Option *ui_rfbuttonD;

    std::vector<Option *> allOptions;
    std::vector<RadioOption *> allRadioOptions;

    void init(lv_obj_t *parent = nullptr) override;
    void loop() override;
    void cleanup() override;
    void showColorPickerModal();
    void updateHeightInvertOptionsVisibility(bool isLevelMode);
};

extern ScrSettings scrSettings;
extern std::vector<ble_addr_t> authblacklist;

#endif /* SCREEN_MODE_CIRCLE */
