// Screen auto rotate. Consumes the IMU through imu.h's board-agnostic accel API and owns
// everything display-related: the IMU->screen axis mapping, the orientation decision, the
// settle timing, and the decision about when a rebuild is safe.

#include "auto_rotate.h"

#if AUTO_ROTATE_SUPPORTED == 1

#include <math.h>

#include "util.h"

extern bool isScreenDimmed();

// Committing a rotation runs applyScreenRotation() + reinitializeScreens(), which tears down
// and rebuilds every LVGL object and blanks the panel for roughly two seconds. A phone-style
// snap debounce would make the controller unusable on a bumpy road, so an orientation has to
// hold still for AUTO_ROTATE_SETTLE_MS before it is worth that cost.
#define AUTO_ROTATE_POLL_MS 100
#define AUTO_ROTATE_SETTLE_MS 5000
// One-pole low pass, ~1 s time constant at the poll rate above. Cornering load and vibration
// ride on top of gravity; smoothing them out keeps the settle timer from being reset forever.
#define AUTO_ROTATE_FILTER_ALPHA 0.1f
// Below this much in-plane tilt the panel is near flat (face up or down) and the direction of
// the remaining gravity is noise, so no orientation is claimed.
#define AUTO_ROTATE_MIN_TILT_G 0.35f
// The dominant axis must beat the other by this ratio, moving the switch point about 8 degrees
// off the 45 degree diagonal instead of sitting right on it.
#define AUTO_ROTATE_DOMINANCE 1.3f

#define ROTATION_NONE 0xFF

// Screen-space gravity -> one of the four saved rotation values, or -1 for "no opinion"
// (panel near flat, or tilt too close to the diagonal to call).
static int orientationFromGravity(float gx, float gy)
{
    if (sqrtf(gx * gx + gy * gy) < AUTO_ROTATE_MIN_TILT_G)
        return -1;

    float mx = fabsf(gx);
    float my = fabsf(gy);

    if (my > mx)
    {
        if (my < mx * AUTO_ROTATE_DOMINANCE)
            return -1;
        // Gravity pointing down the screen means the panel is the right way up.
        return (gy > 0.0f) ? 0 : 2;
    }
    if (mx < my * AUTO_ROTATE_DOMINANCE)
        return -1;
    // Rotation 1 is the one you read with the panel's left edge toward the ground (ST7789
    // MADCTL 0x60 / Arduino_GFX 90), which is gravity pointing toward screen -X. This is a
    // property of the firmware's rotation numbering, not of how any IMU is glued down, so it
    // stays here rather than in a board's IMU_SCREEN_* macros.
    return (gx > 0.0f) ? 3 : 1;
}

void autoRotateLoop()
{
    static unsigned long lastPoll = 0;
    static unsigned long pendingSince = 0;
    static uint8_t pendingRotation = ROTATION_NONE;
    static bool filterPrimed = false;
    static float filteredX = 0.0f;
    static float filteredY = 0.0f;

    if (!imuAvailable() || !getautoRotate())
    {
        pendingRotation = ROTATION_NONE;
        filterPrimed = false;
        return;
    }

    unsigned long now = millis();
    if (now - lastPoll < AUTO_ROTATE_POLL_MS)
        return;
    lastPoll = now;

    float ax, ay, az;
    if (!imuReadAccel(&ax, &ay, &az))
        return;

    float gx = IMU_SCREEN_X(ax, ay, az);
    float gy = IMU_SCREEN_Y(ax, ay, az);

    if (!filterPrimed)
    {
        // Seed with the first sample so the filter does not have to ramp up from zero, which
        // would otherwise look like a flat panel for the first second after enabling.
        filteredX = gx;
        filteredY = gy;
        filterPrimed = true;
    }
    else
    {
        filteredX += (gx - filteredX) * AUTO_ROTATE_FILTER_ALPHA;
        filteredY += (gy - filteredY) * AUTO_ROTATE_FILTER_ALPHA;
    }

    int candidate = orientationFromGravity(filteredX, filteredY);
    if (candidate < 0)
        return; // hold whatever was pending; an ambiguous reading is not a new orientation

    if ((uint8_t)candidate != pendingRotation)
    {
        pendingRotation = (uint8_t)candidate;
        pendingSince = now;
        log_i("Auto rotate: candidate %u (g=%.2f,%.2f)", pendingRotation, filteredX, filteredY);
        return;
    }

    if (pendingRotation == getscreenRotation())
        return;
    if (now - pendingSince < AUTO_ROTATE_SETTLE_MS)
        return;

    // The settle timer has expired, but rebuilding the UI right now would be destructive:
    // it drives the backlight back to full on a dimmed screen, and it deletes any open msgbox
    // or on-screen keyboard out from under the user. Hold instead. pendingSince is already
    // past, so the rotation lands on the first pass after the blocker clears.
    if (isScreenDimmed())
        return;
    // A queued runNextFrame is almost always a pending reinitializeScreens (theme, preset count,
    // colour picker). Rotating now would rebuild the UI twice back to back, roughly four seconds
    // of splash, and the queued rebuild would land on top of ours anyway.
    if (isFunctionQueuedForNextFrame())
        return;
    if (currentScr != NULL && currentScr->isMsgBoxDisplayed())
        return;
    if (!isKeyboardHidden())
        return;

    log_i("Auto rotate: %u -> %u", getscreenRotation(), pendingRotation);
    setscreenRotation(pendingRotation);
    reinitializeScreens();
}

#endif /* AUTO_ROTATE_SUPPORTED */
