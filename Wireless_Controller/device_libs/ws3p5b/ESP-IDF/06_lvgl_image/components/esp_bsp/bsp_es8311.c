#include "esp_idf_version.h"

#include "driver/i2s_std.h"
#include "driver/i2s_tdm.h"
#include "soc/soc_caps.h"

#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"

#include "driver/i2c_master.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_heap_caps.h"
#include "bsp_i2c.h"

#define USE_IDF_I2C_MASTER

#define I2S_MCK_PIN 44  // MCLK 引脚
#define I2S_BCK_PIN 13  // BCLK 引脚
#define I2S_LRCK_PIN 15 // LRCLK 引脚
#define I2S_DOUT_PIN 16 // 数据输出（录制）
#define I2S_DIN_PIN 14  // 数据输入（播放）

i2s_chan_handle_t tx_handle;
i2s_chan_handle_t rx_handle;
esp_codec_dev_handle_t output_dev;
esp_codec_dev_handle_t input_dev;

static esp_err_t es8311_i2s_init(void)
{
    esp_err_t esp_err;
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    i2s_std_config_t std_cfg = {};
    std_cfg.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;
    std_cfg.clk_cfg.sample_rate_hz = 16000;
    std_cfg.clk_cfg.clk_src = I2S_CLK_SRC_DEFAULT;

    std_cfg.slot_cfg.data_bit_width = 16;
    std_cfg.slot_cfg.slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO;
    std_cfg.slot_cfg.slot_mode = I2S_SLOT_MODE_STEREO;
    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_BOTH;
    std_cfg.slot_cfg.ws_width = 16;
    std_cfg.slot_cfg.ws_pol = false;
    std_cfg.slot_cfg.bit_shift = true;
    std_cfg.slot_cfg.left_align = true;
    std_cfg.slot_cfg.big_endian = false;
    std_cfg.slot_cfg.bit_order_lsb = false;

    std_cfg.gpio_cfg.mclk = (gpio_num_t)I2S_MCK_PIN;
    std_cfg.gpio_cfg.bclk = (gpio_num_t)I2S_BCK_PIN;
    std_cfg.gpio_cfg.ws = (gpio_num_t)I2S_LRCK_PIN;
    std_cfg.gpio_cfg.dout = (gpio_num_t)I2S_DOUT_PIN;
    std_cfg.gpio_cfg.din = (gpio_num_t)I2S_DIN_PIN;

    esp_err = i2s_new_channel(&chan_cfg, &tx_handle, &rx_handle);

    esp_err = i2s_channel_init_std_mode(tx_handle, &std_cfg);

    esp_err = i2s_channel_init_std_mode(rx_handle, &std_cfg);
    // For tx master using duplex mode
    i2s_channel_enable(tx_handle);
    i2s_channel_enable(rx_handle);

    return esp_err;
}

