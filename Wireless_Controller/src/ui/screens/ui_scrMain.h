#ifndef ui_scrMain_h
#define ui_scrMain_h

#ifdef __cplusplus
extern "C" {
#endif
#include <Arduino.h>
#include "lvgl.h"
#include "../ui_helpers.h"
#include "../ui_events.h"

typedef struct
{
    lv_obj_t *ui_scrMain;
    lv_obj_t *ui_pnlMain;
    lv_obj_t *ui_lblMilliseconds;
    lv_obj_t *ui_lblMillisecondsValue;
    lv_obj_t *ui_lblCdr;
    lv_obj_t *ui_lblCdrValue;
    lv_obj_t *ui_Rotate;
    lv_obj_t *ui_Label1;
    lv_obj_t *ui_btnCount;
    lv_obj_t *ui_lblButton;
    lv_obj_t *ui_lblCount;
    lv_obj_t *ui_lblCountValue;
    lv_obj_t *ui_GradR;
    lv_obj_t *ui_GradG;
    lv_obj_t *ui_GradB;
    lv_obj_t *ui____initial_actions0;
} ScrMain;

void ui_scrMain_screen_init(void);
void ui_scrMain_event_Rotate(lv_event_t *e);
void ui_scrMain_event_btnCount(lv_event_t *e);
void ui_scrMain_loop();

extern ScrMain scrMain;
extern void ui_allEvents(lv_event_t *e);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif