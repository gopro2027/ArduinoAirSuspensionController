// esp_lcd_touch_axs15231b.cpp - OPTIMIZED VERSION
// Improvements:
// - Throttled I2C reads to reduce bus contention during scrolling
// - Higher I2C clock speed (800kHz) for faster transactions
// - Cached touch data to minimize redundant reads
// - Better error handling and recovery

#include "esp_lcd_touch_axs15231b.h"
#include "i2c_guard.h"

// ---------- Configuration ----------

// Touch read throttling: only read hardware every N milliseconds
// This dramatically reduces I2C bus contention during LVGL scrolling
#ifndef TOUCH_READ_INTERVAL_MS
  #define TOUCH_READ_INTERVAL_MS 10  // 10ms = 100 Hz max touch polling
#endif

// I2C clock speed for touch controller
#ifndef TOUCH_I2C_FREQ_HZ
  #define TOUCH_I2C_FREQ_HZ 800000  // 800kHz (try 600000 if unstable)
#endif

// I2C timeout for touch transactions (shorter than PMIC)
#ifndef TOUCH_I2C_TIMEOUT_MS
  #define TOUCH_I2C_TIMEOUT_MS 6
#endif

// ---------- Module state ----------

static TwoWire *g_touch_i2c = nullptr;

static uint16_t g_width = 0;
static uint16_t g_height = 0;
static uint16_t g_rotation = 0;

static touch_data_t g_touch_data = {0};
static touch_data_t g_touch_cache = {0};  // Last valid touch data

static uint32_t s_last_touch_read_ms = 0;
static uint32_t s_read_count = 0;
static uint32_t s_skip_count = 0;

// Error tracking for diagnostics
static uint32_t s_i2c_errors = 0;
static uint32_t s_timeout_errors = 0;

// ---------- Helper: I2C transaction ----------

static bool touch_i2c_write_read(uint8_t addr, uint8_t *wb, uint32_t wl,
                                 uint8_t *rb, uint32_t rl)
{
  // Try to acquire I2C lock with short timeout (touch should be quick)
  if (!::i2c_lock(TOUCH_I2C_TIMEOUT_MS)) {
    s_timeout_errors++;
    return false;
  }

  bool success = true;

  // Write command
  g_touch_i2c->beginTransmission(addr);
  g_touch_i2c->write(wb, wl);
  if (g_touch_i2c->endTransmission() != 0) {
    s_i2c_errors++;
    success = false;
    goto cleanup;
  }

  // Read response
  g_touch_i2c->requestFrom(addr, rl);
  if (g_touch_i2c->available() != rl) {
    s_i2c_errors++;
    success = false;
    goto cleanup;
  }
  g_touch_i2c->readBytes(rb, rl);

cleanup:
  ::i2c_unlock();
  return success;
}

// ---------- Initialization ----------

void bsp_touch_init(TwoWire *touch_i2c, int tp_rst, uint16_t rotation, 
                    uint16_t width, uint16_t height)
{
  g_touch_i2c = touch_i2c;
  g_width = width;
  g_height = height;
  g_rotation = rotation;

  // OPTIMIZED: Set higher I2C clock for faster transactions
  if (g_touch_i2c) {
    g_touch_i2c->setClock(TOUCH_I2C_FREQ_HZ);
    Serial.printf("[TOUCH] I2C clock set to %u Hz\n", TOUCH_I2C_FREQ_HZ);
  }

  // Hardware reset if pin provided
  if (tp_rst != -1) {
    pinMode(tp_rst, OUTPUT);
    digitalWrite(tp_rst, LOW);
    delay(200);
    digitalWrite(tp_rst, HIGH);
    delay(300);
    Serial.println("[TOUCH] Hardware reset complete");
  }

  // Initialize cache
  memset(&g_touch_data, 0, sizeof(g_touch_data));
  memset(&g_touch_cache, 0, sizeof(g_touch_cache));
  s_last_touch_read_ms = 0;

  Serial.printf("[TOUCH] Initialized: %ux%u, rotation=%u, throttle=%ums\n",
                width, height, rotation, TOUCH_READ_INTERVAL_MS);
}

