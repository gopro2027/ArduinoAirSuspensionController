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

template <size_t N>
class RadioOption
{
public:
    Option options[N];
    int selected;

    RadioOption(lv_obj_t *parent, const char **text, option_event_cb_t _event_cb, int _selected = 0);
    void setSelectedOption(int _selected);
    int getSelectedOption();
};

#endif