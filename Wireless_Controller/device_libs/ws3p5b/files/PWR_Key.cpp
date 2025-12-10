#include "PWR_Key.h"

#include <Arduino.h>
#include <Wire.h>
#include <TCA9554.h>

#include "PMU_AXP2101.h"
#include "axp2101_pmic.h"

#include <SensorQMI8658.hpp>

// -----------------------------------------------------------------------------
// Board config
// -----------------------------------------------------------------------------

#ifndef TCA_ADDR
  #define TCA_ADDR 0x20
#endif

#ifndef TCA_LCD_RSTBIT
  #define TCA_LCD_RSTBIT 1
#endif

#ifndef TCA_PWR_KEY_BIT
  #define TCA_PWR_KEY_BIT 6
#endif

// IMU I2C + interrupt pins for the QMI8658 on this board
#ifndef IMU_SDA_PIN
  #define IMU_SDA_PIN 8
#endif

#ifndef IMU_SCL_PIN
  #define IMU_SCL_PIN 7
#endif

// GPIO connected to QMI8658 INT1 (labelled IMU_INT1 in the schematic).
// Set this to the actual pin for your board (e.g. 16) or override in
// platformio.ini / build flags.
#ifndef IMU_INT_GPIO
  #define IMU_INT_GPIO -1
#endif

// Default I2C address for QMI8658C on Waveshare board
#ifndef QMI8658_I2C_ADDR
  #define QMI8658_I2C_ADDR QMI8658_L_SLAVE_ADDRESS
#endif

#ifndef WAVESHARE_PMIC_DEFAULT_ACTIONS
  #define WAVESHARE_PMIC_DEFAULT_ACTIONS 1
#endif

// -----------------------------------------------------------------------------
// Local state
// -----------------------------------------------------------------------------

static bool  s_pmic_ok          = false;
static bool  s_pmic_inited      = false;
static TCA9554 *s_tca           = nullptr;

#if defined(WAVESHARE_PMIC_DEFAULT_ACTIONS)
static bool  s_blanked          = false;
static float s_brightness_on    = 0.80f;
static float s_brightness_blank = 0.01f;
#endif

// IMU / motion-wake state
static SensorQMI8658 s_imu;
static bool          s_imu_ok              = false;
static bool          s_motion_wake_armed   = false;
static bool          s_motion_wake_pending = false;

// Remember last wakeup cause so we can tell timer vs. motion
static esp_sleep_wakeup_cause_t s_last_wakeup_cause = ESP_SLEEP_WAKEUP_UNDEFINED;

// -----------------------------------------------------------------------------
// External helpers
// -----------------------------------------------------------------------------

void set_brightness(float level);

// -----------------------------------------------------------------------------
// Latch control
// -----------------------------------------------------------------------------

static inline void write_latch(bool on) {
    if (PWR_LATCH_PIN < 0) return;
    pinMode(PWR_LATCH_PIN, OUTPUT);
    digitalWrite(PWR_LATCH_PIN,
                 on ? PWR_LATCH_ACTIVE_LEVEL : !PWR_LATCH_ACTIVE_LEVEL);
}

void power_latch_on()  { write_latch(true); }
void power_latch_off() { write_latch(false); }

// -----------------------------------------------------------------------------
// LCD reset via TCA9554
// -----------------------------------------------------------------------------

static void reset_panel_via_tca()
{
    if (!s_tca) {
        s_tca = new TCA9554(TCA_ADDR);
        if (!s_tca->begin()) {
            Serial.println("[PWR] TCA9554 init failed for LCD reset");
            delete s_tca;
            s_tca = nullptr;
            return;
        }
    }

    s_tca->pinMode1(TCA_LCD_RSTBIT, OUTPUT);
    s_tca->write1(TCA_LCD_RSTBIT, 1);
    delay(10);
    s_tca->write1(TCA_LCD_RSTBIT, 0);
    delay(10);
    s_tca->write1(TCA_LCD_RSTBIT, 1);
    delay(180);

    Serial.println("[PWR] LCD reset via TCA9554 complete");
}

// -----------------------------------------------------------------------------
// PMIC power-key callbacks
// -----------------------------------------------------------------------------

#if defined(WAVESHARE_PMIC_DEFAULT_ACTIONS)

static void on_power_short()
{
    Serial.println("[PMIC] Power key short press detected");

    // Toggle blanked state
    s_blanked = !s_blanked;

    if (s_blanked) {
        Serial.println("[PMIC] -> Screen will blank (sleep from main loop)");
    } else {
        Serial.println("[PMIC] -> Screen will wake");
        set_brightness(s_brightness_on);
    }
}

static void on_power_long()
{
    Serial.println("[PMIC] Long press -> deep sleep");
    set_brightness(0.0f);
    delay(500);
    Serial.flush();
    esp_deep_sleep_start();
}

#endif

// -----------------------------------------------------------------------------
// IMU helpers (init + wake-on-motion stubs)
// -----------------------------------------------------------------------------

