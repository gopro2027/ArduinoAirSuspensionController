#include "LVGL_Example.h"
#include "LVGL_Music.h"
#include <demos/lv_demos.h>
// #include <demos/music/lv_demo_music_main.h>
// #include <demos/music/lv_demo_music_list.h>


/**********************
 *      TYPEDEFS
 **********************/
typedef enum {
    DISP_SMALL,
    DISP_MEDIUM,
    DISP_LARGE,
} disp_size_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void Onboard_create(lv_obj_t * parent);
static void Music_create(lv_obj_t * parent);

static void ta_event_cb(lv_event_t * e);
void example1_increase_lvgl_tick(lv_timer_t * t);
/**********************
 *  STATIC VARIABLES
 **********************/
static disp_size_t disp_size;

static lv_obj_t * tv;
// static lv_obj_t * calendar;
lv_style_t style_text_muted;
lv_style_t style_title;
static lv_style_t style_icon;
static lv_style_t style_bullet;


static const lv_font_t * font_large;
static const lv_font_t * font_normal;

static lv_timer_t * auto_step_timer;
static lv_color_t original_screen_bg_color;

static lv_timer_t * meter2_timer;

lv_obj_t * SD_Size;
lv_obj_t * FlashSize;
lv_obj_t * BAT_Volts;
lv_obj_t * Board_angle;
lv_obj_t * RTC_Time;
lv_obj_t * Wireless_Scan;
lv_obj_t * Backlight_slider;



