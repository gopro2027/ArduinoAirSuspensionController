#include "device_lib_exports.h"
#include "PWR_Key.h"
#include <math.h>

#if HAS_MOTION_IMU == 1
// ───────── IMU moving/parked detection ─────────
// Runs entirely in task_waveshare (off the LVGL thread). The UI reads the
// result via isVehicleMoving(); see Hard rule 7 (no lv_* from other tasks).

static bool imuPresent = false;
static bool vehicleMoving = false;

// Poll cadence (~30 Hz is plenty for motion classification).
static const uint32_t IMU_POLL_MS = 33;

// Gravity estimate low-pass factor (per sample). Small => slow to track,
// so real gravity is removed but sustained driving accel still shows up.
static const float GRAVITY_ALPHA = 0.02f;
// Activity smoothing (exponential moving average of the per-sample metric).
static const float ACTIVITY_BETA = 0.20f;

// Combined activity metric = linear-accel magnitude (g) + gyro magnitude (dps)
// scaled into the same rough range. Thresholds are conservative and meant to
// be tuned on the bench/vehicle; this only drives a display label.
static const float GYRO_WEIGHT = 0.01f;   // 1 dps -> 0.01 units
static const float ACTIVITY_MOVING_THRESH = 0.05f;
static const float ACTIVITY_PARKED_THRESH = 0.02f;

// Hysteresis: the opposing condition must hold this long before switching.
static const uint32_t MOVING_CONFIRM_MS = 1000;
static const uint32_t PARKED_CONFIRM_MS = 3000;

static float gravityX = 0.0f, gravityY = 0.0f, gravityZ = 1.0f;
static float activityAvg = 0.0f;
static bool gravityInit = false;

static void imu_motion_loop()
{
    if (!imuPresent) return;

    static uint32_t nextPoll = 0;
    static uint32_t conditionStart = 0; // when the opposing condition began
    uint32_t now = millis();
    if (now < nextPoll) return;
    nextPoll = now + IMU_POLL_MS;

    float ax, ay, az, gx, gy, gz;
    if (!QMI8658_ReadAccelGyro(&ax, &ay, &az, &gx, &gy, &gz)) return;

    // Seed the gravity estimate on first valid sample to avoid a startup spike.
    if (!gravityInit)
    {
        gravityX = ax;
        gravityY = ay;
        gravityZ = az;
        gravityInit = true;
    }

    gravityX += GRAVITY_ALPHA * (ax - gravityX);
    gravityY += GRAVITY_ALPHA * (ay - gravityY);
    gravityZ += GRAVITY_ALPHA * (az - gravityZ);

    float lx = ax - gravityX;
    float ly = ay - gravityY;
    float lz = az - gravityZ;
    float linMag = sqrtf(lx * lx + ly * ly + lz * lz);
    float gyroMag = sqrtf(gx * gx + gy * gy + gz * gz);

    float activity = linMag + gyroMag * GYRO_WEIGHT;
    activityAvg += ACTIVITY_BETA * (activity - activityAvg);

    if (!vehicleMoving)
    {
        if (activityAvg > ACTIVITY_MOVING_THRESH)
        {
            if (conditionStart == 0) conditionStart = now;
            else if (now - conditionStart >= MOVING_CONFIRM_MS)
            {
                vehicleMoving = true;
                conditionStart = 0;
            }
        }
        else
        {
            conditionStart = 0;
        }
    }
    else
    {
        if (activityAvg < ACTIVITY_PARKED_THRESH)
        {
            if (conditionStart == 0) conditionStart = now;
            else if (now - conditionStart >= PARKED_CONFIRM_MS)
            {
                vehicleMoving = false;
                conditionStart = 0;
            }
        }
        else
        {
            conditionStart = 0;
        }
    }
}
#endif // HAS_MOTION_IMU

void waveshare_init()
{
    BAT_Init();
    PWR_Init();
#if HAS_MOTION_IMU == 1
    imuPresent = QMI8658_Init();
#endif
}

bool isVehicleMoving()
{
#if HAS_MOTION_IMU == 1
    return vehicleMoving;
#else
    return false;
#endif
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

#if HAS_MOTION_IMU == 1
    imu_motion_loop();
#endif

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