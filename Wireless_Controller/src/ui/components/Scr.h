#ifndef ui_scr_h
#define ui_scr_h

#include <Arduino.h>
#include "lvgl.h"

#include "utils/util.h"
#include "utils/touch_lib.h"

#include "alert.h"

class Alert;

struct DialogData
{
    int type;
    std::function<void()> callback;
};

class Scr
{
public:
    bool showPressures;
    lv_obj_t *scr;
    lv_obj_t *rect_bg;
    Alert *alert;
    lv_obj_t *ui_lblPressureFrontDriver;
    lv_obj_t *ui_lblPressureRearDriver;
    lv_obj_t *ui_lblPressureFrontPassenger;
    lv_obj_t *ui_lblPressureRearPassenger;
    lv_obj_t *ui_lblPressureTank;
    int prevPressures[5];
    int prevUnitsMode;
    lv_obj_t *mb_dialog;
    DialogData dialogDataYes;
    DialogData dialogDataNo;
    bool deleteMessageBoxNextFrame; // want to make sure message box is still shown for the full duration of the current frame that it is deleted on, else the clicks on the dialog closing it will also register on the screen possibly triggering other buttons. Will delete it immediately at beginning of next frame
    bool mb_force_button_press;

    Scr(bool showPressures);
    virtual void init(lv_obj_t *parent);
    virtual void loop();
    virtual void cleanup();
    void updatePressureValues();
    void showMsgBox(const char *title, const char *text, const char *yesText, const char *noText, std::function<void()> onYes, std::function<void()> onNo, bool forceButtonPress);
    bool isMsgBoxDisplayed();
};

#endif