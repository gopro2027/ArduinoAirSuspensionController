#include "ST7701S.h"

#define SPI_WriteComm(cmd) ST7701S_WriteCommand(St7701S_handle, cmd)
#define SPI_WriteData(data) ST7701S_WriteData(St7701S_handle, data)
#define Delay(ms) vTaskDelay(ms / portTICK_PERIOD_MS)

uint8_t LCD_Backlight = 100;
static const char *LCD_TAG = "LCD";

void ioexpander_init(){};
void ioexpander_write_cmd(){};
void ioexpander_write_data(){};

/**
 * @brief Example Create an ST7701S object
 * @param SDA SDA pin
 * @param SCL SCL pin
 * @param CS  CS  pin
 * @param channel_select SPI channel selection
 * @param method_select  SPI_METHOD,IOEXPANDER_METHOD
 * @note
*/
ST7701S_handle ST7701S_newObject(int SDA, int SCL, int CS, char channel_select, char method_select)
{
    // if you use `malloc()`, please set 0 in the area to be assigned.
    ST7701S_handle st7701s_handle = heap_caps_calloc(1, sizeof(ST7701S), MALLOC_CAP_DEFAULT);
    st7701s_handle->method_select = method_select;
    
    if(method_select){
        st7701s_handle->spi_io_config_t.miso_io_num = -1;
        st7701s_handle->spi_io_config_t.mosi_io_num = SDA;
        st7701s_handle->spi_io_config_t.sclk_io_num = SCL;
        st7701s_handle->spi_io_config_t.quadwp_io_num = -1;
        st7701s_handle->spi_io_config_t.quadhd_io_num = -1;

        st7701s_handle->spi_io_config_t.max_transfer_sz = SOC_SPI_MAXIMUM_BUFFER_SIZE;

        ESP_ERROR_CHECK(spi_bus_initialize(channel_select, &(st7701s_handle->spi_io_config_t),SPI_DMA_CH_AUTO));

        st7701s_handle->st7701s_protocol_config_t.command_bits = 1;
        st7701s_handle->st7701s_protocol_config_t.address_bits = 8;
        st7701s_handle->st7701s_protocol_config_t.clock_speed_hz = 4000000;
        st7701s_handle->st7701s_protocol_config_t.mode = 0;
        st7701s_handle->st7701s_protocol_config_t.spics_io_num = CS;
        st7701s_handle->st7701s_protocol_config_t.queue_size = 1;

        ESP_ERROR_CHECK(spi_bus_add_device(channel_select, &(st7701s_handle->st7701s_protocol_config_t),
                                        &(st7701s_handle->spi_device)));
        
        return st7701s_handle;
    }else{
        ioexpander_init();
    }
    return NULL;
}

