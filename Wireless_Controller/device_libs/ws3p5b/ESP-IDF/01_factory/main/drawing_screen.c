#include "drawing_screen.h"
#include "lvgl_ui.h"
#include <stdio.h>

#define CANVAS_WIDTH 320
#define CANVAS_HEIGHT 480

#define POINT_WIDTH 5
#define POINT_HEIGHT 5

#define LV_COLOR_RED lv_color_make(0xFF, 0x00, 0x00)
#define LV_COLOR_BLUE lv_color_make(0x00, 0x00, 0xFF)
#define LV_COLOR_WHITE lv_color_make(0xFF, 0xFF, 0xFF)

lv_obj_t *canvas;
bool canvas_exit = false;
void draw_point(lv_obj_t *canvas, lv_coord_t x, lv_coord_t y)
{
    lv_color_t palette_color = {.full = 0};
    lv_coord_t x_begin = x - POINT_WIDTH / 2;
    lv_coord_t x_end = x + POINT_WIDTH / 2;
    lv_coord_t y_begin = y - POINT_WIDTH / 2;
    lv_coord_t y_end = y + POINT_WIDTH / 2;
    if (x_begin < 0)
        x_begin = 0;
    if (x_end > CANVAS_WIDTH - 1)
        x_end = CANVAS_WIDTH - 1;

    if (y_begin < 0)
        y_begin = 0;
    else if (y_end > CANVAS_HEIGHT - 1)
        y_end = CANVAS_HEIGHT - 1;
    for (int i = x_begin; i <= x_end; i++)
    {
        for (int j = y_begin; j <= y_end; j++)
        {
            lv_canvas_set_px_color(canvas, i, j, palette_color);
        }
    }
}

lv_obj_t *ui_drawing_screen = NULL;

// Event callback function
void event_handler(lv_event_t *e)
{
    // static uint32_t count = 0;
    // Get the coordinates of the touch point
    lv_point_t point;
    lv_obj_t *canvas = lv_event_get_target(e);
    // Get the coordinates of the touch point
    lv_indev_get_point(lv_indev_get_act(), &point);
    draw_point(canvas, point.x, point.y);
    // Output coordinates
    // printf("Clicked at: x = %d, y = %d\n", point.x, point.y);
}

void btn_clear_event_handler(lv_event_t *e)
{
    // Get the passed parameter
    lv_obj_t *canvas = lv_event_get_user_data(e);
    lv_color_t c1 = {.full = 1};
    // Set background
    lv_canvas_fill_bg(canvas, c1, LV_OPA_COVER);
}

void btn_exit_event_handler(lv_event_t *e)
{
    // Wait for touchscreen release
    lv_indev_wait_release(lv_indev_get_act());
    canvas_exit = true;
    // lvgl_ui_init();
    // lv_obj_del(canvas);

    // Switch to main interface
    // _ui_screen_change(&ui_main_screen, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, &main_screen_init);
}

void drawing_screen_init(void)
{
    // ui_drawing_screen = lv_scr_act();

    /*Create a buffer for the canvas*/
    // Create a buffer
    static lv_color_t cbuf[LV_CANVAS_BUF_SIZE_INDEXED_1BIT(CANVAS_WIDTH, CANVAS_HEIGHT)];

    /*Create a canvas and initialize its palette*/
    // Create a canvas
    canvas = lv_canvas_create(lv_scr_act());
    // lv_obj_set_size(ui_drawing_screen, lv_pct(100), lv_pct(100)); // Set container width to screen width, set height to 60
    // Set canvas buffer
    lv_canvas_set_buffer(canvas, cbuf, CANVAS_WIDTH, CANVAS_HEIGHT, LV_IMG_CF_INDEXED_1BIT);
    // Set canvas position
    lv_obj_align(canvas, LV_ALIGN_CENTER, 0, 0);
    // Set palette color
    lv_canvas_set_palette(canvas, 0, LV_COLOR_RED);
    // Set palette color
    lv_canvas_set_palette(canvas, 1, LV_COLOR_WHITE);

    // Add clickable flag
    lv_obj_add_flag(canvas, LV_OBJ_FLAG_CLICKABLE);
    // Add press event callback
    lv_obj_add_event_cb(canvas, event_handler, LV_EVENT_PRESSED, NULL);
    // Add long press event callback
    lv_obj_add_event_cb(canvas, event_handler, LV_EVENT_PRESSING, NULL);

    /*Create colors with the indices of the palette*/
    lv_color_t bg_color = {.full = 1};
    // Set background
    lv_canvas_fill_bg(canvas, bg_color, LV_OPA_COVER);

    // Create a container to hold buttons
    lv_obj_t *cont = lv_obj_create(canvas);

    // Set container width to 120, set height to 55
    lv_obj_set_size(cont, 100, 50);
    // Align container to top center of the screen, with vertical offset
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 20);
    // Set button background opacity
    lv_obj_set_style_bg_opa(cont, LV_OPA_20, 0);
    // Set horizontal distribution
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    // Horizontal centering
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    // Set padding
    lv_obj_set_style_pad_all(cont, 0, 0);

    static lv_style_t style; // Create style
    // Initialize style
    lv_style_init(&style);

    // Set style radius
    lv_style_set_radius(&style, 5);
    // Set style background opacity
    lv_style_set_opa(&style, LV_OPA_COVER);
    // Set style background color
    lv_style_set_bg_color(&style, lv_palette_lighten(LV_PALETTE_GREEN, 1));

    // Create clear button
    lv_obj_t *btn = lv_btn_create(cont);
    lv_obj_set_style_bg_opa(btn, LV_OPA_80, 0); // Set button background opacity
    lv_obj_set_size(btn, 40, 40);
    // lv_obj_set_width(btn, 80);
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_REFRESH); // Set button label
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0); // 20-point font
    lv_obj_add_style(btn, &style, 0);            // Add style to object
    lv_obj_add_event_cb(btn, btn_clear_event_handler, LV_EVENT_PRESSED, canvas);

    // Create second button
    btn = lv_btn_create(cont);
    lv_obj_set_style_bg_opa(btn, LV_OPA_80, 0); // Set button background opacity
    // lv_obj_set_width(btn, 80);
    lv_obj_set_size(btn, 40, 40);
    label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_CLOSE); // Set button label
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0); // 20-point font
    lv_obj_add_style(btn, &style, 0);          // Add style to object
    lv_obj_add_event_cb(btn, btn_exit_event_handler, LV_EVENT_PRESSED, NULL);
}