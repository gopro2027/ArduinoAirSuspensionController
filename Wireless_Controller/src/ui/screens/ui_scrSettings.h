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

#if defined(WAVESHARE_BOARD)
#include "waveshare/waveshare.h"
#endif

class ScrSettings : public Scr
{
    using Scr::Scr;
    void updateUpdateButtonVisbility();

public:
    lv_obj_t *optionsContainer;
    lv_obj_t *ui_qrcode;
    Option *ui_s1;
    Option *ui_s2;
    Option *ui_aiReady;
    Option *ui_aiPercentage;
    Option *ui_aiEnabled;
    Option *ui_s3;
    Option *ui_rebootbutton;
    Option *ui_s4;
    Option *ui_s5;
    Option *ui_maintainprssure;
    Option *ui_riseonstart;
    Option *ui_safetymode;
    Option *ui_airoutonshutoff;
    RadioOption *ui_heightsensormode;
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
    void init();
    void runTouchInput(SimplePoint pos, bool down);
    void loop();
};

extern ScrSettings scrSettings;
extern std::vector<ble_addr_t> authblacklist;

#endif