#include "board_driver_util.h"
void set_brightness(float level)
{
    Set_Backlight(level * 100);
}

/**
 * stolen from esp32 smartdisplay
 */

#include <esp_lcd_panel_ops.h>

// Defines for adaptive brightness adjustment
#define BRIGHTNESS_SMOOTHING_MEASUREMENTS 100
#define BRIGHTNESS_DARK_ZONE 250

// Functions to be defined in the tft/touch driver
extern lv_display_t *lvgl_lcd_init();
extern lv_indev_t *lvgl_touch_init();

lv_display_t *display;

lv_indev_t *indev;
touch_calibration_data_t touch_calibration_data;
void (*driver_touch_read_cb)(lv_indev_t *indev, lv_indev_data_t *data);

void lvgl_display_resolution_changed_callback(lv_event_t *drv);

lv_timer_t *update_brightness_timer;

// See: https://www.maximintegrated.com/en/design/technical-documents/app-notes/5/5296.html
void lvgl_touch_calibration_transform(lv_indev_t *indev, lv_indev_data_t *data)
{
    log_v("indev:0x%08x, data:0x%08x", indev, data);

    // Call low level read from the driver
    driver_touch_read_cb(indev, data);
    // Check if transformation is required
    if (touch_calibration_data.valid && data->state == LV_INDEV_STATE_PRESSED)
    {
        log_d("lvgl_touch_calibration_transform: transformation applied");
        lv_point_t pt = {
            .x = roundf(data->point.x * touch_calibration_data.alphaX + data->point.y * touch_calibration_data.betaX + touch_calibration_data.deltaX),
            .y = roundf(data->point.x * touch_calibration_data.alphaY + data->point.y * touch_calibration_data.betaY + touch_calibration_data.deltaY)};
        log_d("Calibrate point (%d, %d) => (%d, %d)", data->point.x, data->point.y, pt.x, pt.y);
        data->point = (lv_point_t){pt.x, pt.y};
    }
}

touch_calibration_data_t smartdisplay_compute_touch_calibration(const lv_point_t screen[3], const lv_point_t touch[3])
{
    log_v("screen:0x%08x, touch:0x%08x", screen, touch);
    const float delta = ((touch[0].x - touch[2].x) * (touch[1].y - touch[2].y)) - ((touch[1].x - touch[2].x) * (touch[0].y - touch[2].y));
    touch_calibration_data_t touch_calibration_data = {
        .valid = true,
        .alphaX = (((screen[0].x - screen[2].x) * (touch[1].y - touch[2].y)) - ((screen[1].x - screen[2].x) * (touch[0].y - touch[2].y))) / delta,
        .betaX = (((touch[0].x - touch[2].x) * (screen[1].x - screen[2].x)) - ((touch[1].x - touch[2].x) * (screen[0].x - screen[2].x))) / delta,
        .deltaX = ((screen[0].x * ((touch[1].x * touch[2].y) - (touch[2].x * touch[1].y))) - (screen[1].x * ((touch[0].x * touch[2].y) - (touch[2].x * touch[0].y))) + (screen[2].x * ((touch[0].x * touch[1].y) - (touch[1].x * touch[0].y)))) / delta,
        .alphaY = (((screen[0].y - screen[2].y) * (touch[1].y - touch[2].y)) - ((screen[1].y - screen[2].y) * (touch[0].y - touch[2].y))) / delta,
        .betaY = (((touch[0].x - touch[2].x) * (screen[1].y - screen[2].y)) - ((touch[1].x - touch[2].x) * (screen[0].y - screen[2].y))) / delta,
        .deltaY = ((screen[0].y * (touch[1].x * touch[2].y - touch[2].x * touch[1].y)) - (screen[1].y * (touch[0].x * touch[2].y - touch[2].x * touch[0].y)) + (screen[2].y * (touch[0].x * touch[1].y - touch[1].x * touch[0].y))) / delta,
    };

    log_d("alphaX: %f, betaX: %f, deltaX: %f, alphaY: %f, betaY: %f, deltaY: %f", touch_calibration_data.alphaX, touch_calibration_data.betaX, touch_calibration_data.deltaX, touch_calibration_data.alphaY, touch_calibration_data.betaY, touch_calibration_data.deltaY);
    return touch_calibration_data;
};

void board_drivers_init()
{
    log_d("smartdisplay_init");

    LCD_Init();

    Lvgl_Init();
#ifdef BCKL_DELAY_MS
    vTaskDelay(pdMS_TO_TICKS(BCKL_DELAY_MS));
#endif
    // Setup backlight
    Backlight_Init();

    // Setup TFT display
    display = lvgl_lcd_init();

    // Register callback for hardware rotation
    lv_display_add_event_cb(display, lvgl_display_resolution_changed_callback, LV_EVENT_RESOLUTION_CHANGED, NULL);

    //  Clear screen
    lv_obj_clean(lv_scr_act());
    // Turn backlight on (50%)
    set_brightness(0.5f);

// If there is a touch controller defined
#ifdef BOARD_HAS_TOUCH
    // Setup touch
    indev = lvgl_touch_init();
    indev->disp = display;
    // Intercept callback
    driver_touch_read_cb = indev->read_cb;
    indev->read_cb = lvgl_touch_calibration_transform;
    lv_indev_enable(indev, true);
#endif
}

// Called when driver resolution is updated (including rotation)
// Top of the display is top left when connector is at the bottom
// The rotation values are relative to how you would rotate the physical display in the clockwise direction.
// So, LV_DISPLAY_ROTATION_90 means you rotate the hardware 90 degrees clockwise, and the display rotates 90 degrees counterclockwise to compensate.
void lvgl_display_resolution_changed_callback(lv_event_t *event)
{
    const esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)(display->user_data);
    switch (display->rotation)
    {
    case LV_DISPLAY_ROTATION_0:
        ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, DISPLAY_SWAP_XY));
        ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y));
        break;
    case LV_DISPLAY_ROTATION_90:
        ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, !DISPLAY_SWAP_XY));
        ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, DISPLAY_MIRROR_X, !DISPLAY_MIRROR_Y));
        break;
    case LV_DISPLAY_ROTATION_180:
        ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, DISPLAY_SWAP_XY));
        ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, !DISPLAY_MIRROR_X, !DISPLAY_MIRROR_Y));
        break;
    case LV_DISPLAY_ROTATION_270:
        ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, !DISPLAY_SWAP_XY));
        ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, !DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y));
        break;
    }
}
