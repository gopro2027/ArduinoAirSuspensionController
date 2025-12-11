#include "axp2101_pmic.h"
#include "i2c_guard.h"

namespace pmic {

#define I2C_LOCK(ms)   if (!i2c_lock(ms)) return
#define I2C_UNLOCK()   i2c_unlock()

static XPowersPMU g_pmu;
static int  g_irq     = -1;   // kept for API compatibility; may be -1
static bool g_inited  = false;

static PekShortCb s_cb_short = nullptr;
static PekLongCb  s_cb_long  = nullptr;
static volatile bool s_flag_short = false;
static volatile bool s_flag_long  = false;

#ifndef PMIC_MEAS_PERIOD_MS
  #define PMIC_MEAS_PERIOD_MS 1500u   // 1.5s between real I2C reads
#endif

static uint32_t s_meas_period_ms = PMIC_MEAS_PERIOD_MS;

struct {
  float    vbus_v   = NAN;
  float    batt_v   = NAN;
  int      batt_pct = -1;
  bool     batt_present = false;
  uint32_t last_ms  = 0;
} s_meas_cache;

// ----------------- Measurement cache -----------------

static inline void ensure_meas_fresh() {
  if (!g_inited) return;
  uint32_t now = millis();
  if ((uint32_t)(now - s_meas_cache.last_ms) < s_meas_period_ms) return;

  I2C_LOCK(10);
  float vbus_mV = g_pmu.getVbusVoltage();
  float batt_mV = g_pmu.getBattVoltage();
  int   pct     = g_pmu.isBatteryConnect() ? (int)g_pmu.getBatteryPercent() : -1;
  I2C_UNLOCK();

  // Basic sanity: reject obviously bad reads
  bool bad =
      !isfinite(vbus_mV) ||
      !isfinite(batt_mV) ||
      (vbus_mV <= 0.0f && !g_pmu.isVbusIn());

  if (bad) {
    static uint32_t bad_count = 0;
    if ((bad_count++ % 10) == 0) {
      Serial.println("[PMIC] Bad measurement, keeping previous cache");
    }
    // Do NOT update last_ms, so we'll retry soon.
    return;
  }

  s_meas_cache.vbus_v       = vbus_mV / 1000.0f;
  s_meas_cache.batt_v       = batt_mV / 1000.0f;
  s_meas_cache.batt_pct     = pct;
  s_meas_cache.batt_present = (pct >= 0);
  s_meas_cache.last_ms      = now;
}

void set_measure_period_ms(uint32_t ms) { s_meas_period_ms = ms ? ms : 1; }


// ----------------- Power-key handler registration -----------------

void set_power_key_handlers(PekShortCb on_short, PekLongCb on_long) {
  s_cb_short = on_short;
  s_cb_long  = on_long;
}

bool consume_power_key_short() { bool f = s_flag_short; s_flag_short = false; return f; }
bool consume_power_key_long()  { bool f = s_flag_long;  s_flag_long  = false; return f; }

// ----------------- Internal helpers -----------------

static void configure_common()
{
  // USB input limits similar to Waveshare demo
  g_pmu.setVbusVoltageLimit(XPOWERS_AXP2101_VBUS_VOL_LIM_4V36);
  g_pmu.setVbusCurrentLimit(XPOWERS_AXP2101_VBUS_CUR_LIM_1500MA);

  // System shutdown threshold
  g_pmu.setSysPowerDownVoltage(2600);

  // ADCs we care about
  g_pmu.enableBattDetection();
  g_pmu.enableVbusVoltageMeasure();
  g_pmu.enableBattVoltageMeasure();
  g_pmu.enableSystemVoltageMeasure();

  g_pmu.setPowerKeyPressOffTime(XPOWERS_POWEROFF_4S);
  g_pmu.setPowerKeyPressOnTime(XPOWERS_POWERON_128MS);
}

// Keep it minimal/safe unless you turn on the demo profile
static void configure_rails_safe_minimal()
{
  // 3V3 rails commonly needed by LCD/peripherals
  g_pmu.setDC1Voltage(3300); g_pmu.enableDC1();
  g_pmu.setDC3Voltage(3300); g_pmu.enableDC3();

  // Optional 3V3 LDOs (uncomment if your TP/logic needs them)
  g_pmu.setALDO1Voltage(3300); g_pmu.enableALDO1();
  g_pmu.setALDO2Voltage(3300); g_pmu.enableALDO2();
}


// ----------------- Public API: begin -----------------

bool begin(TwoWire *wire, int sda, int scl, int irqPin)
{
  if (g_inited) return true;

  if (!wire) wire = &Wire;

  // I2C bus is already initialised via i2c_guard_init(); don't fight it.
  const bool ok = g_pmu.begin(*wire, AXP2101_SLAVE_ADDRESS, sda, scl);
  if (!ok) {
    Serial.println("[PMIC] AXP2101 not found on I2C");
    return false;
  }

  configure_common();

  g_irq = irqPin;

  // --- Enable AXP2101 IRQ sources even if we *don't* have a GPIO IRQ line ---
  // We will poll the IRQ status register over I2C instead of relying on the
  // chip's IRQ pin being wired to the MCU.
  g_pmu.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);