// ---------- Touch reading with throttling ----------

void bsp_touch_read(void)
{
  if (!g_touch_i2c) {
    return;
  }

  // OPTIMIZED: Throttle hardware reads to reduce I2C contention
  uint32_t now = millis();
  uint32_t elapsed = now - s_last_touch_read_ms;

  if (elapsed < TOUCH_READ_INTERVAL_MS) {
    // Too soon - reuse cached data
    s_skip_count++;
    return;
  }

  s_last_touch_read_ms = now;
  s_read_count++;

  // Prepare touch read command
  uint8_t data[14] = {0};
  uint8_t cmd[11] = {0xb5, 0xab, 0xa5, 0x5a, 0x00, 0x00, 0x00, 
                     0x0e, 0x00, 0x00, 0x00};

  // Execute I2C transaction
  if (!touch_i2c_write_read(AXS5106L_ADDR, cmd, 11, data, 14)) {
    // I2C error - keep using last valid touch data
    // Don't spam errors, just track them
    return;
  }

  // Validate response data
  if (data[1] == 0 || data[2] == 0 || data[3] < 2 || data[5] < 2) {
    // Invalid data - clear touch
    g_touch_data.touch_num = 0;
    return;
  }

  if (data[0] == 0xff || data[1] > MAX_TOUCH_MAX_POINTS) {
    // Invalid touch count
    g_touch_data.touch_num = 0;
    return;
  }

  // Parse touch points
  g_touch_data.touch_num = data[1];

  for (uint8_t i = 0; i < g_touch_data.touch_num; i++) {
    g_touch_data.coords[i].x = ((data[6 * i + 2] & 0x0F) << 8) | data[6 * i + 3];
    g_touch_data.coords[i].y = ((data[6 * i + 4] & 0x0F) << 8) | data[6 * i + 5];
  }

  // Update cache with valid data
  if (g_touch_data.touch_num > 0) {
    memcpy(&g_touch_cache, &g_touch_data, sizeof(touch_data_t));
  }
}

// ---------- Get transformed coordinates ----------

bool bsp_touch_get_coordinates(touch_data_t *touch_data)
{
  if (touch_data == NULL) {
    return false;
  }

  // Return cached data if no current touch
  if (g_touch_data.touch_num == 0) {
    return false;
  }

  // Transform coordinates based on rotation
  for (int i = 0; i < g_touch_data.touch_num; i++) {
    switch (g_rotation) {
      case 1:
        touch_data->coords[i].y = g_height - 1 - g_touch_data.coords[i].x;
        touch_data->coords[i].x = g_touch_data.coords[i].y;
        break;
      
      case 2:
        touch_data->coords[i].x = g_width - 1 - g_touch_data.coords[i].x;
        touch_data->coords[i].y = g_height - 1 - g_touch_data.coords[i].y;
        break;
      
      case 3:
        touch_data->coords[i].y = g_touch_data.coords[i].x;
        touch_data->coords[i].x = g_width - 1 - g_touch_data.coords[i].y;
        break;
      
      default: // case 0
        touch_data->coords[i].x = g_touch_data.coords[i].x;
        touch_data->coords[i].y = g_touch_data.coords[i].y;
        break;
    }
  }

  touch_data->touch_num = g_touch_data.touch_num;
  return true;
}

// ---------- Diagnostic functions ----------

void bsp_touch_print_stats(void)
{
  Serial.println("[TOUCH] Statistics:");
  Serial.printf("  Hardware reads: %lu\n", s_read_count);
  Serial.printf("  Throttled skips: %lu (%.1f%% reduction)\n", 
                s_skip_count, 
                100.0f * s_skip_count / (s_read_count + s_skip_count));
  Serial.printf("  I2C errors: %lu\n", s_i2c_errors);
  Serial.printf("  Timeout errors: %lu\n", s_timeout_errors);
  Serial.printf("  Current touch points: %u\n", g_touch_data.touch_num);
}

void bsp_touch_reset_stats(void)
{
  s_read_count = 0;
  s_skip_count = 0;
  s_i2c_errors = 0;
  s_timeout_errors = 0;
}