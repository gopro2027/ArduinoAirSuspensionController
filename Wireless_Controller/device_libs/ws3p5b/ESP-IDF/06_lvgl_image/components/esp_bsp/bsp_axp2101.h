#pragma once

#include <stdio.h>
#include "driver/i2c_master.h"

#define XPOWERS_CHIP_AXP2101
#include "XPowersLib.h"

// extern XPowersPMU power;

esp_err_t bsp_axp2101_init(i2c_master_bus_handle_t bus_handle);
void pmu_isr_handler(void);