#include "pmic_cache.h"
#include "waveshare_3p5/axp2101_pmic.h"

#include <math.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static PmicCache     s_cache {NAN, NAN, -1, false, false, 0};
static TaskHandle_t  s_task  = nullptr;
static portMUX_TYPE  s_mux   = portMUX_INITIALIZER_UNLOCKED;

static bool finite_pos(float x) { return isfinite(x) && x > 0.0f; }

// Simple presence heuristics to avoid false-positives
static bool is_vbus_ok(float vv) { return finite_pos(vv) && vv > 3.8f && vv < 5.8f; }
static bool is_batt_ok(float vb) { return finite_pos(vb) && vb > 2.6f && vb < 4.5f; }

static void pmic_cache_task(void *arg)
{
  const uint32_t period_ms = (uint32_t)(uintptr_t)arg;

  for (;;) {
    PmicCache c;

    // Raw reads from PMIC (I²C happens here, NOT in LVGL/touch threads)
    c.vbus_v     = pmic::vbus_voltage();
    c.batt_v     = pmic::battery_voltage();
    c.batt_pct   = pmic::battery_percent();

    c.vbus_present = is_vbus_ok(c.vbus_v);
    c.batt_present = is_batt_ok(c.batt_v) || (c.batt_pct >= 0);
    c.updated_ms   = millis();

    taskENTER_CRITICAL(&s_mux);
    s_cache = c;
    taskEXIT_CRITICAL(&s_mux);

    vTaskDelay(pdMS_TO_TICKS(period_ms));
  }
}

void pmic_cache_start(uint32_t period_ms, int task_core)
{
  if (s_task) return; // already running
  xTaskCreatePinnedToCore(
      pmic_cache_task, "pmic_cache",
      2048, (void*)(uintptr_t)period_ms,
      1 /*low prio*/, &s_task, task_core);
}

PmicCache pmic_cache_get()
{
  taskENTER_CRITICAL(&s_mux);
  PmicCache c = s_cache;
  taskEXIT_CRITICAL(&s_mux);
  return c;
}
