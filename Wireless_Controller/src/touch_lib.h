#ifndef touch_lib_h
#define touch_lib_h

#include <Arduino.h>
#include <esp32_smartdisplay.h>

void setup_touchscreen_hook();

int32_t touchX();
int32_t touchY();
boolean isTouched();
boolean isJustPressed();
boolean isJustReleased();

#endif