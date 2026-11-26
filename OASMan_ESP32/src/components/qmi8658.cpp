#include "qmi8658.h"
#include <Wire.h>
#include "input_type.h"

#define QMI8658_REG_WHO_AM_I 0x00
#define QMI8658_REG_REVISION 0x01
#define QMI8658_REG_CTRL1 0x02 // serial interface / auto-increment
#define QMI8658_REG_CTRL2 0x03 // accel ODR + full scale
#define QMI8658_REG_CTRL3 0x04 // gyro ODR + full scale
#define QMI8658_REG_CTRL5 0x06 // low-pass filters
#define QMI8658_REG_CTRL7 0x08 // sensor enable
#define QMI8658_REG_RESET 0x60 // soft reset command register
#define QMI8658_REG_AX_L 0x35  // accel/gyro output block start

#define QMI8658_RESET_CMD 0xB0
#define QMI8658_WHO_AM_I_VAL 0x05

#define QMI8658_CTRL2_VAL ((1 << 4) | 0x06) // accel +/-4g @ 125Hz
#define QMI8658_CTRL3_VAL ((5 << 4) | 0x06) // gyro +/-512dps @ 125Hz

static const float ACCEL_LSB_PER_G = 8192.0f; // 32768 / 4g
static const float GYRO_LSB_PER_DPS = 64.0f;  // 32768 / 512dps

static uint8_t s_addr = 0; // resolved 7-bit address, 0 = not found
static bool s_present = false;

static bool readReg(uint8_t reg, uint8_t *data, uint8_t len)
{
    i2cBusLock();
    Wire.beginTransmission(s_addr);
    Wire.write(reg);
    bool ok = Wire.endTransmission(false) == 0 && Wire.requestFrom(s_addr, len) == len;
    for (uint8_t i = 0; ok && i < len; i++)
    {
        data[i] = Wire.read();
    }
    i2cBusUnlock();
    return ok;
}

static bool writeReg(uint8_t reg, uint8_t value)
{
    i2cBusLock();
    Wire.beginTransmission(s_addr);
    Wire.write(reg);
    Wire.write(value);
    bool ok = Wire.endTransmission() == 0;
    i2cBusUnlock();
    return ok;
}

bool QMI8658_Init()
{
    s_present = false;
    s_addr = 0;

    const uint8_t candidates[2] = {QMI8658_ADDR_SA0_LOW, QMI8658_ADDR_SA0_HIGH};
    uint8_t whoami = 0;
    for (uint8_t i = 0; i < 2; i++)
    {
        s_addr = candidates[i];
        if (readReg(QMI8658_REG_WHO_AM_I, &whoami, 1) && whoami == QMI8658_WHO_AM_I_VAL)
        {
            break;
        }
        s_addr = 0;
    }

    if (s_addr == 0)
    {
        Serial.println(F("No QMI8658 IMU found, motion detection disabled"));
        return false;
    }

    uint8_t revision = 0;
    readReg(QMI8658_REG_REVISION, &revision, 1);
    log_i("QMI8658 found at 0x%02X, revision 0x%02X", s_addr, revision);

    writeReg(QMI8658_REG_RESET, QMI8658_RESET_CMD); // soft reset to a known state
    delay(20);

    if (!writeReg(QMI8658_REG_CTRL1, 0x40)) // address auto-increment, little-endian
        return false;
    if (!writeReg(QMI8658_REG_CTRL2, QMI8658_CTRL2_VAL))
        return false;
    if (!writeReg(QMI8658_REG_CTRL3, QMI8658_CTRL3_VAL))
        return false;
    if (!writeReg(QMI8658_REG_CTRL5, 0x77)) // accel + gyro low-pass filters, ~17Hz at 125Hz ODR
        return false;
    if (!writeReg(QMI8658_REG_CTRL7, 0x03)) // enable accel + gyro
        return false;

    s_present = true;
    return true;
}

bool QMI8658_Present()
{
    return s_present;
}

bool QMI8658_ReadAccelGyro(float *ax, float *ay, float *az, float *gx, float *gy, float *gz)
{
    if (!s_present)
        return false;

    uint8_t buf[12]; // AX,AY,AZ,GX,GY,GZ as little-endian int16
    if (!readReg(QMI8658_REG_AX_L, buf, sizeof(buf)))
        return false;

    int16_t raw[6];
    for (int i = 0; i < 6; i++)
    {
        raw[i] = (int16_t)((uint16_t)buf[i * 2] | ((uint16_t)buf[i * 2 + 1] << 8));
    }

    if (ax) *ax = raw[0] / ACCEL_LSB_PER_G;
    if (ay) *ay = raw[1] / ACCEL_LSB_PER_G;
    if (az) *az = raw[2] / ACCEL_LSB_PER_G;
    if (gx) *gx = raw[3] / GYRO_LSB_PER_DPS;
    if (gy) *gy = raw[4] / GYRO_LSB_PER_DPS;
    if (gz) *gz = raw[5] / GYRO_LSB_PER_DPS;

    return true;
}