/**
 * @brief Screen initialization
 * @param St7701S_handle 
 * @param type 
 * @note
*/
void ST7701S_screen_init(ST7701S_handle St7701S_handle, unsigned char type)
{
    if (type == 1){
        // 2.8inch
        SPI_WriteComm(0xFF);SPI_WriteData(0x77);SPI_WriteData(0x01);SPI_WriteData(0x00);SPI_WriteData(0x00);SPI_WriteData(0x13);
        SPI_WriteComm(0xEF);SPI_WriteData(0x08);
        SPI_WriteComm(0xFF);SPI_WriteData(0x77);SPI_WriteData(0x01);SPI_WriteData(0x00);SPI_WriteData(0x00);SPI_WriteData(0x10);
        SPI_WriteComm(0xC0);SPI_WriteData(0x4F);SPI_WriteData(0x00);
        SPI_WriteComm(0xC1);SPI_WriteData(0x10);SPI_WriteData(0x02);
        SPI_WriteComm(0xC2);SPI_WriteData(0x07);SPI_WriteData(0x02);
        SPI_WriteComm(0xCC);SPI_WriteData(0x10);
        SPI_WriteComm(0xB0);SPI_WriteData(0x00);SPI_WriteData(0x10);SPI_WriteData(0x17);SPI_WriteData(0x0D);SPI_WriteData(0x11);SPI_WriteData(0x06);SPI_WriteData(0x05);SPI_WriteData(0x08);SPI_WriteData(0x07);SPI_WriteData(0x1F);SPI_WriteData(0x04);SPI_WriteData(0x11);SPI_WriteData(0x0E);SPI_WriteData(0x29);SPI_WriteData(0x30);SPI_WriteData(0x1F);
        SPI_WriteComm(0xB1);SPI_WriteData(0x00);SPI_WriteData(0x0D);SPI_WriteData(0x14);SPI_WriteData(0x0E);SPI_WriteData(0x11);SPI_WriteData(0x06);SPI_WriteData(0x04);SPI_WriteData(0x08);SPI_WriteData(0x08);SPI_WriteData(0x20);SPI_WriteData(0x05);SPI_WriteData(0x13);SPI_WriteData(0x13);SPI_WriteData(0x26);SPI_WriteData(0x30);SPI_WriteData(0x1F);
        SPI_WriteComm(0xFF);SPI_WriteData(0x77);SPI_WriteData(0x01);SPI_WriteData(0x00);SPI_WriteData(0x00);SPI_WriteData(0x11);
        SPI_WriteComm(0xB0);SPI_WriteData(0x65);
        SPI_WriteComm(0xB1);SPI_WriteData(0x71);
        SPI_WriteComm(0xB2);SPI_WriteData(0x82);//87
        SPI_WriteComm(0xB3);SPI_WriteData(0x80);
        SPI_WriteComm(0xB5);SPI_WriteData(0x42);//4D
        SPI_WriteComm(0xB7);SPI_WriteData(0x85);
        SPI_WriteComm(0xB8);SPI_WriteData(0x20);
        SPI_WriteComm(0xC0);SPI_WriteData(0x09);
        SPI_WriteComm(0xC1);SPI_WriteData(0x78);
        SPI_WriteComm(0xC2);SPI_WriteData(0x78);
        SPI_WriteComm(0xD0);SPI_WriteData(0x88);
        SPI_WriteComm(0xEE);SPI_WriteData(0x42);

        SPI_WriteComm(0xE0);SPI_WriteData(0x00);SPI_WriteData(0x00);SPI_WriteData(0x02);
        SPI_WriteComm(0xE1);SPI_WriteData(0x04);SPI_WriteData(0xA0);SPI_WriteData(0x06);SPI_WriteData(0xA0);SPI_WriteData(0x05);SPI_WriteData(0xA0);SPI_WriteData(0x07);SPI_WriteData(0xA0);SPI_WriteData(0x00);SPI_WriteData(0x44);SPI_WriteData(0x44);
        SPI_WriteComm(0xE2);SPI_WriteData(0x00);SPI_WriteData(0x00);SPI_WriteData(0x00);SPI_WriteData(0x00);SPI_WriteData(0x00);SPI_WriteData(0x00);SPI_WriteData(0x00);SPI_WriteData(0x00);SPI_WriteData(0x00);SPI_WriteData(0x00);SPI_WriteData(0x00);SPI_WriteData(0x00);
        SPI_WriteComm(0xE3);SPI_WriteData(0x00);SPI_WriteData(0x00);SPI_WriteData(0x22);SPI_WriteData(0x22);
        SPI_WriteComm(0xE4);SPI_WriteData(0x44);SPI_WriteData(0x44);
        SPI_WriteComm(0xE5);SPI_WriteData(0x0c);SPI_WriteData(0x90);SPI_WriteData(0xA0);SPI_WriteData(0xA0);SPI_WriteData(0x0E);SPI_WriteData(0x92);SPI_WriteData(0xA0);SPI_WriteData(0xA0);SPI_WriteData(0x08);SPI_WriteData(0x8C);SPI_WriteData(0xA0);SPI_WriteData(0xA0);SPI_WriteData(0x0A);SPI_WriteData(0x8E);SPI_WriteData(0xA0);SPI_WriteData(0xA0);
        SPI_WriteComm(0xE6);SPI_WriteData(0x00);SPI_WriteData(0x00);SPI_WriteData(0x22);SPI_WriteData(0x22);
        SPI_WriteComm(0xE7);SPI_WriteData(0x44);SPI_WriteData(0x44);
        SPI_WriteComm(0xE8);SPI_WriteData(0x0D);SPI_WriteData(0x91);SPI_WriteData(0xA0);SPI_WriteData(0xA0);SPI_WriteData(0x0F);SPI_WriteData(0x93);SPI_WriteData(0xA0);SPI_WriteData(0xA0);SPI_WriteData(0x09);SPI_WriteData(0x8D);SPI_WriteData(0xA0);SPI_WriteData(0xA0);SPI_WriteData(0x0B);SPI_WriteData(0x8F);SPI_WriteData(0xA0);SPI_WriteData(0xA0);
        //SPI_WriteComm(0xE9);SPI_WriteData( 2);SPI_WriteData(SPI_WriteComm(0x36);SPI_WriteData(0x00);
        SPI_WriteComm(0xEB);SPI_WriteData(0x00);SPI_WriteData(0x00);SPI_WriteData(0xE4);SPI_WriteData(0xE4);SPI_WriteData(0x44);SPI_WriteData(0x00);SPI_WriteData(0x40);
        SPI_WriteComm(0xED);SPI_WriteData(0xFF);SPI_WriteData(0xF5);SPI_WriteData(0x47);SPI_WriteData(0x6F);SPI_WriteData(0x0B);SPI_WriteData(0xA1);SPI_WriteData(0xAB);SPI_WriteData(0xFF);SPI_WriteData(0xFF);SPI_WriteData(0xBA);SPI_WriteData(0x1A);SPI_WriteData(0xB0);SPI_WriteData(0xF6);SPI_WriteData(0x74);SPI_WriteData(0x5F);SPI_WriteData(0xFF);
        SPI_WriteComm(0xEF);SPI_WriteData(0x08);SPI_WriteData(0x08);SPI_WriteData(0x08);SPI_WriteData(0x40);SPI_WriteData(0x3F);SPI_WriteData(0x64);
        SPI_WriteComm(0xFF);SPI_WriteData(0x77);SPI_WriteData(0x01);SPI_WriteData(0x00);SPI_WriteData(0x00);SPI_WriteData(0x00);
        SPI_WriteComm(0xFF);SPI_WriteData(0x77);SPI_WriteData(0x01);SPI_WriteData(0x00);SPI_WriteData(0x00);SPI_WriteData(0x13);
        SPI_WriteComm(0xE6);SPI_WriteData(0x16);SPI_WriteData(0x7C);
        SPI_WriteComm(0xE8);SPI_WriteData(0x00);SPI_WriteData( 0x0E);
        SPI_WriteComm(0xFF);SPI_WriteData(0x77);SPI_WriteData(0x01);SPI_WriteData(0x00);SPI_WriteData(0x00);SPI_WriteData(0x00);
        SPI_WriteComm(0x11);SPI_WriteData(0x00);
        Delay(200);
        SPI_WriteComm(0xFF);SPI_WriteData(0x77);SPI_WriteData(0x01);SPI_WriteData(0x00);SPI_WriteData(0x00);SPI_WriteData(0x13);
        SPI_WriteComm(0xE8);SPI_WriteData(0x00);SPI_WriteData( 0x0C);
        Delay(150);
        SPI_WriteComm(0xE8);SPI_WriteData(0x00);SPI_WriteData( 0x00);
        SPI_WriteComm(0xFF);SPI_WriteData(0x77);SPI_WriteData(0x01);SPI_WriteData(0x00);SPI_WriteData(0x00);SPI_WriteData(0x00);
        SPI_WriteComm(0x29);SPI_WriteData(0x00);
        SPI_WriteComm(0x35);SPI_WriteData(0x00);

        SPI_WriteComm(0x11);
        SPI_WriteData(0x00);	 //sleep out
        Delay(200);

        SPI_WriteComm(0x29);
        SPI_WriteData(0x00);	  //display on
        SPI_WriteComm(0x29);
        SPI_WriteData(0x00);	  //display on
        Delay(100);
    }
}

