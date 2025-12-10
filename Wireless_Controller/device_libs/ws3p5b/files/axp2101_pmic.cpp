#include "axp2101_pmic.h"
#include "i2c_guard.h"

namespace pmic {

#define I2C_LOCK(ms)   if (!i2c_lock(ms)) return
#define I2C_UNLOCK()   i2c_unlock()

static XPowersPMU g_pmu;
static int  g_irq     = -1;
static bool g_inited  = false;

static PekShortCb s_cb_short = nullptr;
static PekLongCb  s_cb_long  = nullptr;
static volatile bool s_flag_short = false;
static volatile bool s_flag_long  = false;

#ifndef PMIC_MEAS_PERIOD_MS
  #define PMIC_MEAS_PERIOD_MS 1500u
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

  // CRITICAL FIX: Increase timeout to 200ms to allow for ADC conversions
  // Also add proper error handling
  if (!i2c_lock(200)) {
    Serial.println("[PMIC] Failed to acquire I2C lock for measurements");
    return;  // Keep old cache values
  }

  // Wrap in try-catch equivalent for safety
  float vbus_mV = NAN;
  float batt_mV = NAN;
  int   pct     = -1;
  bool  i2c_success = true;

  // Read with timeout awareness
  uint32_t start = millis();
  vbus_mV = g_pmu.getVbusVoltage();
  if ((millis() - start) > 100) {
    Serial.println("[PMIC] VBUS read took too long");
    i2c_success = false;
  }

  if (i2c_success) {
    batt_mV = g_pmu.getBattVoltage();
    if (g_pmu.isBatteryConnect()) {
      pct = (int)g_pmu.getBatteryPercent();
    }
  }

  i2c_unlock();

  // Basic sanity: reject obviously bad reads
  bool bad =
      !isfinite(vbus_mV) ||
      !isfinite(batt_mV) ||
      (vbus_mV <= 0.0f && !g_pmu.isVbusIn()) ||
      !i2c_success;

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
  g_pmu.setVbusVoltageLimit(XPOWERS_AXP2101_VBUS_VOL_LIM_4V36);
  g_pmu.setVbusCurrentLimit(XPOWERS_AXP2101_VBUS_CUR_LIM_1500MA);
  g_pmu.setSysPowerDownVoltage(2600);

  g_pmu.enableBattDetection();
  g_pmu.enableVbusVoltageMeasure();
  g_pmu.enableBattVoltageMeasure();
  g_pmu.enableSystemVoltageMeasure();

  g_pmu.setPowerKeyPressOffTime(XPOWERS_POWEROFF_4S);
  g_pmu.setPowerKeyPressOnTime(XPOWERS_POWERON_128MS);
}

static void configure_rails_safe_minimal()
{
  g_pmu.setDC1Voltage(3300); g_pmu.enableDC1();
  g_pmu.setDC3Voltage(3300); g_pmu.enableDC3();
  g_pmu.setALDO1Voltage(3300); g_pmu.enableALDO1();
  g_pmu.setALDO2Voltage(3300); g_pmu.enableALDO2();
}

// ----------------- Public API: begin -----------------

bool begin(TwoWire *wire, int sda, int scl, int irqPin)
{
  if (g_inited) return true;

  if (!wire) wire = &Wire;

  // CRITICAL: Add longer timeout for initial I2C probe
  if (!i2c_lock(500)) {
    Serial.println("[PMIC] Failed to acquire I2C lock for initialization");
    return false;
  }

  const bool ok = g_pmu.begin(*wire, AXP2101_SLAVE_ADDRESS, sda, scl);
  
  i2c_unlock();

  if (!ok) {
    Serial.println("[PMIC] AXP2101 not found on I2C");
    return false;
  }

  // Lock again for configuration
  if (!i2c_lock(300)) {
    Serial.println("[PMIC] Failed to acquire I2C lock for configuration");
    return false;
  }

  configure_common();

  g_irq = irqPin;

  g_pmu.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);

  uint32_t mask =
      XPOWERS_AXP2101_BAT_INSERT_IRQ  |
      XPOWERS_AXP2101_BAT_REMOVE_IRQ  |
      XPOWERS_AXP2101_VBUS_INSERT_IRQ |
      XPOWERS_AXP2101_VBUS_REMOVE_IRQ;

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

  i2c_unlock();

  if (g_irq >= 0) {
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
  
  if (!i2c_lock(200)) {
    Serial.println("[PMIC] Failed to lock I2C for display power");
    return;
  }

  if (on) {
    configure_rails_safe_minimal();
  } else {
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
  
  i2c_unlock();
}

void set_backlight_rail(bool on) { (void)on; }

void set_touch_rail(bool on)
{
  if (!g_inited) return;
  
  if (!i2c_lock(100)) return;
  
  if (on) {
    g_pmu.setALDO2Voltage(3300);
    g_pmu.enableALDO2();
  } else {
    g_pmu.disableALDO2();
  }
  
  i2c_unlock();
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

  static uint32_t last_ms = 0;
  uint32_t now = millis();
  if (last_ms != 0 && (now - last_ms) < 50) {
    return;
  }
  last_ms = now;

  // Use shorter timeout for polling to avoid blocking
  if (!i2c_lock(50)) {
    return;  // Skip this poll cycle if bus is busy
  }

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

  g_pmu.clearIrqStatus();

  i2c_unlock();
}

XPowersPMU &handle() { return g_pmu; }

} // namespace pmic