void Lvgl_Example1(void){

  disp_size = DISP_SMALL;                            

  font_large = LV_FONT_DEFAULT;                             
  font_normal = LV_FONT_DEFAULT;                         
  
  lv_coord_t tab_h;
  tab_h = 45;
  #if LV_FONT_MONTSERRAT_18
    font_large     = &lv_font_montserrat_18;
  #else
    LV_LOG_WARN("LV_FONT_MONTSERRAT_18 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
  #endif
  #if LV_FONT_MONTSERRAT_12
    font_normal    = &lv_font_montserrat_12;
  #else
    LV_LOG_WARN("LV_FONT_MONTSERRAT_12 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
  #endif
  
  lv_style_init(&style_text_muted);
  lv_style_set_text_opa(&style_text_muted, LV_OPA_90);

  lv_style_init(&style_title);
  lv_style_set_text_font(&style_title, font_large);

  lv_style_init(&style_icon);
  lv_style_set_text_color(&style_icon, lv_theme_get_color_primary(NULL));
  lv_style_set_text_font(&style_icon, font_large);

  lv_style_init(&style_bullet);
  lv_style_set_border_width(&style_bullet, 0);
  lv_style_set_radius(&style_bullet, LV_RADIUS_CIRCLE);

  tv = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, tab_h);

  lv_obj_set_style_text_font(lv_scr_act(), font_normal, 0);

  if(disp_size == DISP_LARGE) {
    lv_obj_t * tab_btns = lv_tabview_get_tab_btns(tv);
    lv_obj_set_style_pad_left(tab_btns, LV_HOR_RES / 2, 0);
    lv_obj_t * logo = lv_img_create(tab_btns);
    LV_IMG_DECLARE(img_lvgl_logo);
    lv_img_set_src(logo, &img_lvgl_logo);
    lv_obj_align(logo, LV_ALIGN_LEFT_MID, -LV_HOR_RES / 2 + 25, 0);

    lv_obj_t * label = lv_label_create(tab_btns);
    lv_obj_add_style(label, &style_title, 0);
    lv_label_set_text(label, "LVGL v8");
    lv_obj_align_to(label, logo, LV_ALIGN_OUT_RIGHT_TOP, 10, 0);

    label = lv_label_create(tab_btns);
    lv_label_set_text(label, "Widgets demo");
    lv_obj_add_style(label, &style_text_muted, 0);
    lv_obj_align_to(label, logo, LV_ALIGN_OUT_RIGHT_BOTTOM, 10, 0);
  }

  lv_obj_t * t1 = lv_tabview_add_tab(tv, "Onboard");
  lv_obj_t * t2 = lv_tabview_add_tab(tv, "music");

  Onboard_create(t1);
  Music_create(t2);
  
}

void Lvgl_Example1_close(void)
{
  /*Delete all animation*/
  lv_anim_del(NULL, NULL);

  lv_timer_del(meter2_timer);
  meter2_timer = NULL;

  lv_obj_clean(lv_scr_act());

  lv_style_reset(&style_text_muted);
  lv_style_reset(&style_title);
  lv_style_reset(&style_icon);
  lv_style_reset(&style_bullet);
}


/**********************
*   STATIC FUNCTIONS
**********************/

static void Onboard_create(lv_obj_t * parent)
{

  /*Create a panel*/
  lv_obj_t * panel1 = lv_obj_create(parent);
  lv_obj_set_height(panel1, LV_SIZE_CONTENT);

  lv_obj_t * panel1_title = lv_label_create(panel1);
  lv_label_set_text(panel1_title, "Onboard parameter");
  lv_obj_add_style(panel1_title, &style_title, 0);

  lv_obj_t * SD_label = lv_label_create(panel1);
  lv_label_set_text(SD_label, "SD Card");
  lv_obj_add_style(SD_label, &style_text_muted, 0);

  SD_Size = lv_textarea_create(panel1);
  lv_textarea_set_one_line(SD_Size, true);
  lv_textarea_set_placeholder_text(SD_Size, "SD Size");
  lv_obj_add_event_cb(SD_Size, ta_event_cb, LV_EVENT_ALL, NULL);

  lv_obj_t * Flash_label = lv_label_create(panel1);
  lv_label_set_text(Flash_label, "Flash Size");
  lv_obj_add_style(Flash_label, &style_text_muted, 0);

  FlashSize = lv_textarea_create(panel1);
  lv_textarea_set_one_line(FlashSize, true);
  lv_textarea_set_placeholder_text(FlashSize, "Flash Size");
  lv_obj_add_event_cb(FlashSize, ta_event_cb, LV_EVENT_ALL, NULL);

  lv_obj_t * BAT_label = lv_label_create(panel1);
  lv_label_set_text(BAT_label, "Battery Voltage");
  lv_obj_add_style(BAT_label, &style_text_muted, 0);

  BAT_Volts = lv_textarea_create(panel1);
  lv_textarea_set_one_line(BAT_Volts, true);
  lv_textarea_set_placeholder_text(BAT_Volts, "BAT Volts");
  lv_obj_add_event_cb(BAT_Volts, ta_event_cb, LV_EVENT_ALL, NULL);

  lv_obj_t * angle_label = lv_label_create(panel1);
  lv_label_set_text(angle_label, "Angular deflection");
  lv_obj_add_style(angle_label, &style_text_muted, 0);

  Board_angle = lv_textarea_create(panel1);
  lv_textarea_set_one_line(Board_angle, true);
  lv_textarea_set_placeholder_text(Board_angle, "Board angle");
  lv_obj_add_event_cb(Board_angle, ta_event_cb, LV_EVENT_ALL, NULL);

  lv_obj_t * Time_label = lv_label_create(panel1);
  lv_label_set_text(Time_label, "RTC Time");
  lv_obj_add_style(Time_label, &style_text_muted, 0);

  RTC_Time = lv_textarea_create(panel1);
  lv_textarea_set_one_line(RTC_Time, true);
  lv_textarea_set_placeholder_text(RTC_Time, "Display time");
  lv_obj_add_event_cb(RTC_Time, ta_event_cb, LV_EVENT_ALL, NULL);


  lv_obj_t * Wireless_label = lv_label_create(panel1);
  lv_label_set_text(Wireless_label, "Wireless scan");
  lv_obj_add_style(Wireless_label, &style_text_muted, 0);

  Wireless_Scan = lv_textarea_create(panel1);
  lv_textarea_set_one_line(Wireless_Scan, true);
  lv_textarea_set_placeholder_text(Wireless_Scan, "Wireless number");
  lv_obj_add_event_cb(Wireless_Scan, ta_event_cb, LV_EVENT_ALL, NULL);

  lv_obj_t * Backlight_label = lv_label_create(panel1);
  lv_label_set_text(Backlight_label, "Backlight brightness");
  lv_obj_add_style(Backlight_label, &style_text_muted, 0);

  Backlight_slider = lv_slider_create(panel1);                                 
  lv_obj_add_flag(Backlight_slider, LV_OBJ_FLAG_CLICKABLE);    
  lv_obj_set_size(Backlight_slider, 200, 35);              
  lv_obj_set_style_radius(Backlight_slider, 3, LV_PART_KNOB);               // Adjust the value for more or less rounding                                            
  lv_obj_set_style_bg_opa(Backlight_slider, LV_OPA_TRANSP, LV_PART_KNOB);                               
  // lv_obj_set_style_pad_all(Backlight_slider, 0, LV_PART_KNOB);                                            
  lv_obj_set_style_bg_color(Backlight_slider, lv_color_hex(0xAAAAAA), LV_PART_KNOB);               
  lv_obj_set_style_bg_color(Backlight_slider, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR);             
  lv_obj_set_style_outline_width(Backlight_slider, 2, LV_PART_INDICATOR);  
  lv_obj_set_style_outline_color(Backlight_slider, lv_color_hex(0xD3D3D3), LV_PART_INDICATOR);      
  lv_slider_set_range(Backlight_slider, 5, Backlight_MAX);              
  lv_slider_set_value(Backlight_slider, LCD_Backlight, LV_ANIM_ON);  
  lv_obj_add_event_cb(Backlight_slider, Backlight_adjustment_event_cb, LV_EVENT_VALUE_CHANGED, NULL);


  static lv_coord_t grid_main_col_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
  static lv_coord_t grid_main_row_dsc[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
  lv_obj_set_grid_dsc_array(parent, grid_main_col_dsc, grid_main_row_dsc);


  /*Create the top panel*/
  static lv_coord_t grid_1_col_dsc[] = {LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
  static lv_coord_t grid_1_row_dsc[] = {
    LV_GRID_CONTENT,  /*Title*/
    5,                /*Separator*/
    LV_GRID_CONTENT,  /*Box title*/
    40,               /*Box*/
    LV_GRID_CONTENT,  /*Box title*/
    40,               /*Box*/
    LV_GRID_CONTENT,  /*Box title*/
    40,               /*Box*/
    LV_GRID_CONTENT,  /*Box title*/
    40,               /*Box*/
    LV_GRID_CONTENT,  /*Box title*/
    40,               /*Box*/
    LV_GRID_CONTENT,  /*Box title*/
    40,               /*Box*/
    LV_GRID_CONTENT,  /*Box title*/
    40,               /*Box*/
    LV_GRID_TEMPLATE_LAST               
  };

  lv_obj_set_grid_cell(panel1, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_START, 0, 1);
  lv_obj_set_grid_dsc_array(panel1, grid_1_col_dsc, grid_1_row_dsc);
  lv_obj_set_grid_cell(panel1_title, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);
  lv_obj_set_grid_cell(SD_label, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_START, 2, 1);
  lv_obj_set_grid_cell(SD_Size, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_CENTER, 3, 1);
  lv_obj_set_grid_cell(Flash_label, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_START, 4, 1);
  lv_obj_set_grid_cell(FlashSize, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_CENTER, 5, 1);
  lv_obj_set_grid_cell(BAT_label, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_START, 6, 1);
  lv_obj_set_grid_cell(BAT_Volts, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_CENTER, 7, 1);
  lv_obj_set_grid_cell(angle_label, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_START, 8, 1);
  lv_obj_set_grid_cell(Board_angle, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_CENTER, 9, 1);
  lv_obj_set_grid_cell(Time_label, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_START, 10, 1);
  lv_obj_set_grid_cell(RTC_Time, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_CENTER, 11, 1);
  lv_obj_set_grid_cell(Wireless_label, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_START, 12, 1);
  lv_obj_set_grid_cell(Wireless_Scan, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_CENTER, 13, 1);
  lv_obj_set_grid_cell(Backlight_label, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_START, 14, 1);
  lv_obj_set_grid_cell(Backlight_slider, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_CENTER, 15, 1);

  auto_step_timer = lv_timer_create(example1_increase_lvgl_tick, 100, NULL);
}

void example1_increase_lvgl_tick(lv_timer_t * t)
{
  char buf[100]; 
  
  snprintf(buf, sizeof(buf), "%ld MB\r\n", SDCard_Size);
  lv_textarea_set_placeholder_text(SD_Size, buf);
  snprintf(buf, sizeof(buf), "%ld MB\r\n", Flash_Size);
  lv_textarea_set_placeholder_text(FlashSize, buf);
  snprintf(buf, sizeof(buf), "%.2f V\r\n", BAT_analogVolts);
  lv_textarea_set_placeholder_text(BAT_Volts, buf);
  snprintf(buf, sizeof(buf), "X:%.2f  Y:%.2f  Z:%.2f\r\n", Accel.x, Accel.y, Accel.z);
  lv_textarea_set_placeholder_text(Board_angle, buf);
  snprintf(buf, sizeof(buf), "%d.%d.%d   %d:%d:%d\r\n",datetime.year,datetime.month,datetime.day,datetime.hour,datetime.minute,datetime.second);
  lv_textarea_set_placeholder_text(RTC_Time, buf);
  if(Scan_finish)
    // snprintf(buf, sizeof(buf), "WIFI: %d    BLE: %d    ..Scan Finish.\r\n",WIFI_NUM,BLE_NUM);
    snprintf(buf, sizeof(buf), "WIFI: %d     ..Scan Finish.\r\n",WIFI_NUM);
  else
    snprintf(buf, sizeof(buf), "WIFI: %d  \r\n",WIFI_NUM);
    // snprintf(buf, sizeof(buf), "WIFI: %d    BLE: %d\r\n",WIFI_NUM,BLE_NUM);
  lv_textarea_set_placeholder_text(Wireless_Scan, buf);
  lv_slider_set_value(Backlight_slider, LCD_Backlight, LV_ANIM_ON); 
  LVGL_Backlight_adjustment(LCD_Backlight);
}
static void Music_create(lv_obj_t * parent)
{
  original_screen_bg_color = lv_obj_get_style_bg_color(parent, 0);
  lv_obj_set_style_bg_color(parent, lv_color_hex(0x343247), 0);

  _lv_demo_music_main_create(parent);
}

void Backlight_adjustment_event_cb(lv_event_t * e) {
  uint8_t Backlight = lv_slider_get_value(lv_event_get_target(e));  
  if (Backlight <= Backlight_MAX)  {
    lv_slider_set_value(Backlight_slider, Backlight, LV_ANIM_ON); 
    LCD_Backlight = Backlight;
    LVGL_Backlight_adjustment(Backlight);
  }
  else
    printf("Volume out of range: %d\n", Backlight);

}


static void ta_event_cb(lv_event_t * e)
{
}

void LVGL_Backlight_adjustment(uint8_t Backlight) {
  Set_Backlight(Backlight);                                 
}




