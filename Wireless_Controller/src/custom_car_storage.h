/**
 * DISCLAIMER: This file was fully AI generated
 */

#ifndef custom_car_storage_h
#define custom_car_storage_h

#include "lvgl.h"

#ifndef SCREEN_MODE_CIRCLE

/** Mount SPIFFS and load custom car/wheel images from flash if present. Call once at boot. */
bool customCarStorageInit();

/** True when both /custom/car.bin and /custom/wheels.bin are valid. */
bool customCarHasImages();

/** Pointers for presets screen — runtime SPIFFS, compile-time custom, or device defaults. */
const lv_image_dsc_t *getPresetCarImage();
const lv_image_dsc_t *getPresetWheelsImage();

bool customCarSaveCar(const uint8_t *data, size_t len, uint16_t w, uint16_t h);
bool customCarSaveWheels(const uint8_t *data, size_t len, uint16_t w, uint16_t h);
void customCarClear();

#endif /* !SCREEN_MODE_CIRCLE */

#endif /* custom_car_storage_h */