void bsp_es8311_init(i2c_master_bus_handle_t bus_handle)
{
    es8311_i2s_init();
    audio_codec_i2s_cfg_t i2s_cfg = {
        .rx_handle = rx_handle,
        .tx_handle = tx_handle,
    };
    const audio_codec_data_if_t *data_if = audio_codec_new_i2s_data(&i2s_cfg);

    static audio_codec_i2c_cfg_t i2c_cfg = {};
    i2c_cfg.addr = ES8311_CODEC_DEFAULT_ADDR;
    i2c_cfg.bus_handle = bus_handle;

    const audio_codec_ctrl_if_t *ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);

    const audio_codec_gpio_if_t *gpio_if = audio_codec_new_gpio();
    // New output codec interface
    es8311_codec_cfg_t es8311_cfg = {};
    es8311_cfg.codec_mode = ESP_CODEC_DEV_WORK_MODE_BOTH;
    es8311_cfg.ctrl_if = ctrl_if;
    es8311_cfg.gpio_if = gpio_if;
    es8311_cfg.pa_pin = GPIO_NUM_NC;
    es8311_cfg.use_mclk = true;
    es8311_cfg.hw_gain.pa_voltage = 5.0;
    es8311_cfg.hw_gain.codec_dac_voltage = 3.3;

    const audio_codec_if_t *codec_if = es8311_codec_new(&es8311_cfg);

    esp_codec_dev_cfg_t dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_OUT,
        .codec_if = codec_if,
        .data_if = data_if,
    };
    output_dev = esp_codec_dev_new(&dev_cfg);
    assert(output_dev != NULL);

    dev_cfg.dev_type = ESP_CODEC_DEV_TYPE_IN;
    input_dev = esp_codec_dev_new(&dev_cfg);
    assert(input_dev != NULL);
    esp_codec_set_disable_when_closed(output_dev, false);
    esp_codec_set_disable_when_closed(input_dev, false);

    esp_codec_dev_sample_info_t fs = {};
    fs.sample_rate = 48000;
    fs.channel = 1;
    fs.bits_per_sample = 16;
    fs.channel_mask = 0;
    fs.mclk_multiple = 0;

    esp_codec_dev_open(output_dev, &fs);
    esp_codec_dev_open(input_dev, &fs);
}

void bsp_es8311_recording(uint8_t *data, size_t limit_size)
{
    int err = 0;
    if (bsp_i2c_lock(0))
    {
        esp_codec_dev_set_in_gain(input_dev, 40.0);
        bsp_i2c_unlock();
    }
    err = esp_codec_dev_read(input_dev, data, limit_size);
    if (bsp_i2c_lock(0))
    {
        esp_codec_dev_set_in_gain(input_dev, 0.0);
        bsp_i2c_unlock();
    }

    if (err == ESP_CODEC_DEV_OK)
        printf("Read %d bytes\n", limit_size);
    else
        printf("Read error %d\n", err);
}

void bsp_es8311_playing(uint8_t *data, size_t limit_size)
{
    int err = 0;
    if (bsp_i2c_lock(0))
    {
        esp_codec_dev_set_out_vol(output_dev, 70.0);
        bsp_i2c_unlock();
    }
    err = esp_codec_dev_write(output_dev, data, limit_size);
    if (bsp_i2c_lock(0))
    {
        esp_codec_dev_set_out_vol(output_dev, 0.0);
        bsp_i2c_unlock();
    }
    if (err == ESP_CODEC_DEV_OK)
        printf("Write %d bytes\n", limit_size);
    else
        printf("Write error %d\n", err);
}

void bsp_es8311_test(void)
{
    int err = 0;
    // 2 Sec
    const int limit_size = 5 * 48000 * 1 * (16 >> 3);

    uint8_t *data = (uint8_t *)heap_caps_malloc(limit_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if (bsp_i2c_lock(0))
    {
        esp_codec_dev_set_in_gain(input_dev, 40.0);
        bsp_i2c_unlock();
    }

    err = esp_codec_dev_read(input_dev, data, limit_size);

    if (bsp_i2c_lock(0))
    {
        esp_codec_dev_set_in_gain(input_dev, 0.0);
        bsp_i2c_unlock();
    }

    if (err == ESP_CODEC_DEV_OK)
        printf("Read %d bytes\n", limit_size);
    else
        printf("Read error %d\n", err);

    if (bsp_i2c_lock(0))
    {
        esp_codec_dev_set_out_vol(output_dev, 70.0);
        bsp_i2c_unlock();
    }

    err = esp_codec_dev_write(output_dev, data, limit_size);

    if (bsp_i2c_lock(0))
    {
        esp_codec_dev_set_out_vol(output_dev, 0.0);
        bsp_i2c_unlock();
    }
    if (err == ESP_CODEC_DEV_OK)
        printf("Write %d bytes\n", limit_size);
    else
        printf("Write error %d\n", err);

    heap_caps_free(data);
}