#include "ui_scrPresets.h"

LV_IMG_DECLARE(navbar_presets);
ScrPresets scrPresets(navbar_presets);

lv_obj_t *list1;
lv_obj_t *list2;

static lv_obj_t *loadButton = NULL;
static lv_obj_t *setButton = NULL;
static lv_obj_t *currentButton = NULL; // TODO move this to class
static void event_handler(lv_event_t *e)
{

    lv_obj_add_flag(loadButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(setButton, LV_OBJ_FLAG_HIDDEN);

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);
    if (code == LV_EVENT_CLICKED)
    {
        // LV_LOG_USER("Clicked: %s", lv_list_get_button_text(list1, obj));

        if (currentButton == obj)
        {
            currentButton = NULL;
        }
        else
        {
            currentButton = obj;
        }
        lv_obj_t *parent = lv_obj_get_parent(obj);
        uint32_t i;
        for (i = 0; i < lv_obj_get_child_count(parent); i++)
        {
            lv_obj_t *child = lv_obj_get_child(parent, i);
            if (child == currentButton)
            {
                lv_obj_add_state(child, LV_STATE_CHECKED);
                lv_obj_remove_flag(loadButton, LV_OBJ_FLAG_HIDDEN);
                lv_obj_remove_flag(setButton, LV_OBJ_FLAG_HIDDEN);
            }
            else
            {
                lv_obj_remove_state(child, LV_STATE_CHECKED);
            }
        }
    }
}

static void event_handler_load(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED)
    {
        if (currentButton == NULL)
            return;
        Serial.println(lv_list_get_button_text(list1, currentButton));

        AirupQuickPacket pkt(0);
        sendRestPacket(&pkt);
    }
}

static void event_handler_set(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED)
    {
        if (currentButton == NULL)
            return;
        Serial.println(lv_list_get_button_text(list1, currentButton));

        SaveCurrentPressuresToProfilePacket pkt(0);
        sendRestPacket(&pkt);
    }
}

void ScrPresets::init()
{
    Scr::init();

    log_i("Icon navbar height: %i", lv_obj_get_height(this->icon_navbar));

    this->panel = lv_obj_create(this->scr);
    lv_obj_remove_style_all(this->panel);
    lv_obj_set_width(this->panel, DISPLAY_WIDTH);
    lv_obj_set_height(this->panel, DISPLAY_HEIGHT - this->navbarImage.header.h);
    lv_obj_set_align(this->panel, LV_ALIGN_TOP_MID);
    // lv_obj_remove_flag(this->panel, LV_OBJ_FLAG_SCROLLABLE); /// Flags

    /*Create a list*/
    list1 = lv_list_create(this->panel);
    lv_obj_set_style_bg_color(list1, lv_color_hex(0x1F1F1F), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_size(list1, lv_pct(60), lv_pct(100));
    lv_obj_set_style_pad_row(list1, 5, 0);
    lv_obj_set_style_border_color(list1, lv_color_hex(0x2F2F2F), LV_PART_MAIN | LV_STATE_DEFAULT);

    /*Add buttons to the list*/
    lv_obj_t *btn;
    int i;
    for (i = 0; i < 5; i++)
    {
        btn = lv_button_create(list1);
        lv_obj_set_width(btn, lv_pct(50));
        lv_obj_add_event_cb(btn, event_handler, LV_EVENT_CLICKED, NULL);

        lv_obj_t *lab = lv_label_create(btn);
        lv_label_set_text_fmt(lab, "%d", i + 1);
    }

    /*Select the first button by default*/
    currentButton = lv_obj_get_child(list1, 0);
    lv_obj_add_state(currentButton, LV_STATE_CHECKED);

    /*Create a second list with up and down buttons*/
    list2 = lv_list_create(this->panel);
    lv_obj_set_style_bg_color(list2, lv_color_hex(0x1F1F1F), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_size(list2, lv_pct(40), lv_pct(100));
    lv_obj_align(list2, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_flex_flow(list2, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_border_color(list2, lv_color_hex(0x2F2F2F), LV_PART_MAIN | LV_STATE_DEFAULT);

    loadButton = lv_list_add_button(list2, NULL, "Load");
    lv_obj_add_event_cb(loadButton, event_handler_load, LV_EVENT_ALL, NULL);
    lv_group_remove_obj(loadButton);
    setButton = lv_list_add_button(list2, NULL, "Save");
    lv_obj_add_event_cb(setButton, event_handler_set, LV_EVENT_ALL, NULL);
    lv_group_remove_obj(setButton);
}

void ScrPresets::runTouchInput(SimplePoint pos, bool down)
{
    Scr::runTouchInput(pos, down);
}

void ScrPresets::loop()
{
    Scr::loop();
}
