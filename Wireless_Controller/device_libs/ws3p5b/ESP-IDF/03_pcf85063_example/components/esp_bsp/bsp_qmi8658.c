#include "bsp_qmi8658.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>
#include <string.h>

#include "esp_log.h"

#include "bsp_i2c.h"

static const char *TAG = "bsp_qmi8658";
#define ACC_SENSITIVITY (4 / 32768.0f) // 每LSB对应的加速度（单位为g）

static i2c_master_dev_handle_t dev_handle;

// 读取QMI8658寄存器的值
static esp_err_t bsp_qmi8658_reg_read(uint8_t reg_addr, uint8_t *data, size_t len)
{
    esp_err_t ret = ESP_FAIL;
    // return bsp_i2c_reg8_read(QMI8658_SENSOR_ADDR, reg_addr, data, len);

    if (bsp_i2c_lock(0))
    {
        ret = i2c_master_transmit_receive(dev_handle, &reg_addr, 1, data, len, pdMS_TO_TICKS(1000));
        bsp_i2c_unlock();
    }
    return ret;
}

// 给QMI8658的寄存器写值
static esp_err_t bsp_qmi8658_reg_write_byte(uint8_t reg_addr, uint8_t *data, size_t len)
{
    esp_err_t ret = ESP_FAIL;
    uint8_t buf[len + 1];
    buf[0] = reg_addr;
    memcpy(buf + 1, data, len);

    if (bsp_i2c_lock(0))
    {
        ret = i2c_master_transmit(dev_handle, buf, len + 1, pdMS_TO_TICKS(1000));
        bsp_i2c_unlock();
    }
    return ret;
}

bool bsp_qmi8658_read_data(qmi8658_data_t *data)
{
    uint8_t status;
    float mask;
    uint16_t buf[6];
    if (bsp_qmi8658_reg_read(QMI8658_STATUS0, &status, 1) != ESP_OK) // 读状态寄存器
    {
        // ESP_LOGI(TAG, "QMI8658 read status fail!");
        return false;
    }
    if (status & 0x03)
    {
        if (bsp_qmi8658_reg_read(QMI8658_AX_L, (uint8_t *)buf, 12) != ESP_OK) // 读加速度和陀螺仪值
        {
            // ESP_LOGI(TAG, "QMI8658 read data fail!");
            return false;
        }
        data->acc_x = buf[0];
        data->acc_y = buf[1];
        data->acc_z = buf[2];
        data->gyr_x = buf[3];
        data->gyr_y = buf[4];
        data->gyr_z = buf[5];
        // ESP_LOGI(TAG, "QMI8658 read data success!");

        mask = (float)data->acc_x / sqrt(((float)data->acc_y * (float)data->acc_y + (float)data->acc_z * (float)data->acc_z));
        data->AngleX = atan(mask) * 57.29578f; // 180/π=57.29578
        mask = (float)data->acc_y / sqrt(((float)data->acc_x * (float)data->acc_x + (float)data->acc_z * (float)data->acc_z));
        data->AngleY = atan(mask) * 57.29578f; // 180/π=57.29578
        mask = sqrt(((float)data->acc_x * (float)data->acc_x + (float)data->acc_y * (float)data->acc_y)) / (float)data->acc_z;
        data->AngleZ = atan(mask) * 57.29578f; // 180/π=57.29578
        return true;
    }
    return false;
}

void bsp_qmi8658_init(i2c_master_bus_handle_t bus_handle)
{
    uint8_t id = 0;
    ESP_LOGI(TAG, "QMI8658 Initialize");
    vTaskDelay(pdMS_TO_TICKS(100));
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = QMI8658_SENSOR_ADDR,
        .scl_speed_hz = 400000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

    ESP_ERROR_CHECK(bsp_qmi8658_reg_read(QMI8658_WHO_AM_I, &id, 1));

    if (0x05 != id)
    {
        // ESP_LOGI(TAG, "QMI8658 not found");
        return;
    }
    ESP_LOGI(TAG, "Find QMI8658");
    bsp_qmi8658_reg_write_byte(QMI8658_RESET, (uint8_t[]){0xb0}, 1); // 复位
    vTaskDelay(pdMS_TO_TICKS(10));                                   // 延时10ms
    bsp_qmi8658_reg_write_byte(QMI8658_CTRL1, (uint8_t[]){0x40}, 1); // CTRL1 设置地址自动增加
    bsp_qmi8658_reg_write_byte(QMI8658_CTRL7, (uint8_t[]){0x03}, 1); // CTRL7 允许加速度和陀螺仪
    bsp_qmi8658_reg_write_byte(QMI8658_CTRL2, (uint8_t[]){0x95}, 1); // CTRL2 设置ACC 4g 250Hz
    bsp_qmi8658_reg_write_byte(QMI8658_CTRL3, (uint8_t[]){0xd5}, 1); // CTRL3 设置GRY 512dps 250Hz
}

static void qmi8658_test_task(void *arg)
{
    qmi8658_data_t data;
    while (1)
    {
        if (bsp_qmi8658_read_data(&data))
        {
            // printf("Acc: %.2f %.2f %.2f-----Gyr: %04d %04d %04d\n", data.acc_x * ACC_SENSITIVITY * 9.8f, data.acc_y * ACC_SENSITIVITY * 9.8f, data.acc_z * ACC_SENSITIVITY * 9.8f, data.gyr_x, data.gyr_y, data.gyr_z);
            // printf("-------------------------------------------------------------------------\n");
            printf("Angle: %.2f %.2f %.2f\n", data.AngleX, data.AngleY, data.AngleZ);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void bsp_qmi8658_test(void)
{
    xTaskCreate(qmi8658_test_task, "qmi8658_test", 4096, NULL, tskIDLE_PRIORITY + 1, NULL);
}