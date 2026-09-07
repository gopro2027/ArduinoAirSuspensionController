// QMI8658 driver. Deliberately knows nothing about screen rotation: the auto rotate state
// machine lives in auto_rotate.cpp and is only one possible consumer of this data (wake on
// movement is the expected next one). Keep board/display concerns out of this file.

#include "imu.h"

#if IMU_SUPPORTED == 1

#include <Wire.h>

// Some boards share Wire between the LVGL task and a PMIC poller pinned to the other core,
// and hand out a mutex to serialise it (see IMU_I2C_GUARDED in the board's device_lib_exports.h).
// Boards that only ever touch I2C from the LVGL task have no such mutex and need no locking.
// A missed lock just fails the read; every caller is expected to cope with that.
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

// ── QMI8658 ───────────────────────────────────────────────────────────────────────────────
// The 6-axis IMU fitted to the Waveshare panels that have one. Only the accelerometer is
// brought up, since nothing here needs rate data yet and leaving the gyro disabled keeps the
// part near its idle current. Enabling it later is a CTRL3 write plus CTRL7 bit 1.
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
        log_i("IMU: no QMI8658 on the bus");
        return;
    }

    imuWriteReg(QMI8658_REG_RESET, 0xB0);
    delay(20); // datasheet soft reset settling time

    imuWriteReg(QMI8658_REG_CTRL1, QMI8658_CTRL1_VALUE);
    imuWriteReg(QMI8658_REG_CTRL2, QMI8658_CTRL2_VALUE);
    imuWriteReg(QMI8658_REG_CTRL7, QMI8658_CTRL7_VALUE);
    delay(10); // let the first conversion land before anything reads

    imuPresent = true;
    log_i("IMU: QMI8658 found at 0x%02X", imuAddr);
}

bool imuAvailable()
{
    return imuPresent;
}

bool imuReadAccel(float *ax, float *ay, float *az)
{
    uint8_t b[6];
    if (!imuPresent || !imuReadRegs(QMI8658_REG_AX_L, b, sizeof(b)))
        return false;

    *ax = (int16_t)((uint16_t)b[0] | ((uint16_t)b[1] << 8)) / QMI8658_ACC_LSB_PER_G;
    *ay = (int16_t)((uint16_t)b[2] | ((uint16_t)b[3] << 8)) / QMI8658_ACC_LSB_PER_G;
    *az = (int16_t)((uint16_t)b[4] | ((uint16_t)b[5] << 8)) / QMI8658_ACC_LSB_PER_G;
    return true;
}

#endif /* IMU_SUPPORTED */
