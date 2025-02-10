#ifndef option_h
#define option_h

#include <Arduino.h>
#include "lvgl.h"
#include "../ui_helpers.h"
#include "../ui_events.h"

#include "utils/util.h"
#include "utils/touch_lib.h"

#include "Scr.h"

#include "utils/util.h"

class Scr;

typedef void (*option_event_cb_t)(void *data);

enum OptionType
{
    TEXT_WITH_VALUE,
    SPACE,
    HEADER,
    ON_OFF,
    RADIO
};

union OptionValue
{
    int INT;
    float FLOAT;
    const char *STRING;
};
const OptionValue VALUE_ZERO = {.INT = 0};

class Option
{
public:
    lv_obj_t *root;
    lv_obj_t *bar;
    lv_obj_t *text;
    lv_obj_t *rightHandObj;
    lv_obj_t *ui_imgOn;
    lv_obj_t *ui_imgOff;
    option_event_cb_t event_cb;
    OptionType type;

    Option(lv_obj_t *parent, OptionType type, const char *text, OptionValue value, option_event_cb_t _event_cb = NULL);
    void setRightHandText(const char *str);
    void setBooleanValue(bool value, bool netSend = false);
    void indentText();
};

#endif