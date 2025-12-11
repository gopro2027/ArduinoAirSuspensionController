#include "PMU_AXP2101.h"

#include "axp2101_pmic.h"
#include "pmic_cache.h"
#include "i2c_guard.h"

// Thin compatibility layer: use the unified PMIC driver so the bus/PMIC
// are only initialised once.
static bool pmu_initialised = false;

#ifndef PMU_IRQ_PIN
#define PMU_IRQ_PIN -1 
#endif

bool PMU_init()
{
    if (pmu_initialised) {
        return true;
    }

    // Ensure shared bus guard is up (safe if called multiple times).
    i2c_guard_init(&Wire, 20);

    if (!pmic::begin(&Wire, I2C_SDA, I2C_SCL, PMU_IRQ_PIN)) {
        Serial.println("[PMU] begin() failed");
        return false;
    }

    pmic::enable_display_power(true);
    pmic_cache_start(1000 /*ms*/, 0 /*core 0*/);
    pmic::set_measure_period_ms(2000);

    pmu_initialised = true;
    Serial.println("[PMU] initialised via unified PMIC driver");
    return true;
}

uint16_t PMU_getBatteryVoltage_mV()
{
    if (!pmu_initialised && !PMU_init()) {
        return 0;
    }
    float v = pmic::battery_voltage();
    return isfinite(v) ? static_cast<uint16_t>(v * 1000.0f) : 0;
}

float PMU_getBatteryVoltage_V()
{
    return static_cast<float>(PMU_getBatteryVoltage_mV()) / 1000.0f;
}

uint8_t PMU_getBatteryPercent()
{
    if (!pmu_initialised && !PMU_init()) {
        return 0;
    }
    int pct = pmic::battery_percent();
    return (pct >= 0) ? static_cast<uint8_t>(pct) : 0;
}

// ----------------------------------------------------------
// Optional stubs to keep older code happy
// ----------------------------------------------------------

void PMU_loop() {}
void printPMU() {}
void enterPmuSleep(void) {}
