#include "PWR_Key.h"

#include <Wire.h>
#include <TCA9554.h>

#include "PMU_AXP2101.h"     // unified PMIC facade
#include "axp2101_pmic.h"    // pmic::set_power_key_handlers(), pmic::poll()

// From your board driver (already used elsewhere)
void set_brightness(float level);

// ---------- Board-specific defaults (Waveshare 3.5B) ----------

#ifndef TCA_ADDR
  #define TCA_ADDR 0x20
#endif

#ifndef TCA_LCD_RSTBIT
  #define TCA_LCD_RSTBIT 1
#endif

// Uncomment/define if you want the short/long power-key actions enabled
//   - short press: toggle display backlight between on/off
//   - long  press: force backlight off (you could hook this to deep sleep)
#ifndef WAVESHARE_PMIC_DEFAULT_ACTIONS
  #define WAVESHARE_PMIC_DEFAULT_ACTIONS 0
#endif

// ---------- Local state ----------

static bool  s_pmic_ok          = false;
static bool  s_pmic_inited      = false;

#if defined(WAVESHARE_PMIC_DEFAULT_ACTIONS)
static bool  s_blanked          = false;
static float s_brightness_on    = 0.80f;  // adjust to taste
static float s_brightness_blank = 0.01f;  // "off" but keep LEDC alive
#endif

// ----------------- Helper: drive latch pin if present -----------------

static inline void write_latch(bool on)
{
    if (PWR_LATCH_PIN < 0) {
        return; // no latch configured
    }

    pinMode(PWR_LATCH_PIN, OUTPUT);
    digitalWrite(
        PWR_LATCH_PIN,
        on ? PWR_LATCH_ACTIVE_LEVEL
           : !PWR_LATCH_ACTIVE_LEVEL
    );
}

// Convenience wrappers for public API
void power_latch_on()  { write_latch(true); }
void power_latch_off() { write_latch(false); }

// ----------------- LCD reset via TCA9554 -----------------

static void reset_panel_via_tca()
{
    // I2C + guard are assumed to be initialised already by PMU_init().
    // Here we only talk to the expander and pulse the LCD reset line.

    TCA9554 tca(TCA_ADDR);
    if (!tca.begin()) {
        Serial.println("[PWR] TCA9554 init failed, skipping LCD reset");
        return;
    }

    // Only configure and drive the LCD reset pin.
    tca.pinMode1(TCA_LCD_RSTBIT, OUTPUT);
    tca.write1(TCA_LCD_RSTBIT, 1);  // default high

    delay(10);
    tca.write1(TCA_LCD_RSTBIT, 1); delay(10);
    tca.write1(TCA_LCD_RSTBIT, 0); delay(10);
    tca.write1(TCA_LCD_RSTBIT, 1); delay(180);

    Serial.println("[PWR] LCD reset via TCA9554 complete");
}

// ----------------- PMIC power-key callbacks -----------------

#if defined(WAVESHARE_PMIC_DEFAULT_ACTIONS)

static void on_power_short()
{
    // Toggle backlight using your project's set_brightness()
    s_blanked = !s_blanked;
    set_brightness(s_blanked ? s_brightness_blank : s_brightness_on);

    Serial.println(
        s_blanked
        ? "[PMIC] Short press -> backlight OFF"
        : "[PMIC] Short press -> backlight ON"
    );
}

static void on_power_long()
{
    // Long-press: blank the display (you could add deep-sleep here)
    s_blanked = true;
    set_brightness(s_brightness_blank);

    Serial.println("[PMIC] Long press -> backlight forced OFF");
}

#endif // WAVESHARE_PMIC_DEFAULT_ACTIONS

// ----------------- One-time PMIC + panel init -----------------

static void pmic_init_once()
{
    if (s_pmic_inited) {
        return;
    }
    s_pmic_inited = true;

    // Use the unified PMU facade so the PMIC / I2C bus are only initialised once.
    if (!PMU_init()) {
        Serial.println("[PWR] PMIC init failed; running without PMIC");
        s_pmic_ok = false;
        return;
    }

    s_pmic_ok = true;

#if defined(WAVESHARE_PMIC_DEFAULT_ACTIONS)
    // Register the short/long handlers with the PMIC driver
    pmic::set_power_key_handlers(&on_power_short, &on_power_long);
#endif

    // Clean LCD reset via TCA expander (rails should now be up)
    reset_panel_via_tca();

    Serial.println("[PWR] PMIC initialized (via PMU_AXP2101) and power-key handlers registered");
}

// ----------------- Wakeup helpers -----------------

static void configure_ext0_wakeup()
{
#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32S2
    // ext0 wakeup uses RTC-capable GPIO; ensure PWR_KEY_PIN is such a pin.
    // Level: 0 for active-low buttons, 1 for active-high.
    esp_sleep_enable_ext0_wakeup(
        static_cast<gpio_num_t>(PWR_KEY_PIN),
        PWR_KEY_ACTIVE_LOW ? 0 : 1
    );
#endif
}

// ----------------- Public API -----------------

void power_key_setup()
{
#if PWR_KEY_ACTIVE_LOW
    pinMode(PWR_KEY_PIN, INPUT_PULLUP);
#else
    pinMode(PWR_KEY_PIN, INPUT);
#endif

    // Default: keep the board latched ON after boot
    write_latch(true);

    // Initialize PMIC + LCD power/reset for Waveshare 3.5B
    pmic_init_once();
}

bool power_key_pressed()
{
#if PWR_KEY_ACTIVE_LOW
    return digitalRead(PWR_KEY_PIN) == LOW;
#else
    return digitalRead(PWR_KEY_PIN) == HIGH;
#endif
}

void power_key_loop()
{
    // Let the PMIC driver service its IRQs / power-key events.
    if (s_pmic_ok) {
        pmic::poll();
    }
}

// Light-sleep wake on power key
void power_enable_wakeup_lightsleep()
{
    configure_ext0_wakeup();
}

void power_disable_wakeup_lightsleep()
{
#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32S2
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_EXT0);
#endif
}

// Deep-sleep wake on power key (same trigger)
void power_enable_wakeup_deepsleep()
{
    configure_ext0_wakeup();
}
