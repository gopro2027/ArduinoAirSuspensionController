#include "axp2101_pmic.h"
#include "i2c_guard.h"

namespace pmic {

#define I2C_LOCK(ms)   if (!::i2c_lock(ms)) return
#define I2C_UNLOCK()   ::i2c_unlock()

static XPowersPMU g_pmu;
static int  g_irq     = -1;
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

// If you added the i2c_guard earlier, uncomment these 3 lines
#include "i2c_guard.h"
#define I2C_LOCK(ms)   if (!i2c_lock(ms)) return
#define I2C_UNLOCK()   i2c_unlock()

static inline void ensure_meas_fresh() {
  if (!g_inited) return;
  uint32_t now = millis();
  if ((uint32_t)(now - s_meas_cache.last_ms) < s_meas_period_ms) return;

  I2C_LOCK(10);
  float vbus_mV = g_pmu.getVbusVoltage();
  float batt_mV = g_pmu.getBattVoltage();
  int   pct     = g_pmu.isBatteryConnect() ? (int)g_pmu.getBatteryPercent() : -1;
  I2C_UNLOCK();

  s_meas_cache.vbus_v       = isfinite(vbus_mV) ? (vbus_mV / 1000.0f) : NAN;
  s_meas_cache.batt_v       = isfinite(batt_mV) ? (batt_mV / 1000.0f) : NAN;
  s_meas_cache.batt_pct     = pct;
  s_meas_cache.batt_present = (pct >= 0);
  s_meas_cache.last_ms      = now;
}

void set_measure_period_ms(uint32_t ms) { s_meas_period_ms = ms ? ms : 1; }


void set_power_key_handlers(PekShortCb on_short, PekLongCb on_long) {
  s_cb_short = on_short;
  s_cb_long  = on_long;
}

bool consume_power_key_short() { bool f = s_flag_short; s_flag_short = false; return f; }
bool consume_power_key_long()  { bool f = s_flag_long;  s_flag_long  = false; return f; }

// --- internal helpers ---
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


// --- public API ---
bool begin(TwoWire *wire, int sda, int scl, int irqPin)
{
  if (g_inited) return true;

  if (!wire) wire = &Wire;
  wire->begin(sda, scl, 400000); // 400 kHz fast I2C

  const bool ok = g_pmu.begin(*wire, AXP2101_SLAVE_ADDRESS, sda, scl);
  if (!ok) {
    Serial.println("[PMIC] AXP2101 not found on I2C");
    return false;
  }

  configure_common();

  g_irq = irqPin;
    if (g_irq >= 0) {
    pinMode(g_irq, INPUT_PULLUP);
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
    }


  g_inited = true;
  Serial.printf("[PMIC] AXP2101 online, chipID=0x%02X\n", g_pmu.getChipID());
  return true;
}

void enable_display_power(bool on)
{
  if (!g_inited) return;
  if (on) {
    #ifdef PMIC_PROFILE_WAVESHARE_DEMO
      configure_rails_waveshare_demo();
    #else
      configure_rails_safe_minimal();
    #endif
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

void poll() {
  if (!g_inited) return;

  if (g_irq >= 0) {
    if (digitalRead(g_irq) != LOW) return;   // IRQ-driven
  } else {
    static uint32_t last_ms = 0, now = millis();
    if (now - last_ms < 1000) return;        // throttle to 1s (reduce contention)
    last_ms = now;
  }

  if (!::i2c_lock(10)) return;
  g_pmu.getIrqStatus();
  if (g_pmu.isPekeyShortPressIrq()) { s_flag_short = true; if (s_cb_short) s_cb_short(); }
  if (g_pmu.isPekeyLongPressIrq())  { s_flag_long  = true; if (s_cb_long)  s_cb_long();  }
  g_pmu.clearIrqStatus();
  ::i2c_unlock();
}


XPowersPMU &handle() { return g_pmu; }

} // namespace pmic
