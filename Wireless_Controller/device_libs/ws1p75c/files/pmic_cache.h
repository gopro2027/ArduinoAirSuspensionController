#pragma once
#include <Arduino.h>

struct PmicCache {
  float    vbus_v;        // NAN if unknown
  float    batt_v;        // NAN if unknown
  int      batt_pct;      // -1 if unknown
  bool     vbus_present;  // heuristics based
  bool     batt_present;  // from XPowers battery detect
  uint32_t updated_ms;    // millis() of last update
};

// Start a low-priority task that refreshes the cache every period_ms.
void pmic_cache_start(uint32_t period_ms = 1000, int task_core = 0);

// Read a snapshot (thread-safe copy).
PmicCache pmic_cache_get();
