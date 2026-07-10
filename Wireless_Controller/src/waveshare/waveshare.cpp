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

// Slow low-pass factor (per sample) used to track constant offsets: gravity on
// the accel and zero-rate bias on the gyro. Small => slow to track, so the DC
// offset is removed but real transient motion still shows up.
static const float BIAS_ALPHA = 0.02f;
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

// Retry the (lazy) IMU init up to this many times, ~1s apart, then give up.
static const uint8_t IMU_INIT_MAX_ATTEMPTS = 10;

static float gravityX = 0.0f, gravityY = 0.0f, gravityZ = 1.0f;
// Gyro zero-rate bias (dps). MEMS gyros read several dps when perfectly still;
// without subtracting this the activity metric never drops back to "parked".
static float gyroBiasX = 0.0f, gyroBiasY = 0.0f, gyroBiasZ = 0.0f;
static float activityAvg = 0.0f;
static bool biasInit = false;

static void imu_motion_loop()
{
    static uint32_t nextPoll = 0;
    static uint32_t conditionStart = 0; // when the opposing condition began
    static uint32_t nextInitAttempt = 0;
    static uint8_t initAttempts = 0;
    static uint32_t nextDebugLog = 0;
    uint32_t now = millis();

    // Lazy, self-healing init. task_waveshare (priority 5) starts before
    // board_drivers_init() runs I2C_Init()/Wire.begin(), so an init attempt in
    // waveshare_init() would race the bus setup and fail permanently. Retry here
    // (on this task, the only Wire user) until the IMU answers.
    if (!imuPresent)
    {
        if (initAttempts >= IMU_INIT_MAX_ATTEMPTS) return;
        if (now < nextInitAttempt) return;
        nextInitAttempt = now + 1000;
        initAttempts++;
        imuPresent = QMI8658_Init();
        if (!imuPresent) return;
    }

    if (now < nextPoll) return;
    nextPoll = now + IMU_POLL_MS;

    float ax, ay, az, gx, gy, gz;
    if (!QMI8658_ReadAccelGyro(&ax, &ay, &az, &gx, &gy, &gz)) return;

    // Seed the offset estimates on the first valid sample to avoid a startup spike.
    if (!biasInit)
    {
        gravityX = ax;
        gravityY = ay;
        gravityZ = az;
        gyroBiasX = gx;
        gyroBiasY = gy;
        gyroBiasZ = gz;
        biasInit = true;
    }

    gravityX += BIAS_ALPHA * (ax - gravityX);
    gravityY += BIAS_ALPHA * (ay - gravityY);
    gravityZ += BIAS_ALPHA * (az - gravityZ);
    gyroBiasX += BIAS_ALPHA * (gx - gyroBiasX);
    gyroBiasY += BIAS_ALPHA * (gy - gyroBiasY);
    gyroBiasZ += BIAS_ALPHA * (gz - gyroBiasZ);

    float lx = ax - gravityX;
    float ly = ay - gravityY;
    float lz = az - gravityZ;
    float rx = gx - gyroBiasX;
    float ry = gy - gyroBiasY;
    float rz = gz - gyroBiasZ;
    float linMag = sqrtf(lx * lx + ly * ly + lz * lz);
    float gyroMag = sqrtf(rx * rx + ry * ry + rz * rz);

    float activity = linMag + gyroMag * GYRO_WEIGHT;
    activityAvg += ACTIVITY_BETA * (activity - activityAvg);

    // Periodic diagnostics (once per second) to confirm the IMU is being read
    // and to help tune the thresholds against real motion.
    if (now >= nextDebugLog)
    {
        nextDebugLog = now + 1000;
        log_i("IMU lin=%.4fg gyro=%.1fdps (bias %.1f,%.1f,%.1f) activity=%.4f avg=%.4f moving=%d",
              linMag, gyroMag, gyroBiasX, gyroBiasY, gyroBiasZ,
              activity, activityAvg, vehicleMoving ? 1 : 0);
    }

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
    // NOTE: IMU init is deliberately NOT done here. This task starts before
    // board_drivers_init() brings up the I2C bus, so the IMU is initialized
    // lazily (with retries) from imu_motion_loop() once the bus is ready.
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