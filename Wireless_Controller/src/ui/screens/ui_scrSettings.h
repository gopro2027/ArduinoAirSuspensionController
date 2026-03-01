#ifndef ui_scrSettings_h
#define ui_scrSettings_h

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

class ScrSettings : public Scr
{
    using Scr::Scr;

public:
    void updateUpdateButtonVisbility();
    lv_obj_t *pages[9];  // Array of page containers for each section
    lv_obj_t *ui_qrcode;
    Option *ui_s1;  // Compressor Frozen (text with value)
    Option *ui_s2;  // Compressor Status (switch)
    Option *ui_s3;  // ACC Status (text with value)
    Option *ui_ebrakeStatus;  // E-Brake Status (text with value)
    Option *ui_rebootbutton;  // Reboot button
    Option *ui_aiReady;  // AI Ready (text with value)
    Option *ui_aiPercentage;  // AI Percentage (text with value)
    Option *ui_aiEnabled;  // AI Enabled (switch)
    Option *ui_maintainprssure;  // Maintain Pressure (switch)
    Option *ui_riseonstart;  // Rise on start (switch)
#if ENABLE_AIR_OUT_ON_SHUTOFF
    Option *ui_airoutonshutoff;  // Fall on shutdown (switch)
#endif
    Option *ui_safetymode;  // Safety Mode (switch)
    RadioOption *ui_heightsensormode;  // Height sensor mode (radio)
    Option *ui_heightInvertFP;  // Invert Front Passenger height sensor
    Option *ui_heightInvertRP;  // Invert Rear Passenger height sensor
    Option *ui_heightInvertFD;  // Invert Front Driver height sensor
    Option *ui_heightInvertRD;  // Invert Rear Driver height sensor
    Option *ui_config1;  // Bag Max PSI (slider)
    Option *ui_config2;  // Shutoff Time (textarea)
    Option *ui_config3;  // Compressor On PSI (textarea)
    Option *ui_config4;  // Compressor Off PSI (textarea)
    Option *ui_config5;  // Pressure Sensor Rating PSI (textarea)
    Option *ui_config6;  // Bag Volume Percentage (slider)
    Option *ui_updateBtn;  // Update button (Option*)
    Option *ui_manifoldUpdateStatus;  // Manifold status (text with value)
    Option *ui_mac;  // MAC address (text with value)
    Option *ui_volts;  // Battery voltage (text with value)
    Option *ui_brightnessSlider;  // Brightness (slider)
    Option *ui_screenRotation;  // Screen rotation toggle button
    RadioOption *ui_themePreset;  // Theme preset selection (radio)
    Option *ui_rfbuttonA;
    Option *ui_rfbuttonB;
    Option *ui_rfbuttonC;
    Option *ui_rfbuttonD;

    // Track all Options/RadioOptions for cleanup
    std::vector<Option*> allOptions;
    std::vector<RadioOption*> allRadioOptions;

    void init(lv_obj_t *parent = nullptr) override;
    void loop() override;
    void cleanup() override;
    void showColorPickerModal();
};

extern ScrSettings scrSettings;
extern std::vector<ble_addr_t> authblacklist;

#endif