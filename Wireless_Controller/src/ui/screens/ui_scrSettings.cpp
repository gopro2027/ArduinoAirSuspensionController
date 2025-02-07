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

    // test values

    setupPressureLabel(this, &this->ui_s1, 0, 15 * 0, LV_ALIGN_TOP_MID, "0");
    setupPressureLabel(this, &this->ui_s2, 0, 15 * 1, LV_ALIGN_TOP_MID, "0");
    setupPressureLabel(this, &this->ui_s3, 0, 15 * 2, LV_ALIGN_TOP_MID, "0");
    setupPressureLabel(this, &this->ui_s4, 0, 15 * 3, LV_ALIGN_TOP_MID, "0");
    setupPressureLabel(this, &this->ui_s5, 0, 15 * 4, LV_ALIGN_TOP_MID, "0");
}

void ScrSettings::runTouchInput(SimplePoint pos, bool down)
{
    Scr::runTouchInput(pos, down);
}

void ScrSettings::loop()
{
    Scr::loop();
    lv_label_set_text_fmt(this->ui_s1, "Compressor Frozen: %s", statusBittset & (1 << COMPRESSOR_FROZEN) ? "Yes" : "No");
    lv_label_set_text_fmt(this->ui_s2, "Compressor Status: %s", statusBittset & (1 << COMPRESSOR_STATUS_ON) ? "On" : "Off");
    lv_label_set_text_fmt(this->ui_s3, "ACC Status: %s", statusBittset & (1 << ACC_STATUS_ON) ? "On" : "Off");
    lv_label_set_text_fmt(this->ui_s4, "Timer Expired: %s", statusBittset & (1 << TIMER_STATUS_EXPIRED) ? "Yes" : "No");
    lv_label_set_text_fmt(this->ui_s5, "Clock: %s", statusBittset & (1 << CLOCK) ? "1" : "0");
}
