#include "motion_detect.h"

#ifdef HAS_MOTION_IMU

#include <math.h>
#include "qmi8658.h"

// Ported from the controller IMU_testing branch (tested in car on ws2p8), thresholds still need tuning on the manifold mount
static const float BIAS_ALPHA = 0.005f;   // slow low-pass that tracks gravity and gyro zero-rate bias (~8s at 30Hz)
static const float ACTIVITY_BETA = 0.20f; // activity moving average
static const float GYRO_WEIGHT = 0.01f;   // 1 dps -> 0.01 activity units
static const float ACTIVITY_MOVING_THRESH = 0.05f;
static const float ACTIVITY_PARKED_THRESH = 0.02f;
static const uint32_t MOTION_MOVING_CONFIRM_MS = 1000;
static const uint32_t MOTION_PARKED_CONFIRM_MS = 7000; // must be continuously quiet this long
static const uint8_t IMU_INIT_MAX_ATTEMPTS = 10;       // ~1s apart, then give up

static volatile bool imuPresent = false;
static volatile bool vehicleMoving = true; // not stationary until the IMU proves it

static float gravityX = 0.0f, gravityY = 0.0f, gravityZ = 1.0f;
static float gyroBiasX = 0.0f, gyroBiasY = 0.0f, gyroBiasZ = 0.0f;
static float activityAvg = 0.0f;
static bool biasInit = false;

void motionDetectLoop()
{
    static uint32_t conditionStart = 0; // when the opposing condition began
    static uint32_t nextInitAttempt = 0;
    static uint8_t initAttempts = 0;
    static uint32_t nextDebugLog = 0;
    uint32_t now = millis();

    if (!imuPresent)
    {
        if (initAttempts >= IMU_INIT_MAX_ATTEMPTS || now < nextInitAttempt)
            return;
        nextInitAttempt = now + 1000;
        initAttempts++;
        imuPresent = QMI8658_Init();
        if (!imuPresent)
        {
            if (initAttempts >= IMU_INIT_MAX_ATTEMPTS)
                log_e("IMU not found, isVehicleParked will always be false");
            return;
        }
    }

    float ax, ay, az, gx, gy, gz;
    if (!QMI8658_ReadAccelGyro(&ax, &ay, &az, &gx, &gy, &gz))
        return;

    if (!biasInit) // seed offsets on the first sample to avoid a startup spike
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

    if (now >= nextDebugLog) // once a second, for threshold tuning
    {
        nextDebugLog = now + 1000;
        log_i("IMU lin=%.4fg gyro=%.1fdps (bias %.1f,%.1f,%.1f) activity=%.4f avg=%.4f moving=%d",
              linMag, gyroMag, gyroBiasX, gyroBiasY, gyroBiasZ, activity, activityAvg, vehicleMoving ? 1 : 0);
    }

    if (!vehicleMoving)
    {
        if (activityAvg > ACTIVITY_MOVING_THRESH)
        {
            if (conditionStart == 0)
                conditionStart = now;
            else if (now - conditionStart >= MOTION_MOVING_CONFIRM_MS)
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
            if (conditionStart == 0)
                conditionStart = now;
            else if (now - conditionStart >= MOTION_PARKED_CONFIRM_MS)
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

bool motionImuPresent()
{
    return imuPresent;
}

bool motionImuSaysStationary()
{
    return imuPresent && !vehicleMoving;
}

#endif
