#ifndef radiooption_h
#define radiooption_h

#include <Arduino.h>
#include "lvgl.h"
#include "../ui_helpers.h"
#include "../ui_events.h"

#include "utils/util.h"
#include "utils/touch_lib.h"

#include "Scr.h"

#include "utils/util.h"

#include "option.h"

class Scr;

class RadioOption
{
public:
    Option **options;
    int size;
    int selected;
    option_event_cb_t onSelect;

    RadioOption(lv_obj_t *parent, const char **text, int _size, option_event_cb_t _event_cb, int _selected = 0);
    void setSelectedOption(int _selected, bool callOnSelect = false);
    int getSelectedOption();
    int getOptionIndex(Option *option);
};

#endif