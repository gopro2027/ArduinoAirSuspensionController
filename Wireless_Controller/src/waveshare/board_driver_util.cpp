#include "board_driver_util.h"


void set_brightness(float level)
{
    Set_Backlight(level * 100);
}

/**
 * stolen from esp32 smartdisplay
 */

// Defines for adaptive brightness adjustment
#define BRIGHTNESS_SMOOTHING_MEASUREMENTS 100
#define BRIGHTNESS_DARK_ZONE 250

// Functions to be defined in the tft/touch driver

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
            .x = (int32_t)roundf(data->point.x * touch_calibration_data.alphaX + data->point.y * touch_calibration_data.betaX + touch_calibration_data.deltaX),
            .y = (int32_t)roundf(data->point.x * touch_calibration_data.alphaY + data->point.y * touch_calibration_data.betaY + touch_calibration_data.deltaY)};
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

LV_IMG_DECLARE(oasman_splash);
void board_drivers_init()
{
    log_i("Setting up board drivers");

    // Setup backlight
    Backlight_Init();

    // various i2c features (notably, i2c is required for touch)
    I2C_Init();

    // screen setup (lcd and touch)
    LCD_Init();

    // lvgl setup
    touch_and_screen tas = Lvgl_Init();
    display = tas.screen;
    indev = tas.touch;

    lv_obj_t *splashScr = lv_screen_active();
    lv_obj_set_style_bg_color(splashScr, lv_color_black(), LV_PART_MAIN);
    lv_obj_t *splashscreen = lv_image_create(splashScr);
    lv_image_set_src(splashscreen, &oasman_splash);
    lv_obj_set_align(splashscreen, LV_ALIGN_CENTER);
    lv_timer_handler(); // Force refresh immediately
    delay(50);// just a small delay to give the screen time to finish rendering the logo, otherwise we get a few artifacts
    set_brightness(1); // blind them with the oasman logo

#ifdef BCKL_DELAY_MS
    vTaskDelay(pdMS_TO_TICKS(BCKL_DELAY_MS));
#endif

    // Register callback for hardware rotation
    lv_display_add_event_cb(display, lvgl_display_resolution_changed_callback, LV_EVENT_RESOLUTION_CHANGED, NULL);

    // Delete the splash screen completely before applying rotation
    // First create a new blank screen to be the active one
    lv_obj_t *blankScr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(blankScr, lv_color_black(), LV_PART_MAIN);
    lv_screen_load(blankScr);
    lv_timer_handler();

    // Now delete the old splash screen
    lv_obj_del(splashScr);

    // Apply saved screen rotation using hardware MADCTL rotation
    // Hardware rotation is more efficient - LCD controller rotates pixels directly
    extern byte getscreenRotation();
    byte rotation = getscreenRotation();

    // Set hardware rotation via MADCTL register
    LCD_SetRotation(rotation);

    // Update LVGL resolution to match rotated display
    // Use actual LCD dimensions (works for all display sizes)
    // LCD_WIDTH and LCD_HEIGHT are defined in board JSON as compile-time constants
    if (rotation == 1) {
        // Landscape mode - swap width/height
        lv_display_set_resolution(display, LCD_HEIGHT, LCD_WIDTH);
    } else {
        // Portrait mode - native resolution
        lv_display_set_resolution(display, LCD_WIDTH, LCD_HEIGHT);
    }

    // Create a fresh screen with correct rotated dimensions
    lv_obj_t *tempScr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(tempScr, lv_color_black(), LV_PART_MAIN);
    lv_screen_load(tempScr);
    lv_timer_handler();
    lv_obj_del(blankScr);

    // Setup touch
    // indev = lvgl_touch_init();
    lv_indev_set_display(indev, display);
    // Intercept callback
    driver_touch_read_cb = lv_indev_get_read_cb(indev);
    lv_indev_set_read_cb(indev, lvgl_touch_calibration_transform);
    lv_indev_enable(indev, true);
}

void lvgl_display_resolution_changed_callback(lv_event_t *event)
{
    log_i("Display resolution changed - rotation applied");
}
