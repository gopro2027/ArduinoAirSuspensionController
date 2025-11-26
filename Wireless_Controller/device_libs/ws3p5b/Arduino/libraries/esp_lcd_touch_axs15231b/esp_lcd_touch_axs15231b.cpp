#include "esp_lcd_touch_axs15231b.h"
TwoWire *g_touch_i2c;       

uint16_t g_width; 
uint16_t g_height;
uint16_t g_rotation;
touch_data_t g_touch_data;

bool g_touch_int_flag = false;
static bool touch_i2c_write_read(uint8_t driver_addr, uint8_t *write_buf, uint32_t write_len, uint8_t *read_buf, uint32_t read_len)
{
    g_touch_i2c->beginTransmission(driver_addr);
    g_touch_i2c->write(write_buf, write_len);
    if (g_touch_i2c->endTransmission() != 0) {
        Serial.println("The I2C write fails. - I2C Read\r\n");
        return false;
    }

    g_touch_i2c->requestFrom(driver_addr, read_len);
    if (g_touch_i2c->available() != read_len) {
        Serial.println("The I2C read fails. - I2C Read\r\n");
        return false;
    }
    g_touch_i2c->readBytes(read_buf, read_len);
    return true;
}


void bsp_touch_init(TwoWire *touch_i2c,int tp_rst, uint16_t rotation, uint16_t width, uint16_t height)
{
    g_touch_i2c = touch_i2c;
    g_width = width;
    g_height = height;
    g_rotation = rotation;

    if (tp_rst != -1){
        pinMode(tp_rst, OUTPUT);
        digitalWrite(tp_rst, LOW);
        delay(200);
        digitalWrite(tp_rst, HIGH);
        delay(300);
    }
}

void bsp_touch_read(void)
{
    uint8_t data[14] = {0};
    uint8_t cmd[11] = {0xb5, 0xab, 0xa5, 0x5a, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00};

    touch_i2c_write_read(AXS5106L_ADDR, cmd, 11, data, 14);

    if (data[1] == 0 || data[2] == 0 || data[3] < 2 || data[5] < 2 ) {
        return ;
    }

    if (data[0] == 0xff || data[1] > 2) {
        g_touch_data.touch_num = 0;
        return;
    }

    g_touch_data.touch_num = data[1];

    for (uint8_t i = 0; i < g_touch_data.touch_num; i++){
        g_touch_data.coords[i].x = ((data[6 * i + 2] & 0x0F) << 8) | data[6 * i + 3];
        g_touch_data.coords[i].y = ((data[6 * i + 4] & 0x0F) << 8) | data[6 * i + 5];
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