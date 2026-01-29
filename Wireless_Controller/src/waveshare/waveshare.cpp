#include "device_lib_exports.h"
#include "PWR_Key.h"

void waveshare_init()
{
    BAT_Init();
    PWR_Init();
}

static char voltsString[20];
static float smoothedPercent = -1.0f;  // -1 indicates not initialized
static const float SMOOTHING_FACTOR = 0.05f;  // Lower = smoother, slower response
static bool batteryCharging = false;

// Li-ion/LiPo non-linear discharge curve lookup table (3.7V nominal cell)
// More gradual curve in the mid-range where battery spends most time
static const float batteryVoltages[] = {4.20f, 4.10f, 4.00f, 3.90f, 3.85f, 3.80f, 3.75f, 3.70f, 3.65f, 3.60f, 3.50f, 3.40f, 3.30f};
static const float batteryPercents[] = {100.0f, 90.0f, 80.0f, 70.0f, 60.0f, 50.0f, 42.0f, 35.0f, 28.0f, 20.0f, 10.0f, 5.0f, 0.0f};
static const int batteryTableSize = sizeof(batteryVoltages) / sizeof(batteryVoltages[0]);

// Interpolate battery percentage from voltage using lookup table
static float voltageToPercent(float voltage) {
    if (voltage >= batteryVoltages[0]) return batteryPercents[0];
    if (voltage <= batteryVoltages[batteryTableSize - 1]) return batteryPercents[batteryTableSize - 1];

    for (int i = 0; i < batteryTableSize - 1; i++) {
        if (voltage <= batteryVoltages[i] && voltage > batteryVoltages[i + 1]) {
            float voltRange = batteryVoltages[i] - batteryVoltages[i + 1];
            float percentRange = batteryPercents[i] - batteryPercents[i + 1];
            float voltOffset = batteryVoltages[i] - voltage;
            return batteryPercents[i] - (voltOffset / voltRange) * percentRange;
        }
    }
    return 0.0f;
}

void waveshare_loop()
{
    static uint32_t pwr_next = 0;

    auto const now = millis();
    // Power-key state machine tick every 100 ms
    if (now >= pwr_next)
    {
        PWR_Loop();
        pwr_next = now + 100; // 100 ms period → Device_*_Time in 0.1s units
    }

    static uint32_t t = 0;
    if (millis() - t > 2000)
    {
        t = millis();
        float volt = BAT_Get_Volts();
        log_i("Vbat=%.3f", volt);

        // Check if charging (voltage > 4.15V indicates charging)
        batteryCharging = (volt > 4.15f);

        // Calculate raw percentage using non-linear Li-ion discharge curve
        float rawPercent = voltageToPercent(volt);

        // Apply exponential smoothing
        if (smoothedPercent < 0) {
            // First reading - initialize directly
            smoothedPercent = rawPercent;
        } else {
            // Exponential moving average: new = old + factor * (raw - old)
            smoothedPercent = smoothedPercent + SMOOTHING_FACTOR * (rawPercent - smoothedPercent);
        }

        int percent = (int)(smoothedPercent + 0.5f);  // Round to nearest
        if (percent > 100) percent = 100;
        if (percent < 0) percent = 0;

        // Calculated percentage
        snprintf(voltsString, sizeof(voltsString), "%d%%", percent);
    }
}

char *getBatteryVoltageString()
{
    return voltsString;
}

bool isBatteryCharging()
{
    return batteryCharging;
}