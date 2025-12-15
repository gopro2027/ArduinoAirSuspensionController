// src/waveshare_3p5/i2c_guard.h
#pragma once
#include <Arduino.h>
#include <Wire.h>

// Global guard API
bool  i2c_guard_init(TwoWire* w = &Wire, uint32_t wire_timeout_ms = 20);
bool  i2c_lock(uint32_t timeout_ms = 10);
void  i2c_unlock();
