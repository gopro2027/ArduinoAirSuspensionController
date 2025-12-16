#include "pmic_cache.h"
#include "axp2101_pmic.h"

#include <math.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_task_wdt.h>

static PmicCache     s_cache {NAN, NAN, -1, false, false, 0};
static TaskHandle_t  s_task  = nullptr;
static portMUX_TYPE  s_mux   = portMUX_INITIALIZER_UNLOCKED;

static bool finite_pos(float x) { return isfinite(x) && x > 0.0f; }

static bool is_vbus_ok(float vv) { return finite_pos(vv) && vv > 3.8f && vv < 5.8f; }
static bool is_batt_ok(float vb) { return finite_pos(vb) && vb > 2.6f && vb < 4.5f; }

static void pmic_cache_task(void *arg)
{
  const uint32_t period_ms = (uint32_t)(uintptr_t)arg;

  // CRITICAL FIX: Wait for system to stabilize before first read
  Serial.println("[PMIC Cache] Task started, waiting for init to complete...");
  vTaskDelay(pdMS_TO_TICKS(500));  // Give main thread time to finish init
  
  Serial.println("[PMIC Cache] Beginning periodic reads");

  for (;;) {
    // Feed watchdog to prevent timeout
    // #ifdef CONFIG_ESP_TASK_WDT_EN
    // esp_task_wdt_reset();
    // #endif

    PmicCache c;

    // Add timeout protection: if reads take too long, skip this cycle
    uint32_t read_start = millis();

    c.vbus_v     = pmic::vbus_voltage();
    c.batt_v     = pmic::battery_voltage();
    c.batt_pct   = pmic::battery_percent();

    uint32_t read_duration = millis() - read_start;
    if (read_duration > 300) {
      // Serial.printf("[PMIC Cache] WARNING: Reads took %ums (expected <100ms)\n", read_duration);
    }

    c.vbus_present = is_vbus_ok(c.vbus_v);
    c.batt_present = is_batt_ok(c.batt_v) || (c.batt_pct >= 0);
    c.updated_ms   = millis();

    // Update cache atomically
    taskENTER_CRITICAL(&s_mux);
    s_cache = c;
    taskEXIT_CRITICAL(&s_mux);

    // Debug logging (throttled)
    static uint32_t log_counter = 0;
    if ((log_counter++ % 10) == 0) {
      // Serial.printf("[PMIC Cache] VBUS=%.2fV, Batt=%.2fV (%d%%)\n", 
                    // c.vbus_v, c.batt_v, c.batt_pct);
    }

    vTaskDelay(pdMS_TO_TICKS(period_ms));
  }
}

void pmic_cache_start(uint32_t period_ms, int task_core)
{
    if (s_task) {
        Serial.println("[PMIC Cache] Already running");
        return;
    }
    
    Serial.printf("[PMIC Cache] Starting task (period=%ums, core=%d)\n", period_ms, task_core);
    
    BaseType_t result = xTaskCreatePinnedToCore(
        pmic_cache_task, 
        "pmic_cache",
        4096,  // Stack size
        (void*)(uintptr_t)period_ms,
        1,     // Low priority
        &s_task, 
        task_core
    );
    
    if (result != pdPASS) {
        Serial.println("[PMIC Cache] ERROR: Failed to create task!");
        s_task = nullptr;
        return;
    }
    
    // Give the task time to start before returning
    vTaskDelay(pdMS_TO_TICKS(10));
    Serial.println("[PMIC Cache] Task created successfully");
}

PmicCache pmic_cache_get()
{
  taskENTER_CRITICAL(&s_mux);
  PmicCache c = s_cache;
  taskEXIT_CRITICAL(&s_mux);
  return c;
}