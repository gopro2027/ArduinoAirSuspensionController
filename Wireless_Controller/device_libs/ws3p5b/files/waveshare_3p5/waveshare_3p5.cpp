#include <Arduino.h>
#include <Wire.h>
#include <TCA9554.h>

#include "axp2101_pmic.h"
#include "pmic_cache.h"
#include "i2c_guard.h"

// Uncomment to enable simple default actions on power-key events:
//   - short press: toggle display backlight between on/off
//   - long  press: force backlight off
//#define WAVESHARE_PMIC_DEFAULT_ACTIONS 1

#if defined(WAVESHARE_PMIC_DEFAULT_ACTIONS)
  #include <esp32_smartdisplay.h>   // for smartdisplay_lcd_set_backlight()
#endif

// Use your project’s I2C pin defs if present
#ifndef I2C_SDA
  #define I2C_SDA 8
#endif
#ifndef I2C_SCL
  #define I2C_SCL 7
#endif

#ifndef TCA_ADDR
  #define TCA_ADDR 0x20
#endif
#ifndef TCA_LCD_RSTBIT
  #define TCA_LCD_RSTBIT 1
#endif

namespace waveshare_3p5 {

static void reset_panel_via_tca()
{
  Wire.begin(I2C_SDA, I2C_SCL, 400000);
  TCA9554 tca(TCA_ADDR);
  tca.begin();
  for (uint8_t b = 0; b < 8; ++b) { tca.pinMode1(b, OUTPUT); tca.write1(b, 1); }
  delay(10);
  tca.write1(TCA_LCD_RSTBIT, 1); delay(10);
  tca.write1(TCA_LCD_RSTBIT, 0); delay(10);
  tca.write1(TCA_LCD_RSTBIT, 1); delay(180);
}

#if defined(WAVESHARE_PMIC_DEFAULT_ACTIONS)
static bool   s_blanked            = false;
static float  s_brightness_on      = 0.80f;  // adjust if you like
static float  s_brightness_blank   = 0.01f;  // "off" but keeps LEDC active

static void on_power_short()
{
  // Toggle backlight
  s_blanked = !s_blanked;
  smartdisplay_lcd_set_backlight(s_blanked ? s_brightness_blank : s_brightness_on);
  Serial.println(s_blanked ? "[PMIC] Short press → backlight OFF" : "[PMIC] Short press → backlight ON");
}

static void on_power_long()
{
  // Force off; you could initiate deep sleep here instead
  s_blanked = true;
  smartdisplay_lcd_set_backlight(0.0f);
  Serial.println("[PMIC] Long press → backlight OFF");
}
#else
// No default actions: just log. You can still poll flags if you implemented them in pmic.
static void on_power_short() { Serial.println("[PMIC] Short press"); }
static void on_power_long()  { Serial.println("[PMIC] Long press");  }
#endif

void waveshare_3p5_init()
{
    i2c_guard_init(&Wire, 20);

  // 1) Bring up PMIC rails first
  if (!pmic::begin(&Wire, I2C_SDA, I2C_SCL, /*irqPin*/ -1)) {
    Serial.println("[Waveshare3.5] PMIC init failed; continuing without PMIC");
  } else {
    pmic::enable_display_power(true);
    pmic_cache_start(1000 /*ms*/, 0 /*core 0*/);
    pmic::set_measure_period_ms(2000);  // 2s

    // Optional: configure power-key timings (matches Waveshare demo style)
    // (These enums come from XPowersLib via axp2101_pmic.h)
    pmic::handle().setPowerKeyPressOffTime(XPOWERS_POWEROFF_4S);
    pmic::handle().setPowerKeyPressOnTime(XPOWERS_POWERON_128MS);

    // Register callbacks so power-key events do something visible
    pmic::set_power_key_handlers(&on_power_short, &on_power_long);
  }

  // 2) Clean LCD reset through TCA expander
  reset_panel_via_tca();
}

void waveshare_3p5_loop()
{
  // Poll PMIC IRQs (if you wired NIRQ; otherwise this still works as a poller)
  pmic::poll();

  // If you didn’t wire NIRQ and prefer polling without callbacks, you can also do:
  // if (pmic::consume_power_key_short()) on_power_short();
  // if (pmic::consume_power_key_long())  on_power_long();
}

} // namespace waveshare_3p5
