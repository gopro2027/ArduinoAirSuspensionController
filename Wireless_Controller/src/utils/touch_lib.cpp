#include "touch_lib.h"

int32_t touchX_ = 0;
int32_t touchY_ = 0;
boolean touchPressed = false;
boolean lastTouched = false;

lv_indev_read_cb_t driver_touch_read_cb__;
void lvgl_touch_hook(lv_indev_t *indev, lv_indev_data_t *data)
{

    // Call low level read from the driver
    driver_touch_read_cb__(indev, data);
    if (data->state == LV_INDEV_STATE_PRESSED)
    {
        touchX_ = data->point.x;
        touchY_ = data->point.y;
    }
    lastTouched = touchPressed;
    touchPressed = data->state;
}

int32_t touchX()
{
    return touchX_;
}
int32_t touchY()
{
    return touchY_;
}

boolean isTouched()
{
    return touchPressed;
}

boolean isJustPressed()
{
    if (touchPressed && lastTouched == false)
    {
        return true;
    }
    return false;
}

boolean isJustReleased()
{
    if (touchPressed == false && lastTouched)
    {
        return true;
    }
    return false;
}

// effectively disables 'just pressed' and 'just released'
// reasoning: previously this code was inside each of those functions, but it makes more sense to just do it at the end of the frame instead so that we can call 'just' multiple times per frame and not limit it to 1 call per frame due to it resetting ittself
void resetTouchInputFrame()
{
    lastTouched = touchPressed;
}

extern lv_indev_t *indev; // esp32_smartdisplay.c
void setup_touchscreen_hook()
{
    // assume touch already setup in smartdisplay_init

    // Intercept callback (hook)
    driver_touch_read_cb__ = indev->read_cb;
    indev->read_cb = lvgl_touch_hook;
}