#ifndef screen_h
#define screen_h

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "bitmaps.h"
#include "user_defines.h"
#include "airSuspensionUtil.h"
#include "tasks/taskUtil.h"

extern Adafruit_SSD1306 display;

void drawPSIReadings();
void drawsplashscreen();

#endif
