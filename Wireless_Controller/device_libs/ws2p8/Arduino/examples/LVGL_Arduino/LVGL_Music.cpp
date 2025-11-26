#include "LVGL_Music.h"
/*********************
 *      DEFINES
 *********************/
#define INTRO_TIME          2000
#define BAR_COLOR1          lv_color_hex(0xe9dbfc)
#define BAR_COLOR2          lv_color_hex(0x6f8af6)
#define BAR_COLOR3          lv_color_hex(0xffffff)
#if LV_DEMO_MUSIC_LARGE
    #define BAR_COLOR1_STOP     160
    #define BAR_COLOR2_STOP     200
#else
    #define BAR_COLOR1_STOP     80
    #define BAR_COLOR2_STOP     100
#endif
#define BAR_COLOR3_STOP     (2 * LV_HOR_RES / 3)
#define BAR_CNT             20
#define DEG_STEP            (180/BAR_CNT)
#define BAND_CNT            4
#define BAR_PER_BAND_CNT    (BAR_CNT / BAND_CNT)


/**********************
 *  STATIC VARIABLES
 **********************/
lv_style_t music_style;
lv_style_t parts_style;
 
lv_obj_t * panel1;
lv_obj_t * panel2;
static lv_obj_t * main_cont;
static lv_obj_t * spectrum_obj;
static lv_obj_t * title_label;
static lv_obj_t * time_obj;
static lv_obj_t * album_img_obj;
static lv_obj_t * slider_obj;
static uint32_t spectrum_i = 0;
static uint32_t spectrum_i_pause = 0;
static uint32_t bar_ofs = 0;
static uint32_t bar_rot = 0;
static uint32_t time_act;
static lv_timer_t  * sec_counter_timer; 
static lv_timer_t  * spectrum_timer; 
static const lv_font_t * font_small;
static const lv_font_t * font_large;
static bool Playing_Flag;                                     
static uint32_t track_id;
static bool start_anim;
static lv_coord_t start_anim_values[40];
static lv_obj_t * play_obj;
static const uint16_t (* spectrum)[4];
static uint32_t spectrum_len;
static const uint16_t rnd_array[30] = {994, 285, 553, 11, 792, 707, 966, 641, 852, 827, 44, 352, 146, 581, 490, 80, 729, 58, 695, 940, 724, 561, 124, 653, 27, 292, 557, 506, 382, 199};



char SD_Name[100][100] ;    
char File_Name[100][100] ;     
char Audio_Name[100] ;         
uint16_t ACTIVE_TRACK_CNT;     
uint32_t Audio_duration;      
uint32_t Audio_duration_A;        
uint32_t Audio_Elapsed;         
uint16_t Audio_energy;         

static lv_obj_t * list;
static lv_style_t style_btn_round;
static lv_style_t style_btn_pr;
static lv_style_t style_btn_play;
static lv_style_t style_btn_stop;
static lv_style_t style_title;
static bool first_Flag = false;
LV_IMG_DECLARE(img_lv_demo_music_btn_list_play);
LV_IMG_DECLARE(img_lv_demo_music_btn_list_pause);

/*
 * Callback adapter function to convert parameter types to avoid compile-time
 * warning.
 */
void _img_set_zoom_anim_cb(void * obj, int32_t zoom)
{
    lv_img_set_zoom((lv_obj_t *)obj, (uint16_t)zoom);
}

/*
 * Callback adapter function to convert parameter types to avoid compile-time
 * warning.
 */
void _obj_set_x_anim_cb(void * obj, int32_t x)
{
    lv_obj_set_x((lv_obj_t *)obj, (lv_coord_t)x);
}

lv_obj_t * _lv_demo_music_main_create(lv_obj_t * parent)
{

  LVGL_Search_Music();   
  if(ACTIVE_TRACK_CNT) {                                  
    lv_style_init(&music_style);
    lv_style_set_text_font(&music_style, font_large);

    font_small = &lv_font_montserrat_12;
    font_large = &lv_font_montserrat_16;

  // 1
    panel1 = lv_obj_create(parent);
    lv_obj_set_height(panel1, LV_SIZE_CONTENT);
    
    lv_obj_t * cont = create_cont(panel1);
    create_wave_images(cont);
    spectrum_obj = create_spectrum_obj(panel1);
    lv_obj_add_style(spectrum_obj, &music_style, 0);

    lv_obj_t * title_box = create_title_box(panel1);
    lv_obj_add_style(title_box, &music_style, 0);
    lv_obj_t * ctrl_box = create_ctrl_box(panel1);
    lv_obj_add_style(ctrl_box, &music_style, 0);
  
    static lv_coord_t grid_main_col_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t grid_main_row_dsc[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(parent, grid_main_col_dsc, grid_main_row_dsc);
  /*Create the top panel*/
    static lv_coord_t grid_1_col_dsc[] = {LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t grid_1_row_dsc[] = {
      LV_GRID_CONTENT,      /*title_box*/
      LV_GRID_CONTENT,      /*cont*/
      170,                   /*spectrum_obj*/
      LV_GRID_CONTENT,      /*ctrl_box*/
      LV_GRID_CONTENT,      /*handle_box*/
      LV_GRID_CONTENT,      /*Button2*/
      LV_GRID_TEMPLATE_LAST
      };
    lv_obj_set_grid_cell(panel1, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_START, 0, 1);  
    lv_obj_set_grid_dsc_array(panel1, grid_1_col_dsc, grid_1_row_dsc);        
    lv_obj_set_grid_cell(title_box    , LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_set_grid_cell(cont         , LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_CENTER, 1, 1);
    lv_obj_set_grid_cell(spectrum_obj , LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_CENTER, 2, 1);
    lv_obj_set_grid_cell(ctrl_box , LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_CENTER, 3, 1);

  // 2
    panel2 = lv_obj_create(parent);
    lv_obj_set_height(panel2, LV_SIZE_CONTENT);

    lv_obj_t * list_box = create_List_box(panel2);
    
    lv_obj_set_size(list_box, LV_SIZE_CONTENT, LV_SIZE_CONTENT);               
    // lv_obj_add_style(list_box, &music_style, 0);

    static lv_coord_t grid_2_col_dsc[] = {LV_GRID_FR(1),LV_GRID_FR(1),  LV_GRID_TEMPLATE_LAST};
    static lv_coord_t grid_2_row_dsc[] = {
        LV_GRID_CONTENT,        /*list_box*/
        LV_GRID_TEMPLATE_LAST   
    };
    lv_obj_set_grid_cell(panel2, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_START, 1, 1);       
    lv_obj_set_grid_dsc_array(panel2, grid_2_col_dsc, grid_2_row_dsc);    
    lv_obj_set_grid_cell(list_box , LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_CENTER, 0, 1);


    sec_counter_timer = lv_timer_create(timer_cb, 1000, NULL);
    lv_timer_pause(sec_counter_timer);

    spectrum_timer = lv_timer_create(spectrum_timer_cb, 100, NULL);
    lv_timer_pause(spectrum_timer);

    lv_obj_fade_in(title_box, 500, INTRO_TIME - 1000);
    lv_obj_fade_in(ctrl_box, 500, INTRO_TIME - 1000);
    lv_obj_fade_in(album_img_obj, 300, INTRO_TIME - 1000);
    lv_obj_fade_in(spectrum_obj, 0, INTRO_TIME - 1000);
  }
  else{ 
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, "No MP3 file found in SD card!");
    // lv_obj_set_size(label, LV_PCT(100), LV_PCT(100));

    lv_obj_set_size(label, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);  
  }
  return main_cont;
}


/************************************************************************************************************************************
 *   create_title_box                 
************************************************************************************************************************************/
lv_obj_t * create_title_box(lv_obj_t * parent)
{
  /*Create the titles*/
  lv_obj_t * cont = lv_obj_create(parent);
  lv_obj_remove_style_all(cont);                                                                  
  lv_obj_set_height(cont, LV_SIZE_CONTENT);                                                       
  lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);                                             
  lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);   

  title_label = lv_label_create(cont);                                                            
  lv_obj_set_style_text_font(title_label, font_large, 0);                                                        
  lv_obj_set_style_text_color(title_label, lv_color_hex(0x504d6d), 0);                            
  lv_label_set_text(title_label, Audio_Name);                                                    
  lv_obj_set_height(title_label, lv_font_get_line_height(font_large) );                         
  return cont;
}
/************************************************************************************************************************************
 *  create_title_box END            *  create_title_box END             *  create_title_box END             *  create_title_box END
************************************************************************************************************************************/