static void imu_init_once()
{
    if (s_imu_ok) return;

#if IMU_INT_GPIO >= 0
    // Init I2C bus (if not already done elsewhere)
    Wire.begin(IMU_SDA_PIN, IMU_SCL_PIN);

    // Optional: use interrupt pin with the SensorQMI8658 API
    s_imu.setPins(IMU_INT_GPIO);

    if (!s_imu.begin(Wire, QMI8658_I2C_ADDR, IMU_SDA_PIN, IMU_SCL_PIN)) {
        Serial.println("[IMU] QMI8658 init FAILED, wake-on-motion disabled");
        s_imu_ok = false;
        return;
    }

    // Basic accel/gyro config – tune as you like
    s_imu.configAccelerometer(
        SensorQMI8658::ACC_RANGE_4G,
        SensorQMI8658::ACC_ODR_LOWPOWER_21Hz,
        SensorQMI8658::LPF_MODE_0);

    s_imu.configGyroscope(
        SensorQMI8658::GYR_RANGE_64DPS,
        SensorQMI8658::GYR_ODR_224_2Hz,
        SensorQMI8658::LPF_MODE_3);

    s_imu.enableAccelerometer();
    s_imu.enableGyroscope();

    // NOTE: by default the Lewis He lib uses data-ready interrupts.
    // For true wake-on-motion you should configure the QMI8658
    // WOM / any-motion interrupt here (register programming),
    // or extend the library with such a function.
    //
    // For now, we just make sure INT1 is configured as an interrupt pin;
    // the actual motion configuration is done in imu_arm_wake_on_motion().

    Serial.println("[IMU] QMI8658 initialised");
    s_imu_ok = true;
#else
    Serial.println("[IMU] IMU_INT_GPIO not defined, IMU wake disabled");
    s_imu_ok = false;
#endif
}

// Put the IMU into a low-power “wake-on-motion” mode and ensure INT1
// is configured to assert on motion. This is intentionally left as a
// stub where you can drop your own register-level config if you like.
static void imu_arm_wake_on_motion()
{
    if (!s_imu_ok) return;

#if IMU_INT_GPIO >= 0
    Serial.println("[IMU] Arming wake-on-motion");

    // TODO: Replace this block with proper WOM register configuration.
    // For now we enable its interrupt line; user should customise.

    // The Lewis He SensorQMI8658 library exposes low-level APIs like
    //   s_imu.enableINT(SensorQMI8658::INTERRUPT_PIN_1, true);
    // Use that (or direct Wire writes) to:
    //   * put the accel in low-power mode
    //   * configure an any-motion or significant-motion interrupt
    //   * route that interrupt to INT1
    //
    // Example placeholder:
    // s_imu.enableINT(SensorQMI8658::INTERRUPT_PIN_1, true);

#endif
}

// Undo any special WOM configuration – return IMU to normal run mode
// (or deep sleep) once we are awake again.
static void imu_disarm_wake_on_motion()
{
    if (!s_imu_ok) return;

#if IMU_INT_GPIO >= 0
    Serial.println("[IMU] Disarming wake-on-motion");

    // TODO: Complement your WOM config here (disable motion interrupt,
    // restore normal ODRs, or put device into low-power/deep-sleep).
    //
    // Example placeholder:
    // s_imu.enableINT(SensorQMI8658::INTERRUPT_PIN_1, false);

#endif
}

// -----------------------------------------------------------------------------
// One-time PMIC init
// -----------------------------------------------------------------------------

static void pmic_init_once()
{
    if (s_pmic_inited) return;
    s_pmic_inited = true;

    if (!PMU_init()) {
        Serial.println("[PWR] PMIC init failed; running without PMIC");
        s_pmic_ok = false;
        return;
    }

    s_pmic_ok = true;

#if defined(WAVESHARE_PMIC_DEFAULT_ACTIONS)
    pmic::set_power_key_handlers(&on_power_short, &on_power_long);
#endif

    Serial.println("[PWR] PMIC initialized");
}

// -----------------------------------------------------------------------------
// Public API expected by PWR_Key.c
// -----------------------------------------------------------------------------

void power_key_setup()
{
    s_last_wakeup_cause = esp_sleep_get_wakeup_cause();

    switch (s_last_wakeup_cause) {
        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.println("[PWR] Woke from timer");
            break;
        case ESP_SLEEP_WAKEUP_EXT0:
            Serial.println("[PWR] Woke from EXT0 (IMU / external)");
            break;
        case ESP_SLEEP_WAKEUP_EXT1:
            Serial.println("[PWR] Woke from EXT1");
            break;
        default:
            Serial.println("[PWR] Normal boot");
            break;
    }

    // TCA9554 for power key + LCD reset
    s_tca = new TCA9554(TCA_ADDR);
    if (!s_tca->begin()) {
        Serial.println("[PWR] TCA9554 init failed");
        delete s_tca;
        s_tca = nullptr;
    } else {
        s_tca->pinMode1(TCA_PWR_KEY_BIT, INPUT);
        Serial.printf("[PWR] TCA9554 initialized, PWR button on EXIO%d\n",
                      TCA_PWR_KEY_BIT);
    }

    write_latch(true);
    pmic_init_once();
    imu_init_once();

#if defined(WAVESHARE_PMIC_DEFAULT_ACTIONS)
    s_blanked = false;
    set_brightness(s_brightness_on);
#endif

    // Clear motion-wake state on boot
    s_motion_wake_armed   = false;
    s_motion_wake_pending = false;
}

