#pragma once

#include <Arduino.h>
#include "lvgl.h"

/** Compact top overlay: battery + optional alert dot (circular UI).
 *  Positioned inside the circular bezel near top center. */
class CircleStatusbarMini {
public:
    lv_obj_t *container = nullptr;
    lv_obj_t *batteryIcon = nullptr;
    lv_obj_t *batteryLabel = nullptr;
    lv_obj_t *alertDot = nullptr;
    lv_obj_t *vehicleVoltageLabel = nullptr; // manifold's vehicle battery voltage, hidden when it reports 0

    void create(lv_obj_t *parent);
    void update();
    void cleanup();

    /** Show/hide the battery icon + percentage ("Show Battery" setting). */
    void setBatteryVisible(bool batteryVisible);

private:
    uint16_t cachedVehicleDeciVolts = 0xFFFF;
    void refreshBattery();
    void refreshVehicleBattery();
};

extern CircleStatusbarMini circleStatusbarMini;
