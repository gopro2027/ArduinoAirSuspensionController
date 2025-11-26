#include "rgb_tile.h"

lv_timer_t *rgb_tile_timer;

#define BG_COLOR_MAX 3
uint32_t bg_color_arr[BG_COLOR_MAX] = {0xff0000, 0x00ff00, 0x0000ff};
uint16_t bg_color_index = 1;

static lv_obj_t *obj_rgb_tile;
void lv_timer_show_color_tile_cb(lv_timer_t *timer)
{
    static bool is_red = true;

    lv_obj_set_style_bg_color(obj_rgb_tile, lv_color_hex(bg_color_arr[bg_color_index]), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj_rgb_tile, LV_OPA_COVER, LV_PART_MAIN);

    if (++bg_color_index >= BG_COLOR_MAX)
    {
        bg_color_index = 0;
    }
}
void rgb_tile_init(lv_obj_t *parent)
{
    obj_rgb_tile = parent;
    lv_obj_set_style_bg_color(obj_rgb_tile, lv_color_hex(bg_color_arr[0]), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj_rgb_tile, LV_OPA_COVER, LV_PART_MAIN);
    rgb_tile_timer = lv_timer_create(lv_timer_show_color_tile_cb, 1000, NULL);
}