bool power_key_pressed()
{
    // If we woke from motion, treat that as a "virtual" key press once.
    if (s_motion_wake_pending) {
        s_motion_wake_pending = false;   // consume the event
        Serial.println("[PWR] power_key_pressed(): IMU motion wake");
        return true;
    }

    if (!s_tca) return false;

    uint8_t state = s_tca->read1(TCA_PWR_KEY_BIT);

#if PWR_KEY_ACTIVE_LOW
    return (state == LOW);
#else
    return (state == HIGH);
#endif
}

// This is a board-specific helper loop used only by the Waveshare
// implementation (your app calls it directly if you use the PMIC
// short/long press scheme). It does NOT affect the generic PWR_Loop()
// logic in PWR_Key.c, which we are leaving untouched.
void power_key_loop()
{
    static uint32_t last_sleep_check = 0;
    uint32_t now = millis();

    // Poll PMIC for power key events
    if (s_pmic_ok) {
        pmic::poll();
    }

#if defined(WAVESHARE_PMIC_DEFAULT_ACTIONS)
    // Handle sleep state - check every 100ms
    if ((now - last_sleep_check) > 100) {
        last_sleep_check = now;

        if (s_blanked) {
            // The main app wants the UI off; let PWR_Key.c handle the
            // actual Fall_Asleep() via its own state machine. We just
            // dim the backlight here.
            static bool first_sleep = true;

            if (first_sleep) {
                Serial.println("[PWR] (PMIC) screen blank requested");
                set_brightness(s_brightness_blank);
                Serial.flush();
                first_sleep = false;
            }
        } else {
            // Awake - ensure backlight is on
            static uint32_t last_brightness_check = 0;
            if ((now - last_brightness_check) > 5000) {
                last_brightness_check = now;
                set_brightness(s_brightness_on);
            }
        }
    }
#endif
}

void power_key_reset_lcd()
{
    reset_panel_via_tca();
}

// -----------------------------------------------------------------------------
// Wakeup config used by PWR_Key.c::Fall_Asleep()
// -----------------------------------------------------------------------------

void power_enable_wakeup_lightsleep()
{
    // Called right before esp_light_sleep_start() in Fall_Asleep()

    Serial.println("[PWR] Enabling wakeup sources for light sleep");

    // 1) Keep the 10-minute timer from Fall_Asleep() (already configured).
    //    We don't change it here.

#if IMU_INT_GPIO >= 0
    if (s_imu_ok) {
        // 2) Arm the IMU wake-on-motion mode
        imu_arm_wake_on_motion();
        s_motion_wake_armed   = true;
        s_motion_wake_pending = false;

        // 3) Configure EXT0 wake on the IMU interrupt pin.
        // Choose the level based on how you configure the QMI8658 INT1
        // line. Most examples use active-low.
        pinMode(IMU_INT_GPIO, INPUT_PULLUP);
        esp_sleep_enable_ext0_wakeup((gpio_num_t)IMU_INT_GPIO, 0); // wake on LOW

        Serial.printf("[PWR] EXT0 wake enabled on IMU_INT GPIO %d\n",
                      IMU_INT_GPIO);
    } else {
        Serial.println("[PWR] IMU not OK, motion wake disabled");
    }
#else
    Serial.println("[PWR] IMU_INT_GPIO < 0, motion wake disabled");
#endif

    // NOTE: GPIO wake via the TCA9554 power button is not possible here,
    // because the button is behind an I2C expander.
}

void power_disable_wakeup_lightsleep()
{
    // Called at the beginning of onWakeup() in PWR_Key.c

    s_last_wakeup_cause = esp_sleep_get_wakeup_cause();
    Serial.printf("[PWR] Disable wakeup (cause=%d)\n", s_last_wakeup_cause);

#if IMU_INT_GPIO >= 0
    if (s_imu_ok && s_motion_wake_armed &&
        s_last_wakeup_cause == ESP_SLEEP_WAKEUP_EXT0) {

        // We woke from the IMU interrupt – mark a virtual key press so
        // that onWakeup() in PWR_Key.c treats this like a user pressing
        // the power key (and therefore does NOT auto-shutdown).
        s_motion_wake_pending = true;
        Serial.println("[PWR] Motion wake detected via IMU");
    }
#endif

    // Disable timer wake (will be re-armed on next Fall_Asleep call)
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);

#if IMU_INT_GPIO >= 0
    if (s_imu_ok) {
        // Disable EXT0 wake + WOM configuration on the IMU
        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_EXT0);
        imu_disarm_wake_on_motion();
    }
#endif

    s_motion_wake_armed = false;
}

void power_enable_wakeup_deepsleep()
{
    // For deep sleep, we still use the existing behaviour: a 10-minute
    // timer as a safety fallback. Wake from deep sleep always reboots.
    esp_sleep_enable_timer_wakeup(10 * 60 * 1000000ULL);
    Serial.println("[PWR] Deep sleep wakeup via timer (10min)");
}