  uint32_t mask =
      XPOWERS_AXP2101_BAT_INSERT_IRQ  |
      XPOWERS_AXP2101_BAT_REMOVE_IRQ  |
      XPOWERS_AXP2101_VBUS_INSERT_IRQ |
      XPOWERS_AXP2101_VBUS_REMOVE_IRQ;

  // Add whichever key IRQ names your XPowersLib exposes:
  #ifdef XPOWERS_AXP2101_PKEY_SHORT_IRQ
    mask |= XPOWERS_AXP2101_PKEY_SHORT_IRQ;
  #endif
  #ifdef XPOWERS_AXP2101_PKEY_LONG_IRQ
    mask |= XPOWERS_AXP2101_PKEY_LONG_IRQ;
  #endif
  #ifdef XPOWERS_AXP2101_KEY_PRESS_IRQ
    mask |= XPOWERS_AXP2101_KEY_PRESS_IRQ;
  #endif
  #ifdef XPOWERS_AXP2101_KEY_LONG_IRQ
    mask |= XPOWERS_AXP2101_KEY_LONG_IRQ;
  #endif

  g_pmu.enableIRQ(mask);
  g_pmu.clearIrqStatus();

  if (g_irq >= 0) {
    // If the board *does* wire IRQ to a GPIO, we still set it as input,
    // but our logic works even when g_irq == -1.
    pinMode(g_irq, INPUT_PULLUP);
    Serial.printf("[PMIC] Using GPIO%d as AXP2101 IRQ (optional)\n", g_irq);
  } else {
    Serial.println("[PMIC] No IRQ GPIO; using polled IRQ mode");
  }

  g_inited = true;
  Serial.printf("[PMIC] AXP2101 online, chipID=0x%02X\n", g_pmu.getChipID());
  return true;
}


// ----------------- Rail control -----------------

void enable_display_power(bool on)
{
  if (!g_inited) return;
  if (on) {
    configure_rails_safe_minimal();
  } else {
    // Turn off in a safe order
    g_pmu.disableBLDO2();
    g_pmu.disableBLDO1();
    g_pmu.disableALDO4();
    g_pmu.disableALDO3();
    g_pmu.disableALDO2();
    g_pmu.disableALDO1();
    g_pmu.disableDC5();
    g_pmu.disableDC4();
    g_pmu.disableDC3();
    g_pmu.disableDC2();
    g_pmu.disableDC1();
  }
}

void set_backlight_rail(bool on)
{
  // NOP unless you power BL anode from a PMIC LDO you want to gate.
  (void)on;
}

void set_touch_rail(bool on)
{
  if (!g_inited) return;
  if (on) {
    g_pmu.setALDO2Voltage(3300);
    g_pmu.enableALDO2();
  } else {
    g_pmu.disableALDO2();
  }
}


// ----------------- Measurement getters -----------------

int battery_percent()
{
  if (!g_inited) return -1;
  ensure_meas_fresh();
  return s_meas_cache.batt_present ? s_meas_cache.batt_pct : -1;
}

float battery_voltage()
{
  if (!g_inited) return NAN;
  ensure_meas_fresh();
  return s_meas_cache.batt_v;
}

float vbus_voltage()
{
  if (!g_inited) return NAN;
  ensure_meas_fresh();
  return s_meas_cache.vbus_v;
}


// ----------------- Poll: NO-IRQ SHORT/LONG PRESS HANDLING -----------------

void poll()
{
  if (!g_inited) return;

  // Throttle IRQ polling to avoid saturating the I2C bus.
  static uint32_t last_ms = 0;
  uint32_t now = millis();
  if (last_ms != 0 && (now - last_ms) < 50) {
    return;   // ~20 Hz polling
  }
  last_ms = now;

  I2C_LOCK(10);

  // Read/latched IRQ status from the AXP2101.
  g_pmu.getIrqStatus();

  bool any_key = false;

  if (g_pmu.isPekeyShortPressIrq()) {
    any_key = true;
    s_flag_short = true;
    if (s_cb_short) {
      s_cb_short();
    } else {
      Serial.println("[PMIC] PEK short press (no handler registered)");
    }
  }

  if (g_pmu.isPekeyLongPressIrq()) {
    any_key = true;
    s_flag_long = true;
    if (s_cb_long) {
      s_cb_long();
    } else {
      Serial.println("[PMIC] PEK long press (no handler registered)");
    }
  }

  // Clear ALL IRQ flags we enabled earlier.
  g_pmu.clearIrqStatus();

  I2C_UNLOCK();

  // Optional debug
  if (any_key) {
    //Serial.println("[PMIC] Key IRQ handled via polled mode");
  }
}


// ----------------- Raw handle accessor -----------------

XPowersPMU &handle() { return g_pmu; }

} // namespace pmic
