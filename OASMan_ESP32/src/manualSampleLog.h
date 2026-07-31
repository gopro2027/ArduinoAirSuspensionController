#ifndef manualSampleLog_h
#define manualSampleLog_h

#include <Arduino.h>

// AI training-sample logging for MANUAL valve moves (the BLE valveControlBittset path only).
// A manual hold is a single continuous open that reaches whatever pressure the user targets - including
// the high goals the automatic preset path never reaches - so it provides the goal-reaching coverage the
// model needs. Pure data collection: no valve or prediction behavior changes. See OASMan_ESP32/AI_TRAINING.md.

// Called from the BLE valve-control edge loop (ble.cpp) on each solenoid open/close transition.
//   solenoidIndex  = SOLENOID_INDEX (bit index 0..7); even = IN/up/fill, odd = OUT/down/exhaust.
//   opening        = true on a 0->1 (open) edge, false on a 1->0 (close) edge.
//   incomingBittset= the new desired valve bitset, used to count same-direction "others flowing".
// Uses only cached reads (no ADC) so it is safe to call on the BTstack callback thread.
void manualSampleOnValveEdge(int solenoidIndex, bool opening, unsigned int incomingBittset);

// Called from Wheel::loop() each tick. Performs the deferred settle-read + recordLearnSample for this
// wheel's two solenoids once >= MANUAL_SETTLE_MS have elapsed since their close (fresh ADC read + SPIFFS
// write happen here, on the owning wheel task - never on the BLE callback).
void manualSampleServiceWheel(byte wheelNum);

#endif
