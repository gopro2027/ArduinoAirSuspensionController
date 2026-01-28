#include "device_lib_exports.h"
#include "PWR_Key.h"

void waveshare_init()
{
    BAT_Init();
    PWR_Init();
}

static char voltsString[20];
static float smoothedPercent = -1.0f;  // -1 indicates not initialized
static const float SMOOTHING_FACTOR = 0.15f;  // Lower = smoother, slower response
static bool batteryCharging = false;

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

        // Calculate raw percentage
        float v_min = 3.5f;
        float v_max = 4.09f;
        float clamped_volt = volt;
        if (clamped_volt > v_max) clamped_volt = v_max;
        if (clamped_volt < v_min) clamped_volt = v_min;
        float rawPercent = ((clamped_volt - v_min) / (v_max - v_min)) * 100.0f;

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