/************************************************************************************************************************************
 *  create_cont                      *  create_cont                      *  create_cont                      *  create_cont                       
************************************************************************************************************************************/
lv_obj_t * create_cont(lv_obj_t * parent)
{
  /*  */
  /*A transparent container in which the player section will be scrolled*/
  main_cont = lv_obj_create(parent);
  lv_obj_clear_flag(main_cont, LV_OBJ_FLAG_CLICKABLE);                                                
  lv_obj_clear_flag(main_cont, LV_OBJ_FLAG_SCROLL_ELASTIC);                                          
  lv_obj_remove_style_all(main_cont);                            /*Make it transparent*/              
  lv_obj_set_size(main_cont, lv_pct(100), lv_pct(100));                                               
  lv_obj_set_scroll_snap_y(main_cont, LV_SCROLL_SNAP_CENTER);    /*Snap the children to the center*/  

  /*Create a container for the player*/
  lv_obj_t * player = lv_obj_create(main_cont);
  lv_obj_set_y(player, - LV_DEMO_MUSIC_HANDLE_SIZE);
  lv_obj_set_size(player, LV_HOR_RES, 2 * LV_VER_RES + LV_DEMO_MUSIC_HANDLE_SIZE * 2);

  lv_obj_set_style_bg_color(player, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_border_width(player, 0, 0);
  lv_obj_set_style_pad_all(player, 0, 0);
  lv_obj_set_scroll_dir(player, LV_DIR_VER);

  /* Transparent placeholders below the player container
    * It is used only to snap it to center.*/
  lv_obj_t * placeholder1 = lv_obj_create(main_cont);
  lv_obj_remove_style_all(placeholder1);
  lv_obj_clear_flag(placeholder1, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_t * placeholder2 = lv_obj_create(main_cont);
  lv_obj_remove_style_all(placeholder2);
  lv_obj_clear_flag(placeholder2, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_t * placeholder3 = lv_obj_create(main_cont);
  lv_obj_remove_style_all(placeholder3);
  lv_obj_clear_flag(placeholder3, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_size(placeholder1, lv_pct(100), LV_VER_RES);
  lv_obj_set_y(placeholder1, 0);
  lv_obj_set_size(placeholder2, lv_pct(100), LV_VER_RES);
  lv_obj_set_y(placeholder2, LV_VER_RES);
  lv_obj_set_size(placeholder3, lv_pct(100),  LV_VER_RES - 2 * LV_DEMO_MUSIC_HANDLE_SIZE);
  lv_obj_set_y(placeholder3, 2 * LV_VER_RES + LV_DEMO_MUSIC_HANDLE_SIZE);

  lv_obj_update_layout(main_cont);

  return player;
}

void create_wave_images(lv_obj_t * parent)
{
  LV_IMG_DECLARE(img_lv_demo_music_wave_top);                                               
  LV_IMG_DECLARE(img_lv_demo_music_wave_bottom);                                            
  lv_obj_t * wave_top = lv_img_create(parent);
  lv_img_set_src(wave_top, &img_lv_demo_music_wave_top);                                    
  lv_obj_set_width(wave_top, LV_HOR_RES);                                                   
  lv_obj_align(wave_top, LV_ALIGN_TOP_MID, 0, 0);                                           
  lv_obj_add_flag(wave_top, LV_OBJ_FLAG_IGNORE_LAYOUT);                                   

  lv_obj_t * wave_bottom = lv_img_create(parent);                                           
  lv_img_set_src(wave_bottom, &img_lv_demo_music_wave_bottom);                             
  lv_obj_set_width(wave_bottom, LV_HOR_RES);                                                
  lv_obj_align(wave_bottom, LV_ALIGN_BOTTOM_MID, 0, 0);                               
  lv_obj_add_flag(wave_bottom, LV_OBJ_FLAG_IGNORE_LAYOUT);                               

  LV_IMG_DECLARE(img_lv_demo_music_corner_left);                                            
  LV_IMG_DECLARE(img_lv_demo_music_corner_right);
  lv_obj_t * wave_corner = lv_img_create(parent);
  lv_img_set_src(wave_corner, &img_lv_demo_music_corner_left);                             
  lv_obj_align(wave_corner, LV_ALIGN_BOTTOM_LEFT, -LV_HOR_RES / 6, 0);                      
  lv_obj_add_flag(wave_corner, LV_OBJ_FLAG_IGNORE_LAYOUT);                                  

  wave_corner = lv_img_create(parent);
  lv_img_set_src(wave_corner, &img_lv_demo_music_corner_right);                             
  lv_obj_align(wave_corner, LV_ALIGN_BOTTOM_RIGHT, LV_HOR_RES / 6, 0);                      
  lv_obj_add_flag(wave_corner, LV_OBJ_FLAG_IGNORE_LAYOUT);                                         
}

/************************************************************************************************************************************
 *   create_cont END                  *   create_cont END                  *   create_cont END                  *   create_cont END                 
************************************************************************************************************************************/

/************************************************************************************************************************************
 *  spectrum                    *  spectrum                     *  spectrum                     *  spectrum                    
************************************************************************************************************************************/
lv_obj_t * create_spectrum_obj(lv_obj_t * parent)
{
  /*Create the spectrum visualizer*/
  lv_obj_t * obj = lv_obj_create(parent);             
  lv_obj_remove_style_all(obj);                                                                   
  lv_obj_set_height(obj, 250);                                                                  
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);                         
  lv_obj_add_event_cb(obj, spectrum_draw_event_cb, LV_EVENT_ALL, NULL);                           
  lv_obj_refresh_ext_draw_size(obj);                                                              
  album_img_obj = album_img_create(obj);                                                         
  return obj;
}
int32_t get_cos(int32_t deg, int32_t a)
{
  int32_t r = (lv_trigo_cos(deg) * a);                    
  r += LV_TRIGO_SIN_MAX / 2;
  return r >> LV_TRIGO_SHIFT;
}
int32_t get_sin(int32_t deg, int32_t a)
{
  int32_t r = lv_trigo_sin(deg) * a;                      
  r += LV_TRIGO_SIN_MAX / 2;
  return r >> LV_TRIGO_SHIFT;
}
void spectrum_draw_event_cb(lv_event_t * e)
{
  lv_event_code_t code = lv_event_get_code(e);                              
  if(code == LV_EVENT_REFR_EXT_DRAW_SIZE)                                 
    lv_event_set_ext_draw_size(e, LV_VER_RES);
  else if(code == LV_EVENT_COVER_CHECK)
    lv_event_set_cover_res(e, LV_COVER_RES_NOT_COVER);
  else if(code == LV_EVENT_DRAW_POST) {
    lv_obj_t * obj = lv_event_get_target(e);                                
    lv_draw_ctx_t * draw_ctx = lv_event_get_draw_ctx(e);                    
    lv_opa_t opa = lv_obj_get_style_opa_recursive(obj, LV_PART_MAIN);       
    if(opa < LV_OPA_MIN) return;                                            

    lv_point_t poly[4];
    lv_point_t center;
    center.x = obj->coords.x1 + lv_obj_get_width(obj) / 2;
    center.y = obj->coords.y1 + lv_obj_get_height(obj) / 2;

    lv_draw_rect_dsc_t draw_dsc;
    lv_draw_rect_dsc_init(&draw_dsc);                                       
    draw_dsc.bg_opa = LV_OPA_COVER;

    uint16_t r[64];
    uint32_t i;

    lv_coord_t min_a = 5;                                                   
    lv_coord_t r_in = 77;                                                  
    r_in = (r_in * lv_img_get_zoom(album_img_obj)) >> 8;                    
    for(i = 0; i < BAR_CNT; i++) r[i] = r_in + min_a;           
    uint32_t s;
    for(s = 0; s < 4; s++) {
      uint32_t f;
      uint32_t band_w = 0;    /*Real number of bars in this band.*/ 
      switch(s) {                                                           
        case 0:
          band_w = 20;
          break;
        case 1:
          band_w = 8;
          break;
        case 2:
          band_w = 4;
          break;
        case 3:
          band_w = 2;
          break;
      }
      uint32_t Audio_spectrum = Audio_energy/1300;
      uint32_t random_number =(uint32_t) (rand() % (Audio_spectrum + 1));
      
      /* Add "side bars" with cosine characteristic.*/
      for(f = 0; f < band_w; f++) {                                       
        uint32_t ampl_main = random_number ;
        int32_t ampl_mod = get_cos(f * 360 / band_w + 180, 180) + 180;
        int32_t t = BAR_PER_BAND_CNT * s - band_w / 2 + f;
        if(t < 0) t = BAR_CNT + t;
        if(t >= BAR_CNT) t = t - BAR_CNT;
        r[t] += (ampl_main * ampl_mod) >> 9;
      }
    }

    uint32_t amax = 20;                                                  
    int32_t animv = Audio_energy/2000;;                                   
    if(animv > amax) animv = amax;
    for(i = 0; i < BAR_CNT; i++) {                                                
      uint32_t deg_space = 1;
      uint32_t deg = i * DEG_STEP + 90;
      uint32_t j = (i + bar_rot + rnd_array[bar_ofs % 10]) % BAR_CNT;
      uint32_t k = (i + bar_rot + rnd_array[(bar_ofs + 1) % 10]) % BAR_CNT;

      uint32_t v = (r[k] * animv + r[j] * (amax - animv)) / amax;
      
      if(v < BAR_COLOR1_STOP) draw_dsc.bg_color = BAR_COLOR1;
      else if(v > BAR_COLOR3_STOP) draw_dsc.bg_color = BAR_COLOR3;
      else if(v > BAR_COLOR2_STOP) draw_dsc.bg_color = lv_color_mix(BAR_COLOR3, BAR_COLOR2,
                                                                        ((v - BAR_COLOR2_STOP) * 255) / (BAR_COLOR3_STOP - BAR_COLOR2_STOP));
      else draw_dsc.bg_color = lv_color_mix(BAR_COLOR2, BAR_COLOR1,
                                                ((v - BAR_COLOR1_STOP) * 255) / (BAR_COLOR2_STOP - BAR_COLOR1_STOP));

      uint32_t di = deg + deg_space;

      int32_t x1_out = get_cos(di, v);
      poly[0].x = center.x + x1_out;
      poly[0].y = center.y + get_sin(di, v);

      int32_t x1_in = get_cos(di, r_in);
      poly[1].x = center.x + x1_in;
      poly[1].y = center.y + get_sin(di, r_in);
      di += DEG_STEP - deg_space * 2;

      int32_t x2_in = get_cos(di, r_in);
      poly[2].x = center.x + x2_in;
      poly[2].y = center.y + get_sin(di, r_in);

      int32_t x2_out = get_cos(di, v);
      poly[3].x = center.x + x2_out;
      poly[3].y = center.y + get_sin(di, v);

      lv_draw_polygon(draw_ctx, &draw_dsc, poly, 4);

      poly[0].x = center.x - x1_out;
      poly[1].x = center.x - x1_in;
      poly[2].x = center.x - x2_in;
      poly[3].x = center.x - x2_out;
      lv_draw_polygon(draw_ctx, &draw_dsc, poly, 4);
    }
  }
}

lv_obj_t * album_img_create(lv_obj_t * parent)
{
  LV_IMG_DECLARE(img_lv_demo_music_cover_1);                                  
  LV_IMG_DECLARE(img_lv_demo_music_cover_2);                                  
  LV_IMG_DECLARE(img_lv_demo_music_cover_3);                                  
  lv_obj_t * img;
  img = lv_img_create(parent);                                                
  switch(track_id % 3) {                                                     
    case 2:                                                                   
      lv_img_set_src(img, &img_lv_demo_music_cover_3);                        
      break;                                                                  
    case 1:                                                                   
      lv_img_set_src(img, &img_lv_demo_music_cover_2);                        
      break;                                                                  
    case 0:                                                                   
      lv_img_set_src(img, &img_lv_demo_music_cover_1);                        
      break;                                                                  
  }
  spectrum = spectrum_3;                                                      
  spectrum_len = sizeof(spectrum_3) / sizeof(spectrum_3[0]);                  
  lv_img_set_antialias(img, false);                                            
  lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);                                   
  lv_obj_add_event_cb(img, album_gesture_event_cb, LV_EVENT_GESTURE, NULL);   
  lv_obj_clear_flag(img, LV_OBJ_FLAG_GESTURE_BUBBLE);                         
  lv_obj_add_flag(img, LV_OBJ_FLAG_CLICKABLE);                                
  return img;
}
void spectrum_anim_cb(void * a, int32_t v)
{
  lv_obj_t * obj = (lv_obj_t *)a;
  lv_obj_invalidate(obj);                                                     
  static uint16_t Audio_energy_old=0;                                         
  LVGL_Music_Energy();                                                        
  if(Audio_energy_old > Audio_energy + 10000 || Audio_energy > Audio_energy_old + 10000)   
    lv_img_set_zoom(album_img_obj, LV_IMG_ZOOM_NONE + (Audio_energy/2000));   
  Audio_energy_old = Audio_energy;                                            
}
void spectrum_end_cb(lv_anim_t * a)
{
  LV_UNUSED(a);                                                               
  _lv_demo_music_album_next(true);                                            
}
void album_gesture_event_cb(lv_event_t * e)
{
  lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());                
  if(dir == LV_DIR_LEFT) _lv_demo_music_album_next(true);                      
  if(dir == LV_DIR_RIGHT) _lv_demo_music_album_next(false);                   
}

void spectrum_timer_cb(lv_timer_t * t){                                       
  LV_UNUSED(t);                                                               
  lv_obj_invalidate(panel1);                                                  
}
/************************************************************************************************************************************
 *  spectrum  END              *  spectrum   END               *  spectrum   END                *  spectrum  END                  
************************************************************************************************************************************/

/************************************************************************************************************************************
 *   create_ctrl_box                 *   create_ctrl_box                 *   create_ctrl_box                 *   create_ctrl_box                              
************************************************************************************************************************************/

lv_obj_t * create_ctrl_box(lv_obj_t * parent)
{
  lv_obj_t * cont = lv_obj_create(parent);
  lv_obj_remove_style_all(cont);                                                                 
  lv_obj_set_height(cont, LV_SIZE_CONTENT);                                                       
  lv_obj_set_style_pad_bottom(cont, 8, 0);                                                      
  static const lv_coord_t grid_col[] = { LV_GRID_FR(10), LV_GRID_FR(40), LV_GRID_FR(40), LV_GRID_FR(50), LV_GRID_FR(40), LV_GRID_FR(40), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
  static const lv_coord_t grid_row[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
  lv_obj_set_grid_dsc_array(cont, grid_col, grid_row);                                           
  LV_IMG_DECLARE(img_lv_demo_music_btn_loop);                                                    
  LV_IMG_DECLARE(img_lv_demo_music_btn_rnd);                                                      
  LV_IMG_DECLARE(img_lv_demo_music_btn_next);                                                     
  LV_IMG_DECLARE(img_lv_demo_music_btn_prev);                                                     
  LV_IMG_DECLARE(img_lv_demo_music_btn_play);                                                    
  LV_IMG_DECLARE(img_lv_demo_music_btn_pause);                                                   
  lv_obj_t * icon1;
  lv_obj_t * icon2;
  lv_obj_t * icon3;
  lv_obj_t * icon4;                                                                      
  icon1 = lv_img_create(cont);                                                                   
  lv_img_set_src(icon1, &img_lv_demo_music_btn_rnd);                                              
  lv_obj_set_grid_cell(icon1, LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);              
  
  icon2 = lv_obj_create(cont);
  lv_obj_set_size(icon2, 20, 20);
  lv_obj_set_grid_cell(icon2, LV_GRID_ALIGN_CENTER, 5, 1, LV_GRID_ALIGN_CENTER, 0, 1);
  lv_obj_set_style_bg_opa(icon2, LV_OPA_COVER, 0); 
  lv_obj_set_style_border_width(icon2, 0, 0); 
  lv_obj_set_style_radius(icon2, LV_RADIUS_CIRCLE, 0); 
  lv_obj_clear_flag(icon2, LV_OBJ_FLAG_SCROLLABLE); 

  lv_obj_t * icon2_volume = lv_label_create(icon2);                   
  lv_label_set_text(icon2_volume, LV_SYMBOL_VOLUME_MAX); 
  lv_obj_set_style_text_font(icon2_volume, &lv_font_montserrat_14, 0); 
  lv_obj_align(icon2_volume, LV_ALIGN_CENTER, 0, 0); 

  lv_obj_t *circle_button = lv_obj_create(icon2); 
  lv_obj_set_style_bg_opa(circle_button, LV_OPA_TRANSP, 0); 
  lv_obj_set_style_border_width(circle_button, 0, 0); 
  lv_obj_set_style_radius(circle_button, LV_RADIUS_CIRCLE, 0); 
  lv_obj_align(circle_button, LV_ALIGN_CENTER, 0, 0);

  lv_obj_add_event_cb(circle_button, volume_event_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_add_flag(circle_button, LV_OBJ_FLAG_CLICKABLE); 

  icon3 = lv_img_create(cont);                                                                    
  lv_img_set_src(icon3, &img_lv_demo_music_btn_prev);                                              
  lv_obj_set_grid_cell(icon3, LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 0, 1);             
  lv_obj_add_event_cb(icon3, prev_click_event_cb, LV_EVENT_CLICKED, NULL);                         
  lv_obj_add_flag(icon3, LV_OBJ_FLAG_CLICKABLE);                                                   
  
  play_obj = lv_imgbtn_create(cont);                                                                 
  lv_imgbtn_set_src(play_obj, LV_IMGBTN_STATE_RELEASED, NULL, &img_lv_demo_music_btn_play, NULL); 
  lv_imgbtn_set_src(play_obj, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, &img_lv_demo_music_btn_pause, NULL);  
  lv_obj_add_flag(play_obj, LV_OBJ_FLAG_CHECKABLE);                                                
  lv_obj_set_grid_cell(play_obj, LV_GRID_ALIGN_CENTER, 3, 1, LV_GRID_ALIGN_CENTER, 0, 1);          
  lv_obj_add_event_cb(play_obj, play_event_click_cb, LV_EVENT_CLICKED, NULL);                     
  lv_obj_add_flag(play_obj, LV_OBJ_FLAG_CLICKABLE);                                               
  lv_obj_set_width(play_obj, img_lv_demo_music_btn_play.header.w);                                
  
  icon4 = lv_img_create(cont);                                                                    
  lv_img_set_src(icon4, &img_lv_demo_music_btn_next);                                              
  lv_obj_set_grid_cell(icon4, LV_GRID_ALIGN_CENTER, 4, 1, LV_GRID_ALIGN_CENTER, 0, 1);             
  lv_obj_add_event_cb(icon4, next_click_event_cb, LV_EVENT_CLICKED, NULL);                         
  lv_obj_add_flag(icon4, LV_OBJ_FLAG_CLICKABLE);                                                   
  LV_IMG_DECLARE(img_lv_demo_music_slider_knob);                                                  
  
  slider_obj = lv_slider_create(cont);                                                                      
  lv_obj_set_style_anim_time(slider_obj, 100, 0);                                                 
  lv_obj_add_flag(slider_obj, LV_OBJ_FLAG_CLICKABLE);                                              
  lv_obj_set_width(slider_obj, 200); 
  lv_obj_set_height(slider_obj, 3);                                                               
  lv_obj_set_grid_cell(slider_obj, LV_GRID_ALIGN_STRETCH, 1, 4, LV_GRID_ALIGN_CENTER, 1, 1);      
  lv_obj_set_style_bg_img_src(slider_obj, &img_lv_demo_music_slider_knob, LV_PART_KNOB);          
  lv_obj_set_style_bg_opa(slider_obj, LV_OPA_TRANSP, LV_PART_KNOB);                               
  lv_obj_set_style_pad_all(slider_obj, 20, LV_PART_KNOB);                                         
  lv_obj_set_style_bg_grad_dir(slider_obj, LV_GRAD_DIR_HOR, LV_PART_INDICATOR);                   
  lv_obj_set_style_bg_color(slider_obj, lv_color_hex(0x569af8), LV_PART_INDICATOR);               
  lv_obj_set_style_bg_grad_color(slider_obj, lv_color_hex(0xa666f1), LV_PART_INDICATOR);          
  lv_obj_set_style_outline_width(slider_obj, 0, 0);                                               
  
  time_obj = lv_label_create(cont);                                                              
  lv_obj_set_style_text_font(time_obj, font_small, 0);                                            
  lv_obj_set_style_text_color(time_obj, lv_color_hex(0x8a86b8), 0);                               
  lv_label_set_text(time_obj, "0:00");                                                            
  lv_obj_set_grid_cell(time_obj, LV_GRID_ALIGN_END, 6, 1, LV_GRID_ALIGN_CENTER, 1, 1);            

  return cont;
}

void track_load(uint32_t id) 
{
  if(first_Flag) {                                                          
    if(id == track_id) return;                                              
  }
                                                                            
  time_act = 0;                                                             
  lv_slider_set_value(slider_obj, 0, LV_ANIM_OFF);                          
  lv_label_set_text(time_obj, "0:00");                                      
  bool next = false;
  if((track_id + 1) % ACTIVE_TRACK_CNT == id) next = true;                  
  if(first_Flag || id != track_id) {                                                         
    _lv_demo_music_list_btn_check(track_id, false);                        
    track_id = id;                                                          
  }
  _lv_demo_music_list_btn_check(id, true);                                  
  first_Flag = true;                                                        
  lv_label_set_text(title_label, Audio_Name);                               
                                                                            
  lv_anim_t a;                                                              
  lv_anim_init(&a);                                                         
  lv_anim_set_var(&a, album_img_obj);                                      
  lv_anim_set_values(&a, lv_obj_get_style_img_opa(album_img_obj, 0), LV_OPA_TRANSP);   
  lv_anim_set_exec_cb(&a, album_fade_anim_cb);                              
  lv_anim_set_time(&a, 500);                                                
  lv_anim_start(&a);                                                        
                                                                            
  lv_anim_init(&a);                                                          
  lv_anim_set_var(&a, album_img_obj);                                       
  lv_anim_set_time(&a, 500);                                                 
  lv_anim_set_path_cb(&a, lv_anim_path_ease_out);                           
  if(next) {
    lv_anim_set_values(&a, 0, - LV_HOR_RES / 2);                            
  }
  else {
    lv_anim_set_values(&a, 0, LV_HOR_RES / 2);                                
  }
  lv_anim_set_exec_cb(&a, _obj_set_x_anim_cb);                              
  lv_anim_set_ready_cb(&a, lv_obj_del_anim_ready_cb);                       
  lv_anim_start(&a);                                                        
  lv_anim_set_path_cb(&a, lv_anim_path_linear);                             
  lv_anim_set_var(&a, album_img_obj);                                       
  lv_anim_set_time(&a, 500);                                                 
  lv_anim_set_values(&a, LV_IMG_ZOOM_NONE, LV_IMG_ZOOM_NONE / 2);           
  lv_anim_set_exec_cb(&a, _img_set_zoom_anim_cb);                           
  lv_anim_set_ready_cb(&a, NULL);                                           
  lv_anim_start(&a);                                                        
  album_img_obj = album_img_create(spectrum_obj);                           
  lv_anim_set_path_cb(&a, lv_anim_path_overshoot);                          
  lv_anim_set_var(&a, album_img_obj);                                        
  lv_anim_set_time(&a, 500);                                                           
  lv_anim_set_delay(&a, 100);                                                         
  lv_anim_set_values(&a, LV_IMG_ZOOM_NONE / 4, LV_IMG_ZOOM_NONE);           
  lv_anim_set_exec_cb(&a, _img_set_zoom_anim_cb);                                  
  lv_anim_set_ready_cb(&a, NULL);                                           
  lv_anim_start(&a);                                                        
  lv_anim_init(&a);                                       
  lv_anim_set_var(&a, album_img_obj);                                         
  lv_anim_set_values(&a, 0, LV_OPA_COVER);                                      
  lv_anim_set_exec_cb(&a, album_fade_anim_cb);                                     
  lv_anim_set_time(&a, 500);                                                 
  lv_anim_set_delay(&a, 100);                                               
  lv_anim_start(&a);                                                       
}

void play_event_click_cb(lv_event_t * e)
{
  lv_obj_t * obj = lv_event_get_target(e);                                  
  if(lv_obj_has_state(obj, LV_STATE_CHECKED)) {                             
    _lv_demo_music_resume();                                                
  }
  else {
    _lv_demo_music_pause();                                                 
  }
}
void prev_click_event_cb(lv_event_t * e)
{
  lv_event_code_t code = lv_event_get_code(e);                              
  if(code == LV_EVENT_CLICKED) {                                            
    _lv_demo_music_album_next(false);                                       
  }
}
void next_click_event_cb(lv_event_t * e)
{
  lv_event_code_t code = lv_event_get_code(e);                              
  if(code == LV_EVENT_CLICKED) {                                            
    _lv_demo_music_album_next(true);                                          
  }
}

static lv_obj_t * panel;
static lv_obj_t * slider;        
static lv_obj_t * slider_volume;  

void volume_adjustment_event_cb(lv_event_t * e) {
  uint8_t Volume = lv_slider_get_value(lv_event_get_target(e));
  if (Volume >= 0 && Volume <= Volume_MAX)  {
    lv_slider_set_value(slider, Volume, LV_ANIM_ON); 
    LVGL_volume_adjustment(Volume);
    // printf("Volume:%d\r\n",Volume) ;
  }
  else
    printf("Volume out of range: %d\n", Volume);

}

void background_event_cb(lv_event_t * e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CLICKED) {
    if (lv_event_get_target(e) == panel && lv_event_get_target(e) != slider_volume) {
      lv_obj_add_flag(panel, LV_OBJ_FLAG_HIDDEN);
    }
  }
}

void volume_event_cb(lv_event_t * e) {
  printf("Clicked on volume icon\r\n");  
  lv_event_code_t code = lv_event_get_code(e);                              
  if(code == LV_EVENT_CLICKED) {     
    if (!slider) {
      panel = lv_obj_create(lv_scr_act());
      lv_obj_set_size(panel, lv_obj_get_width(lv_scr_act()), lv_obj_get_height(lv_scr_act()));
      lv_obj_set_pos(panel, 0, 0);
      lv_obj_set_style_border_width(panel, 0, 0); 
      lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE); 
      lv_obj_set_style_bg_opa(panel, LV_OPA_TRANSP, LV_PART_MAIN); 

      slider = lv_slider_create(panel);                                 
      lv_obj_add_flag(slider, LV_OBJ_FLAG_CLICKABLE);    
      lv_obj_set_size(slider, 20, 170);                                                          
      lv_obj_set_style_bg_opa(slider, LV_OPA_TRANSP, LV_PART_KNOB);                               
      lv_obj_set_style_pad_all(slider, 20, LV_PART_KNOB);                                            
      lv_obj_set_style_bg_color(slider, lv_color_hex(0xADD8F6), LV_PART_INDICATOR);              
      lv_obj_set_style_outline_width(slider, 0, 0);  
      lv_slider_set_range(slider, 0, Volume_MAX);              
      lv_slider_set_value(slider, Volume, LV_ANIM_ON);  
      lv_obj_align_to(slider, panel, LV_ALIGN_RIGHT_MID, 0, 0); 
      
      slider_volume = lv_slider_create(panel);                                 
      lv_obj_add_flag(slider_volume, LV_OBJ_FLAG_CLICKABLE);    
      lv_obj_set_size(slider_volume, 50, 170);                                                          
      lv_obj_set_style_bg_opa(slider_volume, LV_OPA_TRANSP, LV_PART_KNOB);                               
      lv_obj_set_style_pad_all(slider_volume, 50, LV_PART_KNOB);                                            
      lv_obj_set_style_bg_color(slider_volume, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR);              
      lv_obj_set_style_outline_width(slider_volume, 0, 0);  
      lv_slider_set_range(slider_volume, 0, Volume_MAX);              
      lv_slider_set_value(slider_volume, Volume, LV_ANIM_ON);  
      lv_obj_align_to(slider_volume, panel, LV_ALIGN_RIGHT_MID, 20, 0); 
      lv_obj_set_style_bg_opa(slider_volume, LV_OPA_TRANSP, LV_PART_MAIN); 
      lv_obj_set_style_bg_opa(slider_volume, LV_OPA_TRANSP, LV_PART_KNOB); 
      lv_obj_set_style_bg_opa(slider_volume, LV_OPA_TRANSP, LV_PART_INDICATOR); 

      lv_obj_add_event_cb(slider_volume, volume_adjustment_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
      lv_obj_add_event_cb(panel, background_event_cb, LV_EVENT_ALL, NULL); 
    }
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_HIDDEN); 
  }
}


void hide_slider(lv_event_t * e) {
  lv_obj_t * obj = lv_event_get_target(e);
  if (obj != slider) {
    lv_obj_add_flag(slider, LV_OBJ_FLAG_HIDDEN); 
  }
}

void timer_cb(lv_timer_t * t)
{
  LV_UNUSED(t);                                                             
  if(Audio_duration == 0){
    Audio_duration = Music_Duration();                            
    if(Audio_duration != 0) 
    {
      _lv_demo_music_resume();
    }
  } 
  else{
    time_act++;                                                               
    lv_label_set_text_fmt(time_obj, "%"LV_PRIu32":%02"LV_PRIu32, time_act / 60, time_act % 60);   
    lv_slider_set_value(slider_obj, time_act, LV_ANIM_ON);  
    if(time_act >= Audio_duration){                             
      _lv_demo_music_album_next(true);    
    }
  }                 
}
void album_fade_anim_cb(void * var, int32_t v)
{
  lv_obj_set_style_img_opa((_lv_obj_t*)var, v, 0);                          
}

/************************************************************************************************************************************
 *  create_ctrl_box  END             *  create_ctrl_box  END            *  create_ctrl_box  END           *  create_ctrl_box  END
************************************************************************************************************************************/
/************************************************************************************************************************************
 *   create_ctrl_box                      *   create_ctrl_box                    *   create_ctrl_box                    *   create_ctrl_box                       
************************************************************************************************************************************/

lv_obj_t * create_List_box(lv_obj_t * parent)
{
  static const lv_coord_t grid_cols[] = {LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
  static const lv_coord_t grid_rows[] = {LV_GRID_CONTENT,  LV_GRID_CONTENT,  LV_GRID_CONTENT,  LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
  lv_style_init(&style_btn_stop);
  lv_style_set_bg_opa(&style_btn_stop, LV_OPA_TRANSP);                      
  lv_style_set_grid_column_dsc_array(&style_btn_stop, grid_cols);           
  lv_style_set_grid_row_dsc_array(&style_btn_stop, grid_rows);              
  lv_style_set_grid_row_align(&style_btn_stop, LV_GRID_ALIGN_CENTER);       
  lv_style_set_layout(&style_btn_stop, LV_LAYOUT_GRID);                     
  lv_style_set_pad_right(&style_btn_stop, 20);   

  lv_style_init(&style_btn_round);
  lv_style_set_radius(&style_btn_round, 10); 

  lv_style_init(&style_btn_pr);                                             
  lv_style_set_bg_opa(&style_btn_pr, LV_OPA_COVER);                         
  lv_style_set_bg_color(&style_btn_pr,  lv_color_hex(0xCDE8F3));            

  lv_style_init(&style_btn_play);                                           
  lv_style_set_bg_opa(&style_btn_play, LV_OPA_COVER);                          
  lv_style_set_bg_color(&style_btn_play, lv_color_hex(0xAAD3E0));           

  lv_style_init(&style_title);                                              
  lv_style_set_text_font(&style_title, font_small);                         
  lv_style_set_text_color(&style_title, lv_color_hex(0x101010));            
    
  list = lv_obj_create(parent);
  lv_obj_remove_style_all(list);                                            
  lv_obj_set_size(list, LV_SIZE_CONTENT, LV_SIZE_CONTENT);                  
  lv_obj_set_pos(list, 0, LV_DEMO_MUSIC_HANDLE_SIZE);                       
  // lv_obj_set_y(list, LV_DEMO_MUSIC_HANDLE_SIZE);
  lv_obj_add_style(list, &music_style, LV_PART_SCROLLBAR);
  lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);

  uint32_t List_id;
  for(List_id = 0; List_id < ACTIVE_TRACK_CNT; List_id++) {                 
      add_list_btn(list,  List_id);                                         
  }
  lv_obj_set_scroll_snap_y(list, LV_SCROLL_SNAP_CENTER);                    
  _lv_demo_music_list_btn_check(0, true);                                         
  return list;
}
lv_obj_t * add_list_btn(lv_obj_t * parent, uint32_t List_id)
{
  lv_obj_t * btn = lv_obj_create(parent);                                  
  lv_obj_remove_style_all(btn);                                             
  lv_obj_set_size(btn, lv_pct(100), 60);  
  lv_obj_add_style(btn, &style_btn_round, 0);                                  

  lv_obj_add_style(btn, &style_btn_stop, 0);                                
  lv_obj_add_style(btn, &style_btn_play, LV_STATE_CHECKED);                 
  lv_obj_add_style(btn, &style_btn_pr, LV_STATE_PRESSED);                   
  lv_obj_add_event_cb(btn, btn_click_event_cb, LV_EVENT_CLICKED, NULL);     


  lv_obj_t * icon = lv_img_create(btn);                                     
  lv_img_set_src(icon, &img_lv_demo_music_btn_list_play);                   
  lv_obj_set_grid_cell(icon, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 0, 2);  

  lv_obj_t * title_label = lv_label_create(btn);                           
  lv_label_set_text(title_label, File_Name[List_id]);                       
  lv_obj_set_grid_cell(title_label, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);
  lv_obj_add_style(title_label, &style_title, 0);

  // LV_IMG_DECLARE(img_lv_demo_music_list_border);                            
  // lv_obj_t * border = lv_img_create(btn);                                   
  // lv_img_set_src(border, &img_lv_demo_music_list_border);                   
  // lv_obj_set_width(border, lv_pct(120));                                    
  // lv_obj_align(border, LV_ALIGN_BOTTOM_MID, 0, 0);                          
  // lv_obj_add_flag(border, LV_OBJ_FLAG_IGNORE_LAYOUT);                       
  return btn;
}

void _lv_demo_music_list_btn_check(uint32_t List_id, bool state)
{
  lv_obj_t * btn = lv_obj_get_child(list, List_id);                           
  lv_obj_t * icon = lv_obj_get_child(btn, 0);                                

  if(state) {
    lv_obj_add_state(btn, LV_STATE_CHECKED);                                  
    lv_img_set_src(icon, &img_lv_demo_music_btn_list_pause);                  
    lv_obj_scroll_to_view(btn, LV_ANIM_ON);                                   
  }
  else {
    lv_obj_clear_state(btn, LV_STATE_CHECKED);                                
    lv_img_set_src(icon, &img_lv_demo_music_btn_list_play);                   
  }
  // lv_obj_scroll_to_view(panel1, LV_ANIM_ON);                               
  lv_obj_invalidate(panel1);                                                 
}

void btn_click_event_cb(lv_event_t * e)
{
  lv_obj_t * btn = lv_event_get_target(e);                                    
  uint32_t idx = lv_obj_get_child_id(btn);                                    
  _lv_demo_music_play(idx);                                                   
}

/************************************************************************************************************************************
 *   create_ctrl_box END                *   create_ctrl_box END                 *   create_ctrl_box END                   *   create_ctrl_box END
************************************************************************************************************************************/
/************************************************************************************************************************************
 *  Music                    *  Music                     *  Music                       *  Music          
************************************************************************************************************************************/
void _lv_demo_music_main_close(void)
{
  lv_timer_del(sec_counter_timer);                                
}

void _lv_demo_music_album_next(bool next)
{
  uint32_t id = track_id;
  if(next) {                                                                         
    id++;                                                         
    if(id >= ACTIVE_TRACK_CNT) id = 0;                            
  }
  else {                                                          
    if(id == 0) {                                                 
      id = ACTIVE_TRACK_CNT - 1;                                  
    }
    else {                                                        
      id--;                                                       
    }
  }
  _lv_demo_music_play(id);                                        
}
void _lv_demo_music_play(uint32_t id)
{
  if(Playing_Flag && id == track_id){                             
  }                                                               
  else{                                                              
    if(id == track_id){                                           
      LVGL_Elapsed_Music();                                       
      LVGL_Resume_Music();                                        
      _lv_demo_music_resume();                                    
    }
    else{                                                         
      Audio_Elapsed = 0;                                          
      LVGL_Play_Music(id);                                        
      track_load(id);                                             
      _lv_demo_music_resume();                                    
    }
  }
}
void _lv_demo_music_resume(void) {
  Playing_Flag = true;  
  lv_anim_t a;
  lv_anim_init(&a);
  if(Audio_duration == 0)
    Audio_duration_A = Audio_Elapsed + 100;
  else
    Audio_duration_A = Audio_duration;
  lv_anim_set_values(&a, Audio_Elapsed * 1000, Audio_duration_A * 1000 - 1);  
  lv_anim_set_exec_cb(&a, spectrum_anim_cb);                      
  lv_anim_set_var(&a, spectrum_obj);                              
  lv_anim_set_time(&a, (Audio_duration_A - Audio_Elapsed) * 1000);  
  lv_anim_set_playback_time(&a, 0);                               
  lv_anim_set_ready_cb(&a, spectrum_end_cb);                      
  lv_anim_start(&a);                                              

  lv_timer_resume(sec_counter_timer);  
  lv_timer_resume(spectrum_timer);                            
  lv_slider_set_range(slider_obj, 0, Audio_duration_A );             
  lv_obj_add_state(play_obj, LV_STATE_CHECKED); 
    
  LVGL_Resume_Music();                                            
}
void _lv_demo_music_pause(void)                                 
{
  Playing_Flag = false;                                           
  lv_anim_del(spectrum_obj, spectrum_anim_cb);                    
  lv_obj_invalidate(spectrum_obj);                                

  lv_img_set_zoom(album_img_obj, LV_IMG_ZOOM_NONE);               
  lv_timer_pause(spectrum_timer);                             
  lv_timer_pause(sec_counter_timer);                              
  lv_obj_clear_state(play_obj, LV_STATE_CHECKED);                 

  LVGL_Pause_Music();                                             
}

/************************************************************************************************************************************
 *  Other         *  Other         *  Other         *  Other                   
************************************************************************************************************************************/

void remove_file_extension(char *file_name) {
  char *last_dot = strrchr(file_name, '.');
  if (last_dot != NULL) {
    *last_dot = '\0'; 
  }
}
void LVGL_Search_Music() {        
  ACTIVE_TRACK_CNT = Folder_retrieval("/",".mp3",SD_Name,100);
  if(ACTIVE_TRACK_CNT) {  
    for (int i = 0; i < ACTIVE_TRACK_CNT; i++) {
      strcpy(File_Name[i], SD_Name[i]);
      remove_file_extension(File_Name[i]); 
    }                
    LVGL_Play_Music(0);    
  }                                                             
}
void LVGL_Play_Music(uint32_t ID) {
  Play_Music("/",SD_Name[ID]);
  strncpy(Audio_Name,File_Name[ID], sizeof(File_Name[ID]));       
  Audio_duration = Music_Duration();  
  // while(Audio_duration == 0)   
  //   Audio_duration = Music_Duration();  
}
void LVGL_Elapsed_Music() {
  Audio_Elapsed = Music_Elapsed();                             
}
uint16_t LVGL_Music_Energy( ) {
  Audio_energy = Music_Energy();                                 
  return Audio_energy;
}
void LVGL_Resume_Music() {
  Music_resume();                                                 
}
void LVGL_Pause_Music() {
  Music_pause();                                                  
}
void LVGL_volume_adjustment(uint8_t Volume) {
  Volume_adjustment(Volume);                                 
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
