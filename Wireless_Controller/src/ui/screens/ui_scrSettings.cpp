#include "ui_scrSettings.h"

LV_IMG_DECLARE(navbar_settings);
ScrSettings scrSettings(navbar_settings, false);

void ScrSettings::init()
{
    Scr::init();

    // To use third party libraries, enable the define in lv_conf.h: #define LV_USE_QRCODE 1
    this->ui_qrcode = lv_qrcode_create(scrSettings.scr);
    lv_qrcode_set_size(this->ui_qrcode, 100);
    lv_qrcode_set_dark_color(this->ui_qrcode, lv_color_black());
    lv_qrcode_set_light_color(this->ui_qrcode, lv_color_white());
    const char *qr_data = "https://github.com/gopro2027/ArduinoAirSuspensionController";
    lv_qrcode_update(this->ui_qrcode, qr_data, strlen(qr_data));
    lv_obj_center(this->ui_qrcode);
}

void ScrSettings::runTouchInput(SimplePoint pos, bool down)
{
    Scr::runTouchInput(pos, down);
}

void ScrSettings::loop()
{
    Scr::loop();
}
