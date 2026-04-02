#include "device_lib_exports.h"

#ifdef SCREEN_MODE_CIRCLE

#include "circle_statusbar.h"
#include "utils/util.h"
#include "ui/components/alert.h"
#include "waveshare/waveshare.h"

CircleStatusbarMini circleStatusbarMini;

void CircleStatusbarMini::create(lv_obj_t *parent)
{
    container = lv_obj_create(parent);
    lv_obj_remove_style_all(container);
    lv_obj_set_size(container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_hor(container, 6, 0);
    lv_obj_set_style_pad_ver(container, 2, 0);
    lv_obj_set_style_pad_column(container, 4, 0);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    /* Inset from top bezel — 20px on a 360px circle keeps it visible */
    lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_remove_flag(container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(container, LV_OBJ_FLAG_FLOATING);

    alertDot = lv_obj_create(container);
    lv_obj_remove_style_all(alertDot);
    lv_obj_set_size(alertDot, 8, 8);
    lv_obj_set_style_radius(alertDot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(alertDot, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(alertDot, lv_color_hex(0xFF4444), 0);
    lv_obj_add_flag(alertDot, LV_OBJ_FLAG_HIDDEN);

    batteryIcon = lv_label_create(container);
    lv_obj_set_style_text_font(batteryIcon, &lv_font_montserrat_14, 0);

    batteryLabel = lv_label_create(container);
    lv_label_set_text(batteryLabel, "--%");
    lv_obj_set_style_text_font(batteryLabel, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(batteryLabel, lv_color_hex(0xC0C0C0), 0);
}

void CircleStatusbarMini::refreshBattery()
{
    if (!batteryIcon || !batteryLabel)
        return;

    char *battStr = getBatteryVoltageString();
    int percent = atoi(battStr);
    bool charging = isBatteryCharging();

    const char *icon = LV_SYMBOL_BATTERY_FULL;
    if (charging)
        icon = LV_SYMBOL_CHARGE;
    else if (percent > 75)
        icon = LV_SYMBOL_BATTERY_FULL;
    else if (percent > 50)
        icon = LV_SYMBOL_BATTERY_3;
    else if (percent > 25)
        icon = LV_SYMBOL_BATTERY_2;
    else if (percent > 10)
        icon = LV_SYMBOL_BATTERY_1;
    else
        icon = LV_SYMBOL_BATTERY_EMPTY;

    lv_label_set_text(batteryIcon, icon);

    uint32_t c = 0x4ADE80;
    if (charging)
        c = 0x60A5FA;
    else if (percent <= 20)
        c = 0xFF6B6B;
    else if (percent <= 50)
        c = 0xFBBF24;
    lv_obj_set_style_text_color(batteryIcon, lv_color_hex(c), 0);

    static char buf[24];
    snprintf(buf, sizeof(buf), "%s", battStr);
    lv_label_set_text(batteryLabel, buf);
}

void CircleStatusbarMini::update()
{
    refreshBattery();

    if (alertDot) {
        if (isGlobalAlertActive() && !isGlobalAlertDismissed())
            lv_obj_remove_flag(alertDot, LV_OBJ_FLAG_HIDDEN);
        else
            lv_obj_add_flag(alertDot, LV_OBJ_FLAG_HIDDEN);
    }
}

void CircleStatusbarMini::cleanup()
{
    batteryIcon = batteryLabel = alertDot = nullptr;
    container = nullptr;
}

#endif /* SCREEN_MODE_CIRCLE */
