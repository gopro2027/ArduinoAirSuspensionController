#include "PWR_Key.h"

#include "PMU_AXP2101.h"     // unified PMIC facade
#include "axp2101_pmic.h"    // pmic::handle()
#include "i2c_guard.h"       // shared bus mutex

static bool s_pmic_ok     = false;
static bool s_pmic_inited = false;
static unsigned long s_pmic_ready_ms = 0;

// Shared power state machine (src/waveshare/PWR_Key.cpp); not on this library's include path.
extern void Shutdown(void);

// The press that powers the board on is released while we are still booting, and the AXP latches
// that as a key IRQ. Ignore key IRQs until this has elapsed or the device powers straight back off.
#define PWR_KEY_BOOT_IGNORE_MS 3000

// This board has no latch FET, so write_latch() must always take the early-out below and
// Shutdown()'s power_latch_off() must stay a no-op.
static_assert(PWR_LATCH_PIN < 0,
              "ws1p75c has no power latch GPIO; power is held by the AXP2101. "
              "Use power_hard_shutdown() to drop the rails.");

static inline void write_latch(bool on)
{
    if (PWR_LATCH_PIN < 0) {
        return; // no latch configured
    }

    pinMode(PWR_LATCH_PIN, OUTPUT);
    digitalWrite(PWR_LATCH_PIN, on ? PWR_LATCH_ACTIVE_LEVEL : !PWR_LATCH_ACTIVE_LEVEL);
}

// Both no-ops here (PWR_LATCH_PIN < 0). Real power-off is power_hard_shutdown() below -- do not
// move it in here, PWR_Init() calls power_latch_off() on every boot as part of its latch probe.
void power_latch_on()  { write_latch(true); }
void power_latch_off() { write_latch(false); }

static void pmic_init_once()
{
    if (s_pmic_inited) {
        return;
    }
    s_pmic_inited = true;

    if (!PMU_init()) {
        Serial.println("[PWR] PMIC init failed; running without PMIC");
        s_pmic_ok = false;
        return;
    }

    s_pmic_ok = true;
    s_pmic_ready_ms = millis();
    Serial.println("[PWR] PMIC initialized (via PMU_AXP2101)");
}

void power_hard_shutdown()
{
    if (!s_pmic_ok) {
        Serial.println("[PWR] No PMIC; cannot hard power off");
        return;
    }

    Serial.println("[PWR] AXP2101 shutdown");
    Serial.flush();

    if (!i2c_lock(50)) {
        Serial.println("[PWR] I2C busy; cannot hard power off");
        return;
    }
    pmic::handle().shutdown();
    i2c_unlock();

    // Rails should be gone by now. If we are still executing, the PMIC refused (VBUS present,
    // for example) -- return and let the caller fall back to its deep-sleep shutdown.
    delay(200);
    Serial.println("[PWR] Still alive after AXP2101 shutdown; falling back");
}

static void configure_ext0_wakeup()
{
#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32S2
    esp_sleep_enable_ext0_wakeup(static_cast<gpio_num_t>(PWR_WAKE_GPIO),
                                 PWR_WAKE_ACTIVE_LOW ? 0 : 1);
#endif
}

void power_key_setup()
{
#if PWR_WAKE_ACTIVE_LOW
    pinMode(PWR_WAKE_GPIO, INPUT_PULLUP);
#else
    pinMode(PWR_WAKE_GPIO, INPUT);
#endif

    write_latch(true);
    pmic_init_once();
}

// PWR_Loop() calls this every 100ms and it is the only periodic hook this board has, so the
// AXP2101's PWRON key IRQs are serviced here as well.
bool power_key_pressed()
{
    if (s_pmic_ok && (millis() - s_pmic_ready_ms) > PWR_KEY_BOOT_IGNORE_MS) {
        pmic::poll();
        // PWRON is not on a GPIO, so a press only ever arrives as an IRQ. The AXP's own press-off
        // is already at its 4s minimum, so shutting down on the key IRQ is the only way to make the
        // button react faster than that.
        const bool shortPress = pmic::consume_power_key_short();
        const bool longPress = pmic::consume_power_key_long();
        if (shortPress || longPress) {
            Serial.println("[PWR] PWRON key -> shutdown");
            Shutdown(); // does not return once the PMIC drops the rails
        }
    }

    // Always false: the side PWR button is not readable as a level, and BOOT (GPIO0) is the
    // air-up/preset shortcut button -- reporting it here made PWR_Loop() treat every long shortcut
    // hold as a shutdown request, blanking the screen then powering off on release.
    // ; was: digitalRead(PWR_WAKE_GPIO) == (PWR_WAKE_ACTIVE_LOW ? LOW : HIGH)
    return false;
}

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

void power_enable_wakeup_deepsleep()
{
    configure_ext0_wakeup();
}
