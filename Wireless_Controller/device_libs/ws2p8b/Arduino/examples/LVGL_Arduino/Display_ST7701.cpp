#include "Display_ST7701.h"  
      
spi_device_handle_t SPI_handle = NULL;     
esp_lcd_panel_handle_t panel_handle = NULL;    
uint8_t LCD_Backlight = 100;

void ST7701_WriteCommand(uint8_t cmd)
{
  spi_transaction_t spi_tran = {
    .cmd = 0,
    .addr = cmd,
    .length = 0,
    .rxlength = 0,
  };
  spi_device_transmit(SPI_handle, &spi_tran);
}
void ST7701_WriteData(uint8_t data)
{
  spi_transaction_t spi_tran = {
    .cmd = 1,
    .addr = data,
    .length = 0,
    .rxlength = 0,
  };
  spi_device_transmit(SPI_handle, &spi_tran);
}

void ST7701_CS_EN(){
  Set_EXIO(EXIO_PIN3,Low);
  vTaskDelay(pdMS_TO_TICKS(10));
}
void ST7701_CS_Dis(){
  Set_EXIO(EXIO_PIN3,High);
  vTaskDelay(pdMS_TO_TICKS(10));
  vTaskDelay(pdMS_TO_TICKS(120));
}
void ST7701_Reset(){
  Set_EXIO(EXIO_PIN1,Low);
  vTaskDelay(pdMS_TO_TICKS(10));
  Set_EXIO(EXIO_PIN1,High);
  vTaskDelay(pdMS_TO_TICKS(50));
}
void ST7701_Init()
{
  // 初始化SPI总线
  spi_bus_config_t buscfg = {
    .mosi_io_num = LCD_MOSI_PIN,
    .miso_io_num = -1,
    .sclk_io_num = LCD_CLK_PIN,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = 64, // ESP32 S3 max size is 64Kbytes
  };
  spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
  spi_device_interface_config_t devcfg = {
    .command_bits = 1,
    .address_bits = 8,
    .mode = SPI_MODE0,
    .clock_speed_hz = 80000000,
    .spics_io_num = -1,                     
    .queue_size = 1,            // Not using queues
  };
  spi_bus_add_device(SPI2_HOST, &devcfg, &SPI_handle);            

  ST7701_CS_EN();
  
  // 2.8inch
  ST7701_WriteCommand(0xFF);ST7701_WriteData(0x77);ST7701_WriteData(0x01);ST7701_WriteData(0x00);ST7701_WriteData(0x00);ST7701_WriteData(0x13);
  ST7701_WriteCommand(0xEF);ST7701_WriteData(0x08);
  ST7701_WriteCommand(0xFF);ST7701_WriteData(0x77);ST7701_WriteData(0x01);ST7701_WriteData(0x00);ST7701_WriteData(0x00);ST7701_WriteData(0x10);
  ST7701_WriteCommand(0xC0);ST7701_WriteData(0x4F);ST7701_WriteData(0x00);
  ST7701_WriteCommand(0xC1);ST7701_WriteData(0x10);ST7701_WriteData(0x02);
  ST7701_WriteCommand(0xC2);ST7701_WriteData(0x07);ST7701_WriteData(0x02);
  ST7701_WriteCommand(0xCC);ST7701_WriteData(0x10);
  ST7701_WriteCommand(0xB0);ST7701_WriteData(0x00);ST7701_WriteData(0x10);ST7701_WriteData(0x17);ST7701_WriteData(0x0D);ST7701_WriteData(0x11);ST7701_WriteData(0x06);ST7701_WriteData(0x05);ST7701_WriteData(0x08);ST7701_WriteData(0x07);ST7701_WriteData(0x1F);ST7701_WriteData(0x04);ST7701_WriteData(0x11);ST7701_WriteData(0x0E);ST7701_WriteData(0x29);ST7701_WriteData(0x30);ST7701_WriteData(0x1F);
  ST7701_WriteCommand(0xB1);ST7701_WriteData(0x00);ST7701_WriteData(0x0D);ST7701_WriteData(0x14);ST7701_WriteData(0x0E);ST7701_WriteData(0x11);ST7701_WriteData(0x06);ST7701_WriteData(0x04);ST7701_WriteData(0x08);ST7701_WriteData(0x08);ST7701_WriteData(0x20);ST7701_WriteData(0x05);ST7701_WriteData(0x13);ST7701_WriteData(0x13);ST7701_WriteData(0x26);ST7701_WriteData(0x30);ST7701_WriteData(0x1F);
  ST7701_WriteCommand(0xFF);ST7701_WriteData(0x77);ST7701_WriteData(0x01);ST7701_WriteData(0x00);ST7701_WriteData(0x00);ST7701_WriteData(0x11);
  ST7701_WriteCommand(0xB0);ST7701_WriteData(0x65);
  ST7701_WriteCommand(0xB1);ST7701_WriteData(0x71);
  ST7701_WriteCommand(0xB2);ST7701_WriteData(0x82);//87
  ST7701_WriteCommand(0xB3);ST7701_WriteData(0x80);
  ST7701_WriteCommand(0xB5);ST7701_WriteData(0x42);//4D
  ST7701_WriteCommand(0xB7);ST7701_WriteData(0x85);
  ST7701_WriteCommand(0xB8);ST7701_WriteData(0x20);
  ST7701_WriteCommand(0xC0);ST7701_WriteData(0x09);
  ST7701_WriteCommand(0xC1);ST7701_WriteData(0x78);
  ST7701_WriteCommand(0xC2);ST7701_WriteData(0x78);
  ST7701_WriteCommand(0xD0);ST7701_WriteData(0x88);
  ST7701_WriteCommand(0xEE);ST7701_WriteData(0x42);

  ST7701_WriteCommand(0xE0);ST7701_WriteData(0x00);ST7701_WriteData(0x00);ST7701_WriteData(0x02);
  ST7701_WriteCommand(0xE1);ST7701_WriteData(0x04);ST7701_WriteData(0xA0);ST7701_WriteData(0x06);ST7701_WriteData(0xA0);ST7701_WriteData(0x05);ST7701_WriteData(0xA0);ST7701_WriteData(0x07);ST7701_WriteData(0xA0);ST7701_WriteData(0x00);ST7701_WriteData(0x44);ST7701_WriteData(0x44);
  ST7701_WriteCommand(0xE2);ST7701_WriteData(0x00);ST7701_WriteData(0x00);ST7701_WriteData(0x00);ST7701_WriteData(0x00);ST7701_WriteData(0x00);ST7701_WriteData(0x00);ST7701_WriteData(0x00);ST7701_WriteData(0x00);ST7701_WriteData(0x00);ST7701_WriteData(0x00);ST7701_WriteData(0x00);ST7701_WriteData(0x00);
  ST7701_WriteCommand(0xE3);ST7701_WriteData(0x00);ST7701_WriteData(0x00);ST7701_WriteData(0x22);ST7701_WriteData(0x22);
  ST7701_WriteCommand(0xE4);ST7701_WriteData(0x44);ST7701_WriteData(0x44);
  ST7701_WriteCommand(0xE5);ST7701_WriteData(0x0c);ST7701_WriteData(0x90);ST7701_WriteData(0xA0);ST7701_WriteData(0xA0);ST7701_WriteData(0x0E);ST7701_WriteData(0x92);ST7701_WriteData(0xA0);ST7701_WriteData(0xA0);ST7701_WriteData(0x08);ST7701_WriteData(0x8C);ST7701_WriteData(0xA0);ST7701_WriteData(0xA0);ST7701_WriteData(0x0A);ST7701_WriteData(0x8E);ST7701_WriteData(0xA0);ST7701_WriteData(0xA0);
  ST7701_WriteCommand(0xE6);ST7701_WriteData(0x00);ST7701_WriteData(0x00);ST7701_WriteData(0x22);ST7701_WriteData(0x22);
  ST7701_WriteCommand(0xE7);ST7701_WriteData(0x44);ST7701_WriteData(0x44);
  ST7701_WriteCommand(0xE8);ST7701_WriteData(0x0D);ST7701_WriteData(0x91);ST7701_WriteData(0xA0);ST7701_WriteData(0xA0);ST7701_WriteData(0x0F);ST7701_WriteData(0x93);ST7701_WriteData(0xA0);ST7701_WriteData(0xA0);ST7701_WriteData(0x09);ST7701_WriteData(0x8D);ST7701_WriteData(0xA0);ST7701_WriteData(0xA0);ST7701_WriteData(0x0B);ST7701_WriteData(0x8F);ST7701_WriteData(0xA0);ST7701_WriteData(0xA0);
  //ST7701_WriteCommand(0xE9);ST7701_WriteData( 2);ST7701_WriteData(ST7701_WriteCommand(0x36);ST7701_WriteData(0x00);
  ST7701_WriteCommand(0xEB);ST7701_WriteData(0x00);ST7701_WriteData(0x00);ST7701_WriteData(0xE4);ST7701_WriteData(0xE4);ST7701_WriteData(0x44);ST7701_WriteData(0x00);ST7701_WriteData(0x40);
  ST7701_WriteCommand(0xED);ST7701_WriteData(0xFF);ST7701_WriteData(0xF5);ST7701_WriteData(0x47);ST7701_WriteData(0x6F);ST7701_WriteData(0x0B);ST7701_WriteData(0xA1);ST7701_WriteData(0xAB);ST7701_WriteData(0xFF);ST7701_WriteData(0xFF);ST7701_WriteData(0xBA);ST7701_WriteData(0x1A);ST7701_WriteData(0xB0);ST7701_WriteData(0xF6);ST7701_WriteData(0x74);ST7701_WriteData(0x5F);ST7701_WriteData(0xFF);
  ST7701_WriteCommand(0xEF);ST7701_WriteData(0x08);ST7701_WriteData(0x08);ST7701_WriteData(0x08);ST7701_WriteData(0x40);ST7701_WriteData(0x3F);ST7701_WriteData(0x64);
  ST7701_WriteCommand(0xFF);ST7701_WriteData(0x77);ST7701_WriteData(0x01);ST7701_WriteData(0x00);ST7701_WriteData(0x00);ST7701_WriteData(0x00);
  ST7701_WriteCommand(0xFF);ST7701_WriteData(0x77);ST7701_WriteData(0x01);ST7701_WriteData(0x00);ST7701_WriteData(0x00);ST7701_WriteData(0x13);
  ST7701_WriteCommand(0xE6);ST7701_WriteData(0x16);ST7701_WriteData(0x7C);
  ST7701_WriteCommand(0xE8);ST7701_WriteData(0x00);ST7701_WriteData( 0x0E);
  ST7701_WriteCommand(0xFF);ST7701_WriteData(0x77);ST7701_WriteData(0x01);ST7701_WriteData(0x00);ST7701_WriteData(0x00);ST7701_WriteData(0x00);
  ST7701_WriteCommand(0x11);ST7701_WriteData(0x00);
  delay(200);
  ST7701_WriteCommand(0xFF);ST7701_WriteData(0x77);ST7701_WriteData(0x01);ST7701_WriteData(0x00);ST7701_WriteData(0x00);ST7701_WriteData(0x13);
  ST7701_WriteCommand(0xE8);ST7701_WriteData(0x00);ST7701_WriteData( 0x0C);
  delay(150);
  ST7701_WriteCommand(0xE8);ST7701_WriteData(0x00);ST7701_WriteData( 0x00);
  ST7701_WriteCommand(0xFF);ST7701_WriteData(0x77);ST7701_WriteData(0x01);ST7701_WriteData(0x00);ST7701_WriteData(0x00);ST7701_WriteData(0x00);
  ST7701_WriteCommand(0x29);ST7701_WriteData(0x00);
  ST7701_WriteCommand(0x35);ST7701_WriteData(0x00);

  ST7701_WriteCommand(0x11);
  ST7701_WriteData(0x00);	 //sleep out
  delay(200);

  ST7701_WriteCommand(0x29);
  ST7701_WriteData(0x00);	  //display on
  ST7701_WriteCommand(0x29);
  ST7701_WriteData(0x00);	  //display on
  delay(100);

  ST7701_CS_Dis();

  //  RGB
  esp_lcd_rgb_panel_config_t rgb_config = {
    .clk_src = LCD_CLK_SRC_PLL240M,                                                               // LCD_CLK_SRC_PLL160M   LCD_CLK_SRC_PLL240M   LCD_CLK_SRC_XTAL   LCD_CLK_SRC_DEFAULT
    .timings =  {                                                                                 
      .pclk_hz = ESP_PANEL_LCD_RGB_TIMING_FREQ_HZ,                                               
      .h_res = ESP_PANEL_LCD_WIDTH,                                                              
      .v_res = ESP_PANEL_LCD_HEIGHT,                                                           
      .hsync_pulse_width = ESP_PANEL_LCD_RGB_TIMING_HPW,                                         
      .hsync_back_porch = ESP_PANEL_LCD_RGB_TIMING_HBP,                                          
      .hsync_front_porch = ESP_PANEL_LCD_RGB_TIMING_HFP,                                         
      .vsync_pulse_width = ESP_PANEL_LCD_RGB_TIMING_VPW,                                          
      .vsync_back_porch = ESP_PANEL_LCD_RGB_TIMING_VBP,                                           
      .vsync_front_porch = ESP_PANEL_LCD_RGB_TIMING_VFP,                                          
      .flags = {                                                                       
        .pclk_active_neg = ESP_PANEL_LCD_RGB_PCLK_ACTIVE_NEG,                                                                       
      },
    },
    .data_width = ESP_PANEL_LCD_RGB_DATA_WIDTH,                                                   
    .bits_per_pixel = ESP_PANEL_LCD_RGB_PIXEL_BITS,                                               
    .num_fbs = ESP_PANEL_LCD_RGB_FRAME_BUF_NUM,                                                   
    .bounce_buffer_size_px = ESP_PANEL_LCD_RGB_BOUNCE_BUF_SIZE,                                   
    .psram_trans_align = 64,                                                                      
    .hsync_gpio_num = ESP_PANEL_LCD_PIN_NUM_RGB_HSYNC,                                            
    .vsync_gpio_num = ESP_PANEL_LCD_PIN_NUM_RGB_VSYNC,                                            
    .de_gpio_num = ESP_PANEL_LCD_PIN_NUM_RGB_DE,                                                  
    .pclk_gpio_num = ESP_PANEL_LCD_PIN_NUM_RGB_PCLK,            
    .disp_gpio_num = ESP_PANEL_LCD_PIN_NUM_RGB_DISP,                                                                              
    .data_gpio_nums = {                                                                                                                           
      ESP_PANEL_LCD_PIN_NUM_RGB_DATA0,                                                            
      ESP_PANEL_LCD_PIN_NUM_RGB_DATA1,                                                            
      ESP_PANEL_LCD_PIN_NUM_RGB_DATA2,                                                            
      ESP_PANEL_LCD_PIN_NUM_RGB_DATA3,                                                            
      ESP_PANEL_LCD_PIN_NUM_RGB_DATA4,                                                            
      ESP_PANEL_LCD_PIN_NUM_RGB_DATA5,                                                            
      ESP_PANEL_LCD_PIN_NUM_RGB_DATA6,                                                            
      ESP_PANEL_LCD_PIN_NUM_RGB_DATA7,                                                            
      ESP_PANEL_LCD_PIN_NUM_RGB_DATA8,                                                            
      ESP_PANEL_LCD_PIN_NUM_RGB_DATA9,                                                            
      ESP_PANEL_LCD_PIN_NUM_RGB_DATA10,                                                           
      ESP_PANEL_LCD_PIN_NUM_RGB_DATA11,                                                           
      ESP_PANEL_LCD_PIN_NUM_RGB_DATA12,                                                           
      ESP_PANEL_LCD_PIN_NUM_RGB_DATA13,                                                           
      ESP_PANEL_LCD_PIN_NUM_RGB_DATA14,                                                           
      ESP_PANEL_LCD_PIN_NUM_RGB_DATA15,                                                           
    },  
    .flags = {                                                                                    
      .fb_in_psram = true,                                                                        // 如果启用此标志，帧缓冲区将优先从PSRAM分配
      .double_fb = true
    },
  };
  esp_lcd_new_rgb_panel(&rgb_config, &panel_handle); 
  esp_lcd_rgb_panel_event_callbacks_t cbs = {
    .on_vsync = example_on_vsync_event,
  };
  esp_lcd_rgb_panel_register_event_callbacks(panel_handle, &cbs, NULL);
  esp_lcd_panel_reset(panel_handle);
  esp_lcd_panel_init(panel_handle);
}

