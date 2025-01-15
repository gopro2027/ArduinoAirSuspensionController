#include <Arduino.h>

#include <esp32_smartdisplay.h>
#include <ui/ui.h>

#include "touch_lib.h"

void OnAddOneClicked(lv_event_t *e)
{
    static uint32_t cnt = 0;
    cnt++;
    lv_label_set_text_fmt(scrMain.ui_lblCountValue, "%u", cnt);
}

void OnRotateClicked(lv_event_t *e)
{
    auto disp = lv_disp_get_default();
    auto rotation = (lv_display_rotation_t)((lv_disp_get_rotation(disp) + 1) % (LV_DISPLAY_ROTATION_270 + 1));
    lv_display_set_rotation(disp, rotation);
}

void setup()
{
#ifdef ARDUINO_USB_CDC_ON_BOOT
    delay(5000);
#endif
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    log_i("Board: %s", BOARD_NAME);
    log_i("CPU: %s rev%d, CPU Freq: %d Mhz, %d core(s)", ESP.getChipModel(), ESP.getChipRevision(), getCpuFrequencyMhz(), ESP.getChipCores());
    log_i("Free heap: %d bytes", ESP.getFreeHeap());
    log_i("Free PSRAM: %d bytes", ESP.getPsramSize());
    log_i("SDK version: %s", ESP.getSdkVersion());

    smartdisplay_init();

    __attribute__((unused)) auto disp = lv_disp_get_default();
    // lv_disp_set_rotation(disp, LV_DISP_ROT_90);
    // lv_disp_set_rotation(disp, LV_DISP_ROT_180);
    // lv_disp_set_rotation(disp, LV_DISP_ROT_270);

    ui_init();

    // To use third party libraries, enable the define in lv_conf.h: #define LV_USE_QRCODE 1
    // auto ui_qrcode = lv_qrcode_create(ui_scrMain);
    // lv_qrcode_set_size(ui_qrcode, 100);
    // lv_qrcode_set_dark_color(ui_qrcode, lv_color_black());
    // lv_qrcode_set_light_color(ui_qrcode, lv_color_white());
    // const char *qr_data = "https://github.com/rzeldent/esp32-smartdisplay";
    // lv_qrcode_update(ui_qrcode, qr_data, strlen(qr_data));
    // lv_obj_center(ui_qrcode);

    setup_touchscreen_hook();
}

auto lv_last_tick = millis();

#include <esp32_smartdisplay.h>
#include <esp_lcd_panel_ops.h>

#include <esp_lcd_touch.h>

void loop()
{
    auto const now = millis();

    if (isJustPressed())
    {
        log_i("Just Pressed %d %d ", touchX(), touchY());
    }
    if (isJustReleased())
    {
        log_i("Just Released %d %d ", touchX(), touchY());
    }

    // screen code
    screenLoop();

    // Update the ticker
    lv_tick_inc(now - lv_last_tick);
    lv_last_tick = now;
    // Update the UI
    lv_timer_handler();
}