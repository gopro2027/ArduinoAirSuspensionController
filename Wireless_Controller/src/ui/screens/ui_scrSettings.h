#ifndef ui_scrSettings_h
#define ui_scrSettings_h

#include "ui/components/Scr.h"
#include "ui/components/option.h"
#include "ui/components/radioOption.h"
class ScrSettings : public Scr
{
    using Scr::Scr;

public:
    lv_obj_t *optionsContainer;
    lv_obj_t *ui_qrcode;
    Option *ui_s1;
    Option *ui_s2;
    Option *ui_s3;
    Option *ui_s4;
    Option *ui_s5;
    Option *ui_maintainprssure;
    Option *ui_riseonstart;
    Option *ui_airoutonshutoff;
    Option *ui_config1;
    Option *ui_config2;
    Option *ui_config3;
    Option *ui_config4;
    Option *ui_config5;
    void init();
    void runTouchInput(SimplePoint pos, bool down);
    void loop();
};

extern ScrSettings scrSettings;

#endif