#include "touch_lib.h"
#include "util.h"

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
        // Get raw touch coordinates (native portrait orientation)
        int32_t rawX = data->point.x;
        int32_t rawY = data->point.y;

        // Get rotation from saved preference since we use hardware MADCTL rotation
        // (LVGL rotation is always 0 because we change resolution instead)
        byte rotation = getscreenRotation();

        // Get actual LCD dimensions for proper touch transformation
        const int32_t lcdWidth = LCD_WIDTH;   // Native portrait width
        const int32_t lcdHeight = LCD_HEIGHT; // Native portrait height

        switch (rotation)
        {
        case 0: // Portrait
            touchX_ = rawX;
            touchY_ = rawY;
            break;
        case 1: // Landscape - 90 degrees CW
            // Raw touch is still in portrait coordinates
            // Transform to landscape screen coordinates
            touchX_ = rawY;
            touchY_ = (lcdWidth - 1) - rawX;  // Dynamic based on actual LCD width
            break;
        default:
            touchX_ = rawX;
            touchY_ = rawY;
            break;
        }

        // Also transform data->point for LVGL since we use hardware rotation
        // (LVGL won't auto-transform because lv_display_set_rotation is not used)
        data->point.x = touchX_;
        data->point.y = touchY_;
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
    driver_touch_read_cb__ = lv_indev_get_read_cb(indev);
    lv_indev_set_read_cb(indev, lvgl_touch_hook);
}