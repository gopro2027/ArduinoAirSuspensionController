/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include "GT911.h"

static const char *TAG = "GT911";

/* GT911 registers */
#define ESP_LCD_TOUCH_GT911_READ_XY_REG (0x814E)
#define ESP_LCD_TOUCH_GT911_CONFIG_REG  (0x8047)
#define ESP_LCD_TOUCH_GT911_PRODUCT_ID_REG (0x8140)


static esp_err_t esp_lcd_touch_gt911_read_data(esp_lcd_touch_handle_t tp);
static bool esp_lcd_touch_gt911_get_xy(esp_lcd_touch_handle_t tp, uint16_t *x, uint16_t *y, uint16_t *strength, uint8_t *point_num, uint8_t max_point_num);
static esp_err_t esp_lcd_touch_gt911_del(esp_lcd_touch_handle_t tp);

/* I2C read/write */
static esp_err_t touch_gt911_i2c_read(esp_lcd_touch_handle_t tp, uint16_t reg, uint8_t *data, uint8_t len);
static esp_err_t touch_gt911_i2c_write(esp_lcd_touch_handle_t tp, uint16_t reg, uint8_t data, uint8_t len);

/* GT911 reset */
static esp_err_t touch_gt911_reset(esp_lcd_touch_handle_t tp);
/* Read status and config register */
static void touch_gt911_read_cfg(esp_lcd_touch_handle_t tp);

esp_err_t esp_lcd_touch_new_i2c_gt911(const esp_lcd_panel_io_handle_t io, const esp_lcd_touch_config_t *config, esp_lcd_touch_handle_t *out_touch)
{
    esp_err_t ret = ESP_OK;

    assert(io != NULL);
    assert(config != NULL);
    assert(out_touch != NULL);

    /* Prepare main structure */
    esp_lcd_touch_handle_t esp_lcd_touch_gt911 = heap_caps_calloc(1, sizeof(esp_lcd_touch_t), MALLOC_CAP_DEFAULT);
    ESP_GOTO_ON_FALSE(esp_lcd_touch_gt911, ESP_ERR_NO_MEM, err, TAG, "no mem for GT911 controller");

    /* Communication interface */
    esp_lcd_touch_gt911->io = io;

    /* Only supported callbacks are set */
    esp_lcd_touch_gt911->read_data = esp_lcd_touch_gt911_read_data;
    esp_lcd_touch_gt911->get_xy = esp_lcd_touch_gt911_get_xy;
    esp_lcd_touch_gt911->del = esp_lcd_touch_gt911_del;

    /* Mutex */
    esp_lcd_touch_gt911->data.lock.owner = portMUX_FREE_VAL;

    /* Save config */
    memcpy(&esp_lcd_touch_gt911->config, config, sizeof(esp_lcd_touch_config_t));

    /* Prepare pin for touch interrupt */
    if (esp_lcd_touch_gt911->config.int_gpio_num != GPIO_NUM_NC) {
        const gpio_config_t int_gpio_config = {
            .mode = GPIO_MODE_INPUT,
            .intr_type = GPIO_INTR_NEGEDGE,
            .pin_bit_mask = BIT64(esp_lcd_touch_gt911->config.int_gpio_num)
        };
        ret = gpio_config(&int_gpio_config);
        ESP_GOTO_ON_ERROR(ret, err, TAG, "GPIO config failed");
    }


    /* Reset controller */
    ret = touch_gt911_reset(esp_lcd_touch_gt911);
    ESP_GOTO_ON_ERROR(ret, err, TAG, "GT911 reset failed");

    /* Read status and config info */
    touch_gt911_read_cfg(esp_lcd_touch_gt911);

    *out_touch = esp_lcd_touch_gt911;
    return ESP_OK;
err:
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error (0x%x)! Touch controller GT911 initialization failed!", ret);
        if (esp_lcd_touch_gt911) {
            esp_lcd_touch_gt911_del(esp_lcd_touch_gt911);
        }
    }


    return ret;
}
static esp_err_t esp_lcd_touch_gt911_read_data(esp_lcd_touch_handle_t tp)
{
    esp_err_t err;
    uint8_t buf[41];
    uint8_t touch_cnt = 0;
    uint8_t clear = 0;
    size_t i = 0;

    assert(tp != NULL);

    err = touch_gt911_i2c_read(tp, ESP_LCD_TOUCH_GT911_READ_XY_REG, buf, 1);
    ESP_RETURN_ON_ERROR(err, TAG, "I2C read error!");

    /* Any touch data? */
    if ((buf[0] & 0x80) == 0x00) {
        touch_gt911_i2c_write(tp, ESP_LCD_TOUCH_GT911_READ_XY_REG, clear, 1);
    } else {
        /* Count of touched points */
        touch_cnt = buf[0] & 0x0f;
        if (touch_cnt > 5 || touch_cnt == 0) {
            touch_gt911_i2c_write(tp, ESP_LCD_TOUCH_GT911_READ_XY_REG, clear, 1);
            return ESP_OK;
        }

        /* Read all points */
        err = touch_gt911_i2c_read(tp, ESP_LCD_TOUCH_GT911_READ_XY_REG + 1, &buf[1], touch_cnt * 8);
        ESP_RETURN_ON_ERROR(err, TAG, "I2C read error!");

        /* Clear all */
        err = touch_gt911_i2c_write(tp, ESP_LCD_TOUCH_GT911_READ_XY_REG, clear, 1);
        ESP_RETURN_ON_ERROR(err, TAG, "I2C read error!");

        taskENTER_CRITICAL(&tp->data.lock);

        /* Number of touched points */
        touch_cnt = (touch_cnt > CONFIG_ESP_LCD_TOUCH_MAX_POINTS ? CONFIG_ESP_LCD_TOUCH_MAX_POINTS : touch_cnt);
        tp->data.points = touch_cnt;

        /* Fill all coordinates */
        for (i = 0; i < touch_cnt; i++) {
            tp->data.coords[i].x = ((uint16_t)buf[(i * 8) + 3] << 8) + buf[(i * 8) + 2];
            tp->data.coords[i].y = (((uint16_t)buf[(i * 8) + 5] << 8) + buf[(i * 8) + 4]);
            tp->data.coords[i].strength = (((uint16_t)buf[(i * 8) + 7] << 8) + buf[(i * 8) + 6]);
        }

        taskEXIT_CRITICAL(&tp->data.lock);
    }

    return ESP_OK;
}

