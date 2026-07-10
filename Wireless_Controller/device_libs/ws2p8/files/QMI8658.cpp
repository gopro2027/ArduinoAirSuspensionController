#include "QMI8658.h"
#include "I2C_Driver.h"

// QMI8658 register map (subset)
#define QMI8658_REG_WHO_AM_I 0x00
#define QMI8658_REG_CTRL1    0x02  // serial interface / auto-increment
#define QMI8658_REG_CTRL2    0x03  // accel ODR + full scale
#define QMI8658_REG_CTRL3    0x04  // gyro ODR + full scale
#define QMI8658_REG_CTRL5    0x06  // low-pass filters
#define QMI8658_REG_CTRL7    0x08  // sensor enable
#define QMI8658_REG_AX_L     0x35  // accel/gyro output block start

#define QMI8658_WHO_AM_I_VAL 0x05

// CTRL2: accel +/-4g (aFS=1) @ 125Hz (aODR=6)
#define QMI8658_CTRL2_VAL ((1 << 4) | 0x06)
// CTRL3: gyro +/-512dps (gFS=5) @ 125Hz (gODR=6)
#define QMI8658_CTRL3_VAL ((5 << 4) | 0x06)

// Sensitivities matching the scales above.
static const float ACCEL_LSB_PER_G = 8192.0f;   // 32768 / 4g
static const float GYRO_LSB_PER_DPS = 64.0f;     // 32768 / 512dps

static uint8_t s_addr = 0;      // resolved 7-bit address, 0 = not found
static bool s_present = false;

// I2C_Read/I2C_Write return 0 on success (non-zero on failure).
static bool readReg(uint8_t reg, uint8_t *data, uint32_t len)
{
    return I2C_Read(s_addr, reg, data, len) == 0;
}

static bool writeReg(uint8_t reg, uint8_t value)
{
    return I2C_Write(s_addr, reg, &value, 1) == 0;
}

bool QMI8658_Init(void)
{
    s_present = false;
    s_addr = 0;

    const uint8_t candidates[2] = {QMI8658_ADDR_HIGH, QMI8658_ADDR_LOW};
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
        log_w("QMI8658 not found on I2C bus (WHO_AM_I=0x%02X)", whoami);
        return false;
    }

    log_i("QMI8658 found at 0x%02X", s_addr);

    // CTRL1: enable address auto-increment (bit6), keep I2C mode.
    if (!writeReg(QMI8658_REG_CTRL1, 0x40)) return false;
    // Configure accel + gyro scales/ODR.
    if (!writeReg(QMI8658_REG_CTRL2, QMI8658_CTRL2_VAL)) return false;
    if (!writeReg(QMI8658_REG_CTRL3, QMI8658_CTRL3_VAL)) return false;
    // CTRL5: no additional low-pass filtering.
    if (!writeReg(QMI8658_REG_CTRL5, 0x00)) return false;
    // CTRL7: enable accelerometer (bit0) + gyroscope (bit1).
    if (!writeReg(QMI8658_REG_CTRL7, 0x03)) return false;

    s_present = true;
    return true;
}

bool QMI8658_Present(void)
{
    return s_present;
}

bool QMI8658_ReadAccelGyro(float *ax, float *ay, float *az,
                           float *gx, float *gy, float *gz)
{
    if (!s_present) return false;

    // 6x int16 little-endian: AX,AY,AZ,GX,GY,GZ starting at 0x35.
    uint8_t buf[12];
    if (!readReg(QMI8658_REG_AX_L, buf, sizeof(buf))) return false;

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
