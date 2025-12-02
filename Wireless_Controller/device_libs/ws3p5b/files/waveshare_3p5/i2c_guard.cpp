// src/waveshare_3p5/i2c_guard.cpp
#include "i2c_guard.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

static SemaphoreHandle_t s_i2c_mutex = nullptr;
static TwoWire*          s_wire      = &Wire;

bool i2c_guard_init(TwoWire* w, uint32_t wire_timeout_ms) {
  if (w) s_wire = w;
  if (!s_i2c_mutex) s_i2c_mutex = xSemaphoreCreateMutex();
  if (!s_i2c_mutex) return false;
  s_wire->setTimeOut(wire_timeout_ms);
  return true;
}

bool i2c_lock(uint32_t timeout_ms) {
  if (!s_i2c_mutex) i2c_guard_init(s_wire);
  TickType_t to = (timeout_ms == (uint32_t)portMAX_DELAY)
                    ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
  return xSemaphoreTake(s_i2c_mutex, to) == pdTRUE;
}

void i2c_unlock() {
  if (s_i2c_mutex) xSemaphoreGive(s_i2c_mutex);
}