static bool esp_lcd_touch_gt911_get_xy(esp_lcd_touch_handle_t tp, uint16_t *x, uint16_t *y, uint16_t *strength, uint8_t *point_num, uint8_t max_point_num)
{
    assert(tp != NULL);
    assert(x != NULL);
    assert(y != NULL);
    assert(point_num != NULL);
    assert(max_point_num > 0);

    taskENTER_CRITICAL(&tp->data.lock);

    /* Count of points */
    *point_num = (tp->data.points > max_point_num ? max_point_num : tp->data.points);

    for (size_t i = 0; i < *point_num; i++) {
        x[i] = tp->data.coords[i].x;
        y[i] = tp->data.coords[i].y;

        if (strength) {
            strength[i] = tp->data.coords[i].strength;
        }
    }

    /* Invalidate */
    tp->data.points = 0;

    taskEXIT_CRITICAL(&tp->data.lock);

    return (*point_num > 0);
}

static esp_err_t esp_lcd_touch_gt911_del(esp_lcd_touch_handle_t tp)
{
    assert(tp != NULL);

    /* Reset GPIO pin settings */
    if (tp->config.int_gpio_num != GPIO_NUM_NC) {
        gpio_reset_pin(tp->config.int_gpio_num);
    }

    /* Reset GPIO pin settings */
    if (tp->config.rst_gpio_num != GPIO_NUM_NC) {
        gpio_reset_pin(tp->config.rst_gpio_num);
    }

    free(tp);

    return ESP_OK;
}

/* Reset controller */
static esp_err_t touch_gt911_reset(esp_lcd_touch_handle_t tp)
{
    assert(tp != NULL);

    gpio_set_direction(I2C_Touch_INT_IO, GPIO_MODE_OUTPUT);
    gpio_set_level(I2C_Touch_INT_IO, false);
    vTaskDelay(pdMS_TO_TICKS(150));

    Set_EXIO(TCA9554_EXIO2,false);
    vTaskDelay(pdMS_TO_TICKS(150));
    Set_EXIO(TCA9554_EXIO2,true);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(I2C_Touch_INT_IO, true);
    gpio_set_direction(I2C_Touch_INT_IO, GPIO_MODE_INPUT);
                       

    return ESP_OK;
}

static void touch_gt911_read_cfg(esp_lcd_touch_handle_t tp)
{
    uint8_t buf[4];

    assert(tp != NULL);

    touch_gt911_i2c_read(tp, ESP_LCD_TOUCH_GT911_PRODUCT_ID_REG, (uint8_t *)&buf[0], 3);
    touch_gt911_i2c_read(tp, ESP_LCD_TOUCH_GT911_CONFIG_REG, (uint8_t *)&buf[3], 1);

    printf("TouchPad_ID:0x%02x,0x%02x,0x%02x\r\n", buf[0], buf[1], buf[2]);
    printf("TouchPad_Config_Version:%d\r\n", buf[3]);

    touch_gt911_i2c_read(tp, 0x804D, (uint8_t *)&buf[3], 1);
    printf("TouchPad_INT:%d\r\n", (buf[3]&0x03));

}

static esp_err_t touch_gt911_i2c_read(esp_lcd_touch_handle_t tp, uint16_t reg, uint8_t *data, uint8_t len)
{
    assert(tp != NULL);
    assert(data != NULL);

    /* Read data */
    return esp_lcd_panel_io_rx_param(tp->io, reg, data, len);
}

static esp_err_t touch_gt911_i2c_write(esp_lcd_touch_handle_t tp, uint16_t reg, uint8_t data, uint8_t len)
{
    assert(tp != NULL);

    return esp_lcd_panel_io_tx_param(tp->io, reg, (uint8_t[]){data}, len);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
esp_lcd_touch_handle_t tp = NULL;
void Touch_Init(void)
{
    
/********************* Touch *********************/
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    ESP_LOGI(TAG, "Initialize touch IO (I2C)");
    /* Touch IO handle */
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)I2C_MASTER_NUM, &tp_io_config, &tp_io_handle));
    esp_lcd_touch_config_t tp_cfg = {
        .x_max = EXAMPLE_LCD_V_RES,
        .y_max = EXAMPLE_LCD_H_RES,
        .rst_gpio_num = I2C_Touch_RST_IO,
        .int_gpio_num = I2C_Touch_INT_IO,
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };
    /* Initialize touch */
    ESP_LOGI(TAG, "Initialize touch controller GT911");
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &tp));
    
}