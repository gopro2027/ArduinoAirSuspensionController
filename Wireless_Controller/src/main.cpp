#include <Arduino.h>

#include <esp32_smartdisplay.h>
#include <ui/ui.h>

#include "utils/touch_lib.h"

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
lv_obj_t *burnInRect;
void startBurnInFix();

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

    burnInRect = lv_obj_create(scrHome.scr);
    lv_obj_remove_style_all(burnInRect);
    lv_obj_set_style_bg_opa(burnInRect, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_size(burnInRect, 240, 320);
    lv_obj_center(burnInRect);
    lv_obj_set_style_bg_color(burnInRect, lv_color_hex(esp_random()), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(burnInRect, LV_OBJ_FLAG_HIDDEN);

    startBurnInFix();

    setup_touchscreen_hook();
}

auto lv_last_tick = millis();

boolean doBurnInFix = false;
void startBurnInFix()
{
    smartdisplay_lcd_set_backlight(0.01f);
    doBurnInFix = true;
    lv_obj_clear_flag(burnInRect, LV_OBJ_FLAG_HIDDEN);
}

void stopBurnInFix()
{
    smartdisplay_lcd_set_backlight(0.5f);
    doBurnInFix = false;
    lv_obj_add_flag(burnInRect, LV_OBJ_FLAG_HIDDEN);
}

void loop()
{
    auto const now = millis();

    if (doBurnInFix)
    {
        lv_obj_set_style_bg_color(burnInRect, lv_color_hex(esp_random()), LV_PART_MAIN | LV_STATE_DEFAULT);
        if (isJustPressed())
        {
            stopBurnInFix();
        }
    } else {

        // if (isJustPressed())
        // {
        //     log_i("Just Pressed %d %d ", touchX(), touchY());
        // }
        // if (isJustReleased())
        // {
        //     log_i("Just Released %d %d ", touchX(), touchY());
        // }

        // screen code
        screenLoop();
    }

    // Update the ticker
    lv_tick_inc(now - lv_last_tick);
    lv_last_tick = now;
    // Update the UI
    lv_timer_handler();
}