/**
 * @brief Example Delete the ST7701S object
 * @param St7701S_handle 
*/
void ST7701S_delObject(ST7701S_handle St7701S_handle)
{
    assert(St7701S_handle != NULL);
    free(St7701S_handle);
}

/**
 * @brief SPI write instruction
 * @param St7701S_handle 
 * @param cmd instruction
*/
void ST7701S_WriteCommand(ST7701S_handle St7701S_handle, uint8_t cmd)
{
    if(St7701S_handle->method_select){
        spi_transaction_t spi_tran = {
            .rxlength = 0,
            .length = 0,
            .cmd = 0,
            .addr = cmd,
        };
        spi_device_transmit(St7701S_handle->spi_device, &spi_tran);
    }else{
        ioexpander_write_cmd();
    }
}

/**
 * @brief SPI write data
 * @param St7701S_handle
 * @param data 
*/
void ST7701S_WriteData(ST7701S_handle St7701S_handle, uint8_t data)
{
    if(St7701S_handle->method_select){
        spi_transaction_t spi_tran = {
            .rxlength = 0,
            .length = 0,
            .cmd = 1,
            .addr = data,
        };
        spi_device_transmit(St7701S_handle->spi_device, &spi_tran);
    }else{
        ioexpander_write_data();
    }
}


esp_err_t ST7701S_reset(void)
{
    Set_EXIO(TCA9554_EXIO1,false);
    vTaskDelay(pdMS_TO_TICKS(10));
    Set_EXIO(TCA9554_EXIO1,true);
    vTaskDelay(pdMS_TO_TICKS(10));
    return ESP_OK;
}

esp_err_t ST7701S_CS_EN(void)
{
    Set_EXIO(TCA9554_EXIO3,false);
    vTaskDelay(pdMS_TO_TICKS(10));
    return ESP_OK;
}
esp_err_t ST7701S_CS_Dis(void)
{
    Set_EXIO(TCA9554_EXIO3,true);
    vTaskDelay(pdMS_TO_TICKS(10));
    return ESP_OK;
}

#if CONFIG_EXAMPLE_AVOID_TEAR_EFFECT_WITH_SEM
SemaphoreHandle_t sem_vsync_end;
SemaphoreHandle_t sem_gui_ready;
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static bool example_on_vsync_event(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *event_data, void *user_data)
{
    BaseType_t high_task_awoken = pdFALSE;
#if CONFIG_EXAMPLE_AVOID_TEAR_EFFECT_WITH_SEM
    if (xSemaphoreTakeFromISR(sem_gui_ready, &high_task_awoken) == pdTRUE) {
        xSemaphoreGiveFromISR(sem_vsync_end, &high_task_awoken);
    }
#endif
    return high_task_awoken == pdTRUE;
}

esp_lcd_panel_handle_t panel_handle = NULL;
void LCD_Init(void)
{
    /********************* LCD *********************/
    ST7701S_reset();
    ST7701S_CS_EN();
    vTaskDelay(pdMS_TO_TICKS(100));
    ST7701S_handle st7701s = ST7701S_newObject(LCD_MOSI, LCD_SCLK, LCD_CS, SPI2_HOST, SPI_METHOD);
    
    ST7701S_screen_init(st7701s, 1);
    #if CONFIG_EXAMPLE_AVOID_TEAR_EFFECT_WITH_SEM
        ESP_LOGI(LCD_TAG, "Create semaphores");
        sem_vsync_end = xSemaphoreCreateBinary();
        assert(sem_vsync_end);
        sem_gui_ready = xSemaphoreCreateBinary();
        assert(sem_gui_ready);
    #endif

    /********************* RGB LCD panel driver *********************/
    ESP_LOGI(LCD_TAG, "Install RGB LCD panel driver");
    esp_lcd_rgb_panel_config_t panel_config = {
        .data_width = 16, // RGB565 in parallel mode, thus 16bit in width
        .psram_trans_align = 64,
        .num_fbs = EXAMPLE_LCD_NUM_FB,
#if CONFIG_EXAMPLE_USE_BOUNCE_BUFFER
        .bounce_buffer_size_px = 10 * EXAMPLE_LCD_H_RES,
#endif
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .disp_gpio_num = EXAMPLE_PIN_NUM_DISP_EN,
        .pclk_gpio_num = EXAMPLE_PIN_NUM_PCLK,
        .vsync_gpio_num = EXAMPLE_PIN_NUM_VSYNC,
        .hsync_gpio_num = EXAMPLE_PIN_NUM_HSYNC,
        .de_gpio_num = EXAMPLE_PIN_NUM_DE,
        .data_gpio_nums = {
            EXAMPLE_PIN_NUM_DATA0,
            EXAMPLE_PIN_NUM_DATA1,
            EXAMPLE_PIN_NUM_DATA2,
            EXAMPLE_PIN_NUM_DATA3,
            EXAMPLE_PIN_NUM_DATA4,
            EXAMPLE_PIN_NUM_DATA5,
            EXAMPLE_PIN_NUM_DATA6,
            EXAMPLE_PIN_NUM_DATA7,
            EXAMPLE_PIN_NUM_DATA8,
            EXAMPLE_PIN_NUM_DATA9,
            EXAMPLE_PIN_NUM_DATA10,
            EXAMPLE_PIN_NUM_DATA11,
            EXAMPLE_PIN_NUM_DATA12,
            EXAMPLE_PIN_NUM_DATA13,
            EXAMPLE_PIN_NUM_DATA14,
            EXAMPLE_PIN_NUM_DATA15,
        },
        .timings = {
            .pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
            .h_res = EXAMPLE_LCD_H_RES,
            .v_res = EXAMPLE_LCD_V_RES, 
            .hsync_back_porch = 10,
            .hsync_front_porch = 50,
            .hsync_pulse_width = 8,
            .vsync_back_porch = 18,
            .vsync_front_porch = 8,
            .vsync_pulse_width = 2,
            .flags.pclk_active_neg = false,
        },
        .flags.fb_in_psram = true, // allocate frame buffer in PSRAM
    };
    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &panel_handle));

    ESP_LOGI(LCD_TAG, "Register event callbacks");
    esp_lcd_rgb_panel_event_callbacks_t cbs = {
        .on_vsync = example_on_vsync_event,
    };
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(panel_handle, &cbs, &disp_drv));

    ESP_LOGI(LCD_TAG, "Initialize RGB LCD panel");
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ST7701S_CS_Dis();
    Backlight_Init();
}

/********************* BackLight *********************/
static void example_ledc_init(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 4 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LEDC_OUTPUT_IO,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}
void Backlight_Init(void)
{
    example_ledc_init();
    Set_Backlight(LCD_Backlight);
}

void Set_Backlight(uint8_t Light)
{
  if(Light > Backlight_MAX || Light < 0)
    printf("Set Backlight parameters in the range of 0 to 100 \r\n");
  else{
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, Light*(8192/100)));    // Set duty
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));                 // Update duty to apply the new value
  }
}