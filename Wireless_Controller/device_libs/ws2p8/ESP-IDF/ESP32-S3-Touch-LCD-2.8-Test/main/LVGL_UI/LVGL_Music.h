#pragma once
/*********************
 *      INCLUDES
 *********************/
// #include <demos/music/lv_demo_music_list.h>
#include <demos/music/lv_demo_music.h>
#include <demos/lv_demos.h>
#include <lvgl.h>

// #include "assets/spectrum_1.h"
// #include "assets/spectrum_2.h"
// #include "assets/spectrum_3.h"

#include "SD_MMC.h"
#include "PCM5101.h"

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

extern uint16_t ACTIVE_TRACK_CNT;   
/*
 * Callback adapter function to convert parameter types to avoid compile-time
 * warning. 
 */
void _img_set_zoom_anim_cb(void * obj, int32_t zoom);
/*
 * Callback adapter function to convert parameter types to avoid compile-time
 * warning.
 */
void _obj_set_x_anim_cb(void * obj, int32_t x);
lv_obj_t * _lv_demo_music_main_create(lv_obj_t * parent);
void _lv_demo_music_main_close(void);
void _lv_demo_music_album_next(bool next);
void _lv_demo_music_play(uint32_t id);
void _lv_demo_music_resume(void);
void _lv_demo_music_pause(void);

/**********************
 *   STATIC FUNCTIONS
 **********************/
lv_obj_t * create_List_box(lv_obj_t * parent);
lv_obj_t * add_list_btn(lv_obj_t * parent, uint32_t track_id);
void _lv_demo_music_list_btn_check(uint32_t track_id, bool state);
void btn_click_event_cb(lv_event_t * e);

lv_obj_t * create_cont(lv_obj_t * parent);
void create_wave_images(lv_obj_t * parent);
lv_obj_t * create_title_box(lv_obj_t * parent);
lv_obj_t * create_icon_box(lv_obj_t * parent);
lv_obj_t * create_spectrum_obj(lv_obj_t * parent);
lv_obj_t * create_ctrl_box(lv_obj_t * parent);
lv_obj_t * create_handle(lv_obj_t * parent);
void track_load(uint32_t id);
lv_obj_t * album_img_create(lv_obj_t * parent);
void album_gesture_event_cb(lv_event_t * e);
void play_event_click_cb(lv_event_t * e);
void prev_click_event_cb(lv_event_t * e);
void next_click_event_cb(lv_event_t * e);
void volume_event_cb(lv_event_t * e);

void album_fade_anim_cb(void * var, int32_t v);
void timer_cb(lv_timer_t * t);

void LVGL_Search_Music(); 
void LVGL_Resume_Music();
void LVGL_Pause_Music();  
void LVGL_Play_Music(uint32_t ID);  
void LVGL_volume_adjustment(uint8_t Volume);
