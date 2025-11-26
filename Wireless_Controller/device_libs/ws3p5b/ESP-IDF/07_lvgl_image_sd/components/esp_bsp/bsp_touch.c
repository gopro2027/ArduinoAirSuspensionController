#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"

//#include "esp_lcd_axs15231b.h"
#include "esp_log.h"
#include "bsp_touch.h"
#include "bsp_i2c.h"

static uint16_t g_rotation = 0;
static uint16_t g_width = 0;
static uint16_t g_height = 0;

static i2c_master_dev_handle_t dev_handle;

touch_data_t g_touch_data;

void bsp_touch_read(void)
{
    uint8_t data_err_cnt = 0;
    uint8_t data[14] = {0}; /*1 Point:8;  2 Point: 14 */
    uint8_t read_cmd[11] = {0xb5, 0xab, 0xa5, 0x5a, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00};
    uint16_t temp_x = 0;
    uint16_t temp_y = 0;
    esp_err_t err = ESP_OK;
    if (bsp_i2c_lock(0))
    {
        // i2c_master_transmit(dev_handle, read_cmd, 11, pdMS_TO_TICKS(1000));
        // i2c_master_receive(dev_handle, data, 14, pdMS_TO_TICKS(1000));
        err = i2c_master_transmit_receive(dev_handle, read_cmd, 11, data, 14, pdMS_TO_TICKS(1000));
        bsp_i2c_unlock();
        if (err != ESP_OK)
        {
            return;
        }
        // printf("Received: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11], data[12], data[13]);
        if (data[1] == 0 || data[2] == 0 || data[3] < 2 || data[5] < 2 ) {
            return ;
        }
        
        if (data[0] == 0xff || data[1] > 2)
        {
            g_touch_data.touch_num = 0;
            return;
        }

        g_touch_data.touch_num = data[1];
        for (int i = 0; i < g_touch_data.touch_num; i++)
        {
            g_touch_data.coords[i].x = ((data[6 * i + 2] & 0x0F) << 8) | data[6 * i + 3];
            g_touch_data.coords[i].y = ((data[6 * i + 4] & 0x0F) << 8) | data[6 * i + 5];
        }
    }
}

bool bsp_touch_get_coordinates(touch_data_t *touch_data)
{
    if ((touch_data == NULL) || (g_touch_data.touch_num == 0))
        return false;

    for (int i = 0; i < g_touch_data.touch_num; i++)
    {
        switch (g_rotation)
        {
        case 1:
            touch_data->coords[i].y = g_height - 1 - g_touch_data.coords[i].x;
            touch_data->coords[i].x = g_touch_data.coords[i].y;
            break;
        case 2:
            touch_data->coords[i].x = g_width - 1 - g_touch_data.coords[i].x;
            touch_data->coords[i].y = g_height - 1 - g_touch_data.coords[i].y;
            break;
        case 3:
            touch_data->coords[i].y = g_touch_data.coords[i].x;
            touch_data->coords[i].x = g_width - 1 - g_touch_data.coords[i].y;
            break;
        default:
            touch_data->coords[i].x = g_touch_data.coords[i].x;
            touch_data->coords[i].y = g_touch_data.coords[i].y;
            break;
        }
    }
    return true;
}

void bsp_touch_init(i2c_master_bus_handle_t bus_handle, uint16_t width, uint16_t height, uint16_t rotation)
{
    g_rotation = rotation;
    g_width = width;
    g_height = height;
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = I2C_AXS15231B_ADDRESS,
        .scl_speed_hz = 400000,
        // .flags.disable_ack_check = true,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));
}

// void bsp_touch_init(esp_lcd_touch_handle_t *touch_handle, i2c_master_bus_handle_t bus_handle, uint16_t xmax, uint16_t ymax, uint16_t rotation)
// {
//     esp_lcd_panel_io_handle_t io_handle;
//     // static i2c_master_dev_handle_t dev_handle;
//     // i2c_device_config_t dev_cfg = {
//     //     .dev_addr_length = I2C_ADDR_BIT_LEN_7,
//     //     .device_address = ESP_LCD_TOUCH_IO_I2C_AXS5106_ADDRESS,
//     //     .scl_speed_hz = 400000,
//     // };
//     // ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

//     esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_AXS15231B_CONFIG();
//     tp_io_config.scl_speed_hz = 400000;
//     esp_lcd_touch_config_t tp_cfg = {};
//     tp_cfg.x_max = xmax < ymax ? xmax : ymax;
//     tp_cfg.y_max = xmax < ymax ? ymax : xmax;
//     ;
//     tp_cfg.rst_gpio_num = EXAMPLE_PIN_TP_RST;
//     tp_cfg.int_gpio_num = EXAMPLE_PIN_TP_INT;

//     if (90 == rotation)
//     {
//         tp_cfg.flags.swap_xy = 1;
//         tp_cfg.flags.mirror_x = 0;
//         tp_cfg.flags.mirror_y = 0;
//     }
//     else if (180 == rotation)
//     {
//         tp_cfg.flags.swap_xy = 0;
//         tp_cfg.flags.mirror_x = 0;
//         tp_cfg.flags.mirror_y = 1;
//     }
//     else if (270 == rotation)
//     {
//         tp_cfg.flags.swap_xy = 1;
//         tp_cfg.flags.mirror_x = 1;
//         tp_cfg.flags.mirror_y = 1;
//     }
//     else
//     {
//         tp_cfg.flags.swap_xy = 0;
//         tp_cfg.flags.mirror_x = 0;
//         tp_cfg.flags.mirror_y = 0;
//     }
//     ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(bus_handle, &tp_io_config, &io_handle));
//     ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_axs15231b(io_handle, &tp_cfg, touch_handle));
// }
