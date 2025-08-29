#include "tasks.h"

// Small, safe priorities (0..24). Higher number = higher priority.
constexpr UBaseType_t PRIO_LOW    = tskIDLE_PRIORITY + 1; // 1
constexpr UBaseType_t PRIO_NORMAL = tskIDLE_PRIORITY + 2; // 2

// (Optional) Clamp helper if you ever pass a computed prio.
static inline UBaseType_t clampPrio(UBaseType_t p) {
  const UBaseType_t maxp = configMAX_PRIORITIES - 1; // 24 on ESP32/S3
  return (p > maxp) ? maxp : p;
}

void task_bluetooth(void *parameters)
{
    vTaskDelay(pdMS_TO_TICKS(200)); // legacy settle

    Serial.println(F("Bluetooth Rest Service Beginning"));

    ble_setup();
    vTaskDelay(pdMS_TO_TICKS(10));
    for (;;) {
        ble_loop();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void setup_tasks()
{
    // Bluetooth Task
    xTaskCreate(
        task_bluetooth,
        "Bluetooth",
        512 * 6,        // stack depth in *words* (≈24 KB here)
        nullptr,
        PRIO_NORMAL,    // was 1000 -> crash; now 2
        nullptr
    );
}