bool example_on_vsync_event(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *event_data, void *user_data)
{
  BaseType_t high_task_awoken = pdFALSE;
  return high_task_awoken == pdTRUE;
}
void LCD_Init() {
  TCA9554PWR_Init(0x00);
  Set_EXIO(EXIO_PIN8,Low);
  ST7701_Reset();
  ST7701_Init();
  Touch_Init();
}

void LCD_addWindow(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend,uint8_t* color) {
  Xend = Xend + 1;      // esp_lcd_panel_draw_bitmap: x_end End index on x-axis (x_end not included)
  Yend = Yend + 1;      // esp_lcd_panel_draw_bitmap: y_end End index on y-axis (y_end not included)
  if (Xend > ESP_PANEL_LCD_WIDTH)
    Xend = ESP_PANEL_LCD_WIDTH;
  if (Yend > ESP_PANEL_LCD_HEIGHT)
    Yend = ESP_PANEL_LCD_HEIGHT;

  esp_lcd_panel_draw_bitmap(panel_handle, Xstart, Ystart, Xend, Yend, color);                     // x_end End index on x-axis (x_end not included)
}


// backlight (this might need improved upon!)
#define PWM_CHANNEL_BCKL (SOC_LEDC_CHANNEL_NUM - 1)
void Backlight_Init()
{
  #if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(LCD_Backlight_PIN, Frequency, Resolution);   
  ledcWrite(LCD_Backlight_PIN, Dutyfactor);
  #else        
    ledcSetup(PWM_CHANNEL_BCKL, Frequency, Resolution); // Set frequency to 50Hz, resolution to 10 bits
  ledcAttachPin(LCD_Backlight_PIN, PWM_CHANNEL_BCKL); // Associate GPIO pin with LEDC channel
  digitalWrite(PWM_CHANNEL_BCKL, LOW);
  
  
  #endif

    Set_Backlight(LCD_Backlight);      //0~100    
}

void Set_Backlight(uint8_t Light)                       
{
  
  if(Light > Backlight_MAX || Light < 0)
    printf("Set Backlight parameters in the range of 0 to 100 \r\n");
  else{
    uint32_t Backlight = Light*10;
    if(Backlight == 1000)
      Backlight = 1024;
    ledcWrite(PWM_CHANNEL_BCKL, Backlight);
  }
}





