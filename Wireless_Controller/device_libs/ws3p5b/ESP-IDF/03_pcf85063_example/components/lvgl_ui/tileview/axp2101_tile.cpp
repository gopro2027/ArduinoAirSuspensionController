
#include "axp2101_tile.h"
static lv_obj_t *list;

#define XPOWERS_CHIP_AXP2101
#include "XPowersLib.h"

extern XPowersPMU power;

lv_obj_t *label_charging;
lv_obj_t *label_battery_connect;
lv_obj_t *label_vbus_in;
lv_obj_t *label_battery_percent;
lv_obj_t *label_battery_voltage;
lv_obj_t *label_vbus_voltage;
lv_obj_t *label_system_voltage;
lv_obj_t *label_dc1_voltage;
lv_obj_t *label_aldo1_voltage;
lv_obj_t *label_bldo1_voltage;
lv_obj_t *label_bldo2_voltage;




static void axp2101_time_cb(lv_timer_t *timer)
{
    lv_label_set_text(label_charging, power.isCharging() ? "YES" : "NO");
    lv_label_set_text(label_battery_connect, power.isBatteryConnect() ?  "YES" : "NO");
    lv_label_set_text(label_vbus_in, power.isVbusIn() ?  "YES" : "NO");
    lv_label_set_text_fmt(label_battery_percent, "%d %%", power.getBatteryPercent());
    lv_label_set_text_fmt(label_battery_voltage, "%d mV", power.getBattVoltage());
    lv_label_set_text_fmt(label_vbus_voltage, "%d mV", power.getVbusVoltage());
    lv_label_set_text_fmt(label_system_voltage, "%d mV", power.getSystemVoltage());
    lv_label_set_text_fmt(label_dc1_voltage, "%d mV", power.getDC1Voltage());
    lv_label_set_text_fmt(label_aldo1_voltage, "%d mV", power.getALDO1Voltage());
    lv_label_set_text_fmt(label_bldo1_voltage, "%d mV", power.getBLDO1Voltage());
    lv_label_set_text_fmt(label_bldo2_voltage, "%d mV", power.getBLDO2Voltage());
    
}

void axp2101_tile_init(lv_obj_t *parent) 
{
    /*Create a list*/
    list = lv_list_create(parent);
    lv_obj_t *lable =  lv_label_create(parent);
    lv_obj_set_style_text_font(lable, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_label_set_text(lable, "AXP2101");
    lv_obj_align(lable, LV_ALIGN_TOP_MID, 0, 3);

    lv_obj_set_size(list, lv_pct(95), lv_pct(90));
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 30);

    lv_obj_t *list_item;

    list_item = lv_list_add_btn(list, NULL, "isCharging");
    label_charging = lv_label_create(list_item);
    lv_label_set_text(label_charging, power.isCharging() ? "YES" : "NO");

    list_item = lv_list_add_btn(list, NULL, "isBatteryConnect");
    label_battery_connect = lv_label_create(list_item);
    lv_label_set_text(label_battery_connect, power.isBatteryConnect() ?  "YES" : "NO");

    list_item = lv_list_add_btn(list, NULL, "isVbusIn");
    label_vbus_in = lv_label_create(list_item);
    lv_label_set_text(label_vbus_in, power.isVbusIn() ?  "YES" : "NO");

    list_item = lv_list_add_btn(list, NULL, "BatteryPercent");
    label_battery_percent = lv_label_create(list_item);
    lv_label_set_text_fmt(label_battery_percent, "%d %%", power.getBatteryPercent());

    list_item = lv_list_add_btn(list, NULL, "BatteryVoltage");
    label_battery_voltage = lv_label_create(list_item);
    lv_label_set_text_fmt(label_battery_voltage, "%d mV", power.getBattVoltage());
    
    list_item = lv_list_add_btn(list, NULL, "VbusVoltage");
    label_vbus_voltage = lv_label_create(list_item);
    lv_label_set_text_fmt(label_vbus_voltage, "%d mV", power.getVbusVoltage());

    list_item = lv_list_add_btn(list, NULL, "SystemVoltage");
    label_system_voltage = lv_label_create(list_item);
    lv_label_set_text_fmt(label_system_voltage, "%d mV", power.getSystemVoltage());

    list_item = lv_list_add_btn(list, NULL, "DC1Voltage");
    label_dc1_voltage = lv_label_create(list_item);
    lv_label_set_text_fmt(label_dc1_voltage, "%d mV", power.getDC1Voltage());

    list_item = lv_list_add_btn(list, NULL, "ALDO1Voltage");
    label_aldo1_voltage = lv_label_create(list_item);
    lv_label_set_text_fmt(label_aldo1_voltage, "%d mV", power.getALDO1Voltage());

    list_item = lv_list_add_btn(list, NULL, "BLDO1Voltage");
    label_bldo1_voltage = lv_label_create(list_item);
    lv_label_set_text_fmt(label_bldo1_voltage, "%d mV", power.getBLDO1Voltage());

    list_item = lv_list_add_btn(list, NULL, "BLDO2Voltage");
    label_bldo2_voltage = lv_label_create(list_item);
    lv_label_set_text_fmt(label_bldo2_voltage, "%d mV", power.getBLDO2Voltage());


    lv_timer_create(axp2101_time_cb, 3000, NULL);
}