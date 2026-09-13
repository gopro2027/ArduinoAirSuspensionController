// Un-dim the screen when the device is picked up or shaken. Consumes the IMU through imu.h's
// board-agnostic accel API; owns only the motion tests and the arming policy.
//
// Scope note: this un-dims a *running* device. It cannot wake one from light sleep, because
// esp_light_sleep_start() halts loop() and no board here routes the QMI8658 INT pin to a GPIO,
// so polling is the only mechanism available.

#include "wake_on_movement.h"

#if WAKE_ON_MOVEMENT_SUPPORTED == 1

#include <math.h>

#include "util.h"

extern bool isScreenDimmed();
extern void wakeScreenFromDim();

// Detection is armed ONLY while the screen is dimmed, and fires at most once per dim. That is
// not just an optimisation: wakeScreenFromDim() re-arms the dim deadline on every call, even
// when the screen is already lit, so anything driving it per-sample would stop a device in a
// moving vehicle from ever dimming at all.
#define WAKE_POLL_MS 50 // 20 Hz; a shake is 2-5 Hz, so 10 Hz would alias it
// Ignore the act of setting the device down, and give the filter a moment to settle on the
// resting orientation before that orientation becomes the reference.
#define WAKE_ARM_DELAY_MS 750
// Lighter than auto rotate's 0.1 - this wants responsiveness, not stability.
#define WAKE_FILTER_ALPHA 0.35f
// cos(25 degrees). Below this the device has been tilted meaningfully away from where it was
// left, which is what picking it up looks like.
#define WAKE_TILT_COS_THRESHOLD 0.906f
// ~200 ms held, so a swing that passes through 25 degrees and comes back does not count.
#define WAKE_TILT_HOLD_SAMPLES 4
// Deviation of |accel| from 1 g. Well above cabin vibration, below a deliberate shake.
#define WAKE_SHAKE_G_THRESHOLD 0.35f
// Three qualifying samples inside a rolling 600 ms window. A pothole is one spike and never
// accumulates; a shake is oscillatory and trips it easily.
#define WAKE_SHAKE_WINDOW_MS 600
#define WAKE_SHAKE_MIN_HITS 3

void wakeOnMovementLoop()
{
    static bool wasDimmed = false;
    static bool armed = false;
    static bool referenceCaptured = false;
    static unsigned long armAt = 0;
    static unsigned long lastPoll = 0;
    static float filteredX = 0.0f, filteredY = 0.0f, filteredZ = 0.0f;
    static float refX = 0.0f, refY = 0.0f, refZ = 0.0f;
    static int tiltHits = 0;
    static int shakeHits = 0;
    static unsigned long shakeWindowStart = 0;

    if (!imuAvailable() || !getwakeOnMovement())
    {
        wasDimmed = false;
        armed = false;
        return;
    }

    const unsigned long now = millis();
    const bool dimmed = isScreenDimmed();

    if (dimmed != wasDimmed)
    {
        wasDimmed = dimmed;
        // Entering dim starts a fresh detection window; leaving it disarms until next time.
        armed = dimmed;
        referenceCaptured = false;
        tiltHits = 0;
        shakeHits = 0;
        armAt = now + WAKE_ARM_DELAY_MS;
    }

    if (!dimmed || !armed)
        return;
    if ((long)(now - armAt) < 0)
        return;
    if (now - lastPoll < WAKE_POLL_MS)
        return;
    lastPoll = now;

    float ax, ay, az;
    if (!imuReadAccel(&ax, &ay, &az))
        return; // transient bus failure; keep the state we have and try next pass

    if (!referenceCaptured)
    {
        // First accepted sample after the arm delay defines "where it was left".
        filteredX = refX = ax;
        filteredY = refY = ay;
        filteredZ = refZ = az;
        referenceCaptured = true;
        shakeWindowStart = now;
        return;
    }

    filteredX += (ax - filteredX) * WAKE_FILTER_ALPHA;
    filteredY += (ay - filteredY) * WAKE_FILTER_ALPHA;
    filteredZ += (az - filteredZ) * WAKE_FILTER_ALPHA;

    // ── Tilt: angle between the filtered gravity vector and the reference one ──────────────
    // Normalised dot product, so it is independent of both magnitude and of which way any
    // particular axis happens to point on this board.
    const float refMag = sqrtf(refX * refX + refY * refY + refZ * refZ);
    const float curMag = sqrtf(filteredX * filteredX + filteredY * filteredY + filteredZ * filteredZ);
    if (refMag > 0.1f && curMag > 0.1f)
    {
        const float cosAngle =
            (filteredX * refX + filteredY * refY + filteredZ * refZ) / (refMag * curMag);
        if (cosAngle < WAKE_TILT_COS_THRESHOLD)
        {
            if (++tiltHits >= WAKE_TILT_HOLD_SAMPLES)
            {
                log_i("Wake on movement: tilt (cos %.3f)", cosAngle);
                armed = false;
                wakeScreenFromDim();
                return;
            }
        }
        else
        {
            tiltHits = 0; // has to be held, not passed through
        }
    }

    // ── Shake: raw (unfiltered) magnitude deviating from 1 g, several times in a window ─────
    // Raw, because the low pass is what would smooth an oscillation back down to 1 g.
    if (now - shakeWindowStart > WAKE_SHAKE_WINDOW_MS)
    {
        shakeWindowStart = now;
        shakeHits = 0;
    }
    const float mag = sqrtf(ax * ax + ay * ay + az * az);
    if (fabsf(mag - 1.0f) > WAKE_SHAKE_G_THRESHOLD)
    {
        if (++shakeHits >= WAKE_SHAKE_MIN_HITS)
        {
            log_i("Wake on movement: shake (%.2f g)", mag);
            armed = false;
            wakeScreenFromDim();
            return;
        }
    }
}

#endif /* WAKE_ON_MOVEMENT_SUPPORTED */
