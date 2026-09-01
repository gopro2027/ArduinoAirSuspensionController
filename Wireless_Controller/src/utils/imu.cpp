#include "imu.h"

#if AUTO_ROTATE_SUPPORTED == 1

#include <Wire.h>
#include <math.h>

#include "util.h"

// Some boards share Wire between the LVGL task and a PMIC poller pinned to the other core,
// and hand out a mutex to serialise it (see IMU_I2C_GUARDED in the board's device_lib_exports.h).
// Boards that only ever touch I2C from the LVGL task have no such mutex and need no locking.
// A missed lock just fails the read: auto rotate holds its current orientation for one more
// poll, which is invisible next to the five second settle time.
#if defined(IMU_I2C_GUARDED) && IMU_I2C_GUARDED == 1
#ifndef IMU_I2C_LOCK_MS
#define IMU_I2C_LOCK_MS 10
#endif
static inline bool imuBusLock() { return i2c_lock(IMU_I2C_LOCK_MS); }
static inline void imuBusUnlock() { i2c_unlock(); }
#else
static inline bool imuBusLock() { return true; }
static inline void imuBusUnlock() {}
#endif

extern bool isScreenDimmed();

// ── QMI8658 ───────────────────────────────────────────────────────────────────────────────
// The 6-axis IMU fitted to the Waveshare panels that have one. Only the accelerometer is
// brought up: auto rotate needs the gravity vector and nothing else, and leaving the gyro
// disabled keeps the part near its idle current.
#define QMI8658_REG_WHO_AM_I 0x00
#define QMI8658_REG_CTRL1 0x02
#define QMI8658_REG_CTRL2 0x03
#define QMI8658_REG_CTRL7 0x08
#define QMI8658_REG_AX_L 0x35
#define QMI8658_REG_RESET 0x60

#define QMI8658_WHO_AM_I_VALUE 0x05

// CTRL1: address auto-increment only. Bit 5 (big endian) stays clear so the burst read below
// can assemble little-endian words; the INT pins and the oscillator-disable bit stay clear too.
#define QMI8658_CTRL1_VALUE 0x40
// CTRL2: accel full scale +-4 g (bits 6:4 = 001) at 125 Hz (bits 3:0 = 0110). +-2 g would give
// finer tilt resolution but road bumps clip it, and a clipped axis reads as a fake orientation.
#define QMI8658_CTRL2_VALUE 0x16
// CTRL7: accelerometer enabled, gyro left off.
#define QMI8658_CTRL7_VALUE 0x01
// LSB per g at the +-4 g range selected above.
#define QMI8658_ACC_LSB_PER_G 8192.0f

// ── Auto rotate tuning ────────────────────────────────────────────────────────────────────
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

static bool imuPresent = false;
static uint8_t imuAddr = 0;

// Both of these take the bus for the whole transaction and release it on every exit path -
// a repeated start split across a lock boundary would let the other core's PMIC poller
// address its own chip between our register write and our read.
static bool imuWriteReg(uint8_t reg, uint8_t value)
{
    if (!imuBusLock())
        return false;
    Wire.beginTransmission(imuAddr);
    Wire.write(reg);
    Wire.write(value);
    bool ok = (Wire.endTransmission(true) == 0);
    imuBusUnlock();
    return ok;
}

static bool imuReadRegs(uint8_t reg, uint8_t *buf, uint8_t len)
{
    if (!imuBusLock())
        return false;
    bool ok = false;
    Wire.beginTransmission(imuAddr);
    Wire.write(reg);
    if (Wire.endTransmission(true) == 0 && Wire.requestFrom(imuAddr, len) == len)
    {
        for (uint8_t i = 0; i < len; i++)
            buf[i] = Wire.read();
        ok = true;
    }
    imuBusUnlock();
    return ok;
}

// Both SA0 strap options are in the wild across board revisions, so probe rather than assume.
static bool imuProbe(uint8_t addr)
{
    imuAddr = addr;
    uint8_t who = 0;
    return imuReadRegs(QMI8658_REG_WHO_AM_I, &who, 1) && who == QMI8658_WHO_AM_I_VALUE;
}

void imuInit()
{
    if (!imuProbe(0x6B) && !imuProbe(0x6A))
    {
        imuPresent = false;
        log_i("IMU: no QMI8658 on the bus - auto rotate unavailable");
        return;
    }

    imuWriteReg(QMI8658_REG_RESET, 0xB0);
    delay(20); // datasheet soft reset settling time

    imuWriteReg(QMI8658_REG_CTRL1, QMI8658_CTRL1_VALUE);
    imuWriteReg(QMI8658_REG_CTRL2, QMI8658_CTRL2_VALUE);
    imuWriteReg(QMI8658_REG_CTRL7, QMI8658_CTRL7_VALUE);
    delay(10); // let the first conversion land before anything reads

    imuPresent = true;
    log_i("IMU: QMI8658 found at 0x%02X - auto rotate available", imuAddr);
}

bool imuAvailable()
{
    return imuPresent;
}

bool imuReadGravity(float *gx, float *gy, float *gz)
{
    uint8_t b[6];
    if (!imuPresent || !imuReadRegs(QMI8658_REG_AX_L, b, sizeof(b)))
        return false;

    float ax = (int16_t)((uint16_t)b[0] | ((uint16_t)b[1] << 8)) / QMI8658_ACC_LSB_PER_G;
    float ay = (int16_t)((uint16_t)b[2] | ((uint16_t)b[3] << 8)) / QMI8658_ACC_LSB_PER_G;
    float az = (int16_t)((uint16_t)b[4] | ((uint16_t)b[5] << 8)) / QMI8658_ACC_LSB_PER_G;

    *gx = IMU_SCREEN_X(ax, ay, az);
    *gy = IMU_SCREEN_Y(ax, ay, az);
    *gz = az;
    return true;
}

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
    // ; was: (gx > 0.0f) ? 1 : 3, which named the landscape pair backwards
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

    if (!imuPresent || !getautoRotate())
    {
        pendingRotation = ROTATION_NONE;
        filterPrimed = false;
        return;
    }

    unsigned long now = millis();
    if (now - lastPoll < AUTO_ROTATE_POLL_MS)
        return;
    lastPoll = now;

    float gx, gy, gz;
    if (!imuReadGravity(&gx, &gy, &gz))
        return;

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
    if (currentScr != NULL && currentScr->isMsgBoxDisplayed())
        return;
    if (!isKeyboardHidden())
        return;

    log_i("Auto rotate: %u -> %u", getscreenRotation(), pendingRotation);
    setscreenRotation(pendingRotation);
    reinitializeScreens();
}

#endif /* AUTO_ROTATE_SUPPORTED */
