// IMU_QMI8658.cpp
#include "IMU_QMI8658.h"

static SensorQMI8658 qmi;
static bool s_imu_ok = false;
static bool s_in_wake_mode = false;

bool imu_init()
{
    Serial.println("[IMU] init...");

    // Initialize I2C bus if not already done
    Wire.begin(IMU_SDA_PIN, IMU_SCL_PIN);

    if (!qmi.begin(Wire, QMI8658_L_SLAVE_ADDRESS, IMU_SDA_PIN, IMU_SCL_PIN)) {
        Serial.println("[IMU] QMI8658 not found");
        s_imu_ok = false;
        return false;
    }

    // Basic self-test (optional)
    if (!qmi.selfTestAccel()) {
        Serial.println("[IMU] accel self-test FAILED");
    }
    if (!qmi.selfTestGyro()) {
        Serial.println("[IMU] gyro self-test FAILED");
    }

    // Configure for normal runtime use first (you can tune later)
    qmi.configAccelerometer(
        SensorQMI8658::ACC_RANGE_4G,
        SensorQMI8658::ACC_ODR_125Hz,     // lower ODR to save power
        SensorQMI8658::LPF_MODE_1
    );

    // For wake-on-motion we don't actually need the gyro,
    // so leave gyro disabled to save power.
    qmi.disableGyroscope();
    qmi.enableAccelerometer();

    // Hook up INT1 GPIO
    pinMode(IMU_INT_GPIO, INPUT_PULLUP);
    qmi.setPins(IMU_INT_GPIO);      // tells library where the INT is

    s_imu_ok = true;
    Serial.println("[IMU] QMI8658 initialized");
    return true;
}

void imu_enter_wake_mode()
{
    if (!s_imu_ok) return;

    if (!s_in_wake_mode) {
        Serial.println("[IMU] enter wake-on-motion mode");

        // Reconfigure accel for low-power / low ODR while sleeping
        qmi.configAccelerometer(
            SensorQMI8658::ACC_RANGE_4G,
            SensorQMI8658::ACC_ODR_LOWPOWER_21Hz,   // ~21 Hz
            SensorQMI8658::LPF_MODE_0
        );
        qmi.enableAccelerometer();
        qmi.disableGyroscope();

        // Enable data-ready interrupt on INT1.
        // This is a simple but effective wake-on-motion:
        // any new sample will pulse INT1 low.
        qmi.enableINT(SensorQMI8658::INTERRUPT_PIN_1, true);
        qmi.enableINT(SensorQMI8658::INTERRUPT_PIN_2, false);

        // Configure ESP32 EXT0 wake on IMU_INT_GPIO (active low)
        gpio_config_t io_conf = {};
        io_conf.pin_bit_mask = 1ULL << IMU_INT_GPIO;
        io_conf.mode         = GPIO_MODE_INPUT;
        io_conf.pull_up_en   = GPIO_PULLUP_ENABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.intr_type    = GPIO_INTR_DISABLE;
        gpio_config(&io_conf);

        esp_sleep_enable_ext0_wakeup((gpio_num_t)IMU_INT_GPIO, 0); // wake when INT1 goes LOW

        s_in_wake_mode = true;
    }
}

void imu_exit_wake_mode()
{
    if (!s_imu_ok) return;
    if (!s_in_wake_mode) return;

    Serial.println("[IMU] exit wake-on-motion mode");

    // Disable interrupts from IMU so it stops toggling INT1
    qmi.enableINT(SensorQMI8658::INTERRUPT_PIN_1, false);
    qmi.enableINT(SensorQMI8658::INTERRUPT_PIN_2, false);

    // Optional: switch back to normal runtime configuration
    qmi.configAccelerometer(
        SensorQMI8658::ACC_RANGE_4G,
        SensorQMI8658::ACC_ODR_125Hz,
        SensorQMI8658::LPF_MODE_1
    );
    qmi.enableAccelerometer();

    // Leave gyro still disabled unless you use it elsewhere
    qmi.disableGyroscope();

    s_in_wake_mode = false;
}

bool imu_data_ready()
{
    if (!s_imu_ok) return false;
    // Library’s getDataReady() checks INT + status and clears it
    return qmi.getDataReady();
}

void imu_read(IMUdata &acc, IMUdata &gyr)
{
    acc = {};
    gyr = {};
    if (!s_imu_ok) return;

    qmi.getAccelerometer(acc.x, acc.y, acc.z);
    qmi.getGyroscope(gyr.x, gyr.y, gyr.z);
}
