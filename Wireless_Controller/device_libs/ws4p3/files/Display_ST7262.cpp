#include "Display_ST7262.h"
#include "Touch_GT911.h"

esp_lcd_panel_handle_t panel_handle = NULL;
uint8_t LCD_Backlight = 100;

// Hold LCD_RST (EXIO3) low, then release. Waveshare's LCD pre-begin hook for this board.
static void ST7262_Reset()
{
  CH422G_Set_EXIO(EXIO_LCD_RST, Low);
  vTaskDelay(pdMS_TO_TICKS(10));
  CH422G_Set_EXIO(EXIO_LCD_RST, High);
  vTaskDelay(pdMS_TO_TICKS(100));
}

void ST7262_Init()
{
  // The ST7262 takes no commands -- everything below is the ESP32-S3 LCD_CAM RGB peripheral.
  esp_lcd_rgb_panel_config_t rgb_config = {
    .clk_src = LCD_CLK_SRC_PLL240M,
    .timings = {
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
      // 800*480*2 = 768 KB. Internal RAM could never hold it; the N8R8's 8 MB of octal PSRAM can.
      .fb_in_psram = true,
    },
  };

  esp_err_t err = esp_lcd_new_rgb_panel(&rgb_config, &panel_handle);
  if (err != ESP_OK) {
    // Almost always a failed 768 KB PSRAM allocation. Say so loudly -- the symptom otherwise is
    // just a dead black screen with no hint of why.
    printf("esp_lcd_new_rgb_panel failed: %s\r\n", esp_err_to_name(err));
    return;
  }
  esp_lcd_panel_reset(panel_handle);
  esp_lcd_panel_init(panel_handle);
}

void LCD_Init()
{
  CH422G_Init();
  ST7262_Reset();
  ST7262_Init();
  Touch_Init();
}

void LCD_addWindow(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend, uint8_t *color)
{
  Xend = Xend + 1;  // esp_lcd_panel_draw_bitmap: x_end is exclusive
  Yend = Yend + 1;  // esp_lcd_panel_draw_bitmap: y_end is exclusive
  if (Xend > ESP_PANEL_LCD_WIDTH)
    Xend = ESP_PANEL_LCD_WIDTH;
  if (Yend > ESP_PANEL_LCD_HEIGHT)
    Yend = ESP_PANEL_LCD_HEIGHT;

  esp_lcd_panel_draw_bitmap(panel_handle, Xstart, Ystart, Xend, Yend, color);
}

// ───────────────────────────── Backlight ─────────────────────────────
//
// This board has NO backlight GPIO. The MP3302 LED driver's enable pin (schematic net DISP) hangs
// off the CH422G as EXIO2, so the only thing reachable over I2C is on or off -- there is no PWM
// path and no analog dimming. The firmware-wide brightness slider therefore behaves as a switch
// on this device.
//
// (PWM-ing an I2C expander pin from software was considered and rejected: every duty step is a
// full I2C transaction on the same bus the touch panel polls, which would both flicker visibly
// and starve touch.)
//
// Consequence: the inactivity dim in main.cpp (set_brightness(0.01f) -> Light == 1) is a no-op
// here, so the panel stays lit until power-off. That is deliberate. The user-settable brightness
// minimum is ALSO 1 (getBrightnessFloat() clamps to 1..100), so any threshold low enough to make
// dimming work would also let someone black out their own screen from the settings slider with no
// visible way back.

void Backlight_Init()
{
  // Runs before board_drivers_init() calls I2C_Init(), so bring the bus and the expander up here.
  // Both calls are idempotent.
  I2C_Init();
  CH422G_Init();
  // Deliberately left OFF. board_driver_util.cpp turns it on with set_brightness(1) only after the
  // splash screen has been flushed, which keeps power-on garbage off the panel.
}

void Set_Backlight(uint8_t Light)
{
  if (Light > 100) {
    printf("Set Backlight parameters in the range of 0 to 100 \r\n");
    return;
  }
  LCD_Backlight = Light;
  // Anything the user could plausibly mean as "on" turns it fully on; only a real 0 blanks it.
  CH422G_Set_EXIO(EXIO_LCD_BL, Light > 0 ? High : Low);
}
