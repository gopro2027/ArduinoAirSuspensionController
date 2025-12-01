#include "esp_lcd_touch_axs15231b.h"
#include "waveshare_3p5/i2c_guard.h"

TwoWire *g_touch_i2c;       

uint16_t g_width; 
uint16_t g_height;
uint16_t g_rotation;
touch_data_t g_touch_data;

bool g_touch_int_flag = false;
static bool touch_i2c_write_read(uint8_t addr, uint8_t *wb, uint32_t wl,
                                 uint8_t *rb, uint32_t rl)
{
  if (!::i2c_lock(6)) return false;   // short timeout so touch “wins”

  g_touch_i2c->beginTransmission(addr);
  g_touch_i2c->write(wb, wl);
  if (g_touch_i2c->endTransmission() != 0) {
    ::i2c_unlock();
    return false;
  }
  g_touch_i2c->requestFrom(addr, rl);
  if (g_touch_i2c->available() != rl) {
    ::i2c_unlock();
    return false;
  }
  g_touch_i2c->readBytes(rb, rl);

  ::i2c_unlock();
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