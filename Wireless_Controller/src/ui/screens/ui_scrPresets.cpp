#include "ui_scrPresets.h"

LV_IMG_DECLARE(navbar_presets);
ScrPresets scrPresets(navbar_presets, true);

LV_IMG_DECLARE(presets_bg);
LV_IMG_DECLARE(img_car);
LV_IMG_DECLARE(img_wheels);
LV_IMG_DECLARE(selected_1);
LV_IMG_DECLARE(selected_2);
LV_IMG_DECLARE(selected_3);
LV_IMG_DECLARE(selected_4);
LV_IMG_DECLARE(selected_5);

const int car_x = DISPLAY_WIDTH / 2 - img_car.header.w / 2;
const int wheels_x = DISPLAY_WIDTH / 2 - img_wheels.header.w / 2;
const int wheels_y = 88;
const int car_y_1 = wheels_y - 21;
const int car_y_2 = car_y_1 - 4;
const int car_y_3 = car_y_2 - 4;
const int car_y_4 = car_y_3 - 4;
const int car_y_5 = car_y_4 - 4;

void car_anim_func(lv_obj_t *obj, int32_t y)
{

    lv_obj_set_y(obj, y);
    lv_obj_move_foreground(scrPresets.wheels);
}

void animCarPreset(ScrPresets *scr, lv_coord_t end)
{
    int duration = 1000;                       // animation duration in ms
    lv_coord_t start = lv_obj_get_y(scr->car); // start is the current location
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)car_anim_func);
    lv_anim_set_var(&a, scr->car);
    lv_anim_set_time(&a, duration);
    lv_anim_set_values(&a, start, end);
    lv_anim_start(&a);
}

void ScrPresets::init()
{
    Scr::init();

    lv_obj_add_flag(this->rect_bg, LV_OBJ_FLAG_HIDDEN); // hide grey background, we have a different one on this page
    lv_obj_delete(this->rect_bg);

    // background image
    this->rect_bg = lv_image_create(this->scr);
    lv_image_set_src(this->rect_bg, &presets_bg);
    lv_obj_set_align(this->rect_bg, LV_ALIGN_TOP_MID);

    // car
    this->car = lv_image_create(this->scr);
    lv_image_set_src(this->car, &img_car);
    // lv_obj_set_align(this->car, LV_ALIGN_CENTER);
    lv_obj_set_x(this->car, car_x);
    lv_obj_set_y(this->car, car_y_1);

    // wheels
    this->wheels = lv_image_create(this->scr);
    lv_image_set_src(this->wheels, &img_wheels);
    lv_obj_set_x(this->wheels, wheels_x);
    lv_obj_set_y(this->wheels, wheels_y);

    // preset selector overlays
    this->btnPreset1 = lv_image_create(this->scr);
    lv_image_set_src(this->btnPreset1, &selected_1);
    lv_obj_set_x(this->btnPreset1, ctr_preset_1.cx - selected_1.header.w / 2);
    lv_obj_set_y(this->btnPreset1, ctr_preset_1.cy - selected_1.header.h / 2);

    this->btnPreset2 = lv_image_create(this->scr);
    lv_image_set_src(this->btnPreset2, &selected_2);
    lv_obj_set_x(this->btnPreset2, ctr_preset_2.cx - selected_2.header.w / 2);
    lv_obj_set_y(this->btnPreset2, ctr_preset_2.cy - selected_2.header.h / 2);

    this->btnPreset3 = lv_image_create(this->scr);
    lv_image_set_src(this->btnPreset3, &selected_3);
    lv_obj_set_x(this->btnPreset3, ctr_preset_3.cx - selected_3.header.w / 2);
    lv_obj_set_y(this->btnPreset3, ctr_preset_3.cy - selected_3.header.h / 2);

    this->btnPreset4 = lv_image_create(this->scr);
    lv_image_set_src(this->btnPreset4, &selected_4);
    lv_obj_set_x(this->btnPreset4, ctr_preset_4.cx - selected_4.header.w / 2);
    lv_obj_set_y(this->btnPreset4, ctr_preset_4.cy - selected_4.header.h / 2);

    this->btnPreset5 = lv_image_create(this->scr);
    lv_image_set_src(this->btnPreset5, &selected_5);
    lv_obj_set_x(this->btnPreset5, ctr_preset_5.cx - selected_5.header.w / 2);
    lv_obj_set_y(this->btnPreset5, ctr_preset_5.cy - selected_5.header.h / 2);

    lv_obj_move_foreground(this->icon_navbar);                  // bring navbar to foreground
    lv_obj_move_foreground(this->ui_lblPressureFrontPassenger); // pressures to foreground front
    lv_obj_move_foreground(this->ui_lblPressureRearPassenger);  // pressures to foreground front
    lv_obj_move_foreground(this->ui_lblPressureFrontDriver);    // pressures to foreground front
    lv_obj_move_foreground(this->ui_lblPressureRearDriver);     // pressures to foreground front
    lv_obj_move_foreground(this->ui_lblPressureTank);           // pressures to foreground front

    this->hideSelectors();
    setPreset(3);
}

void ScrPresets::hideSelectors()
{
    lv_obj_add_flag(btnPreset1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(btnPreset2, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(btnPreset3, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(btnPreset4, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(btnPreset5, LV_OBJ_FLAG_HIDDEN);
};
int currentPreset = 3;
void ScrPresets::setPreset(int num)
{
    currentPreset = num;
    hideSelectors();
    switch (num)
    {
    case 1:
        animCarPreset(this, car_y_1);
        lv_obj_remove_flag(btnPreset1, LV_OBJ_FLAG_HIDDEN);
        break;
    case 2:
        animCarPreset(this, car_y_2);
        lv_obj_remove_flag(btnPreset2, LV_OBJ_FLAG_HIDDEN);
        break;
    case 3:
        animCarPreset(this, car_y_3);
        lv_obj_remove_flag(btnPreset3, LV_OBJ_FLAG_HIDDEN);
        break;
    case 4:
        animCarPreset(this, car_y_4);
        lv_obj_remove_flag(btnPreset4, LV_OBJ_FLAG_HIDDEN);
        break;
    case 5:
        animCarPreset(this, car_y_5);
        lv_obj_remove_flag(btnPreset5, LV_OBJ_FLAG_HIDDEN);
        break;
    }

    // request preset data (not really used at the moment but may be in the future)
    PresetPacket pkt(currentPreset - 1, 0, 0, 0, 0);
    sendRestPacket(&pkt);
}

void ScrPresets::runTouchInput(SimplePoint pos, bool down)
{
    Scr::runTouchInput(pos, down);
    if (down == false) // just released on button
    {
        if (cr_contains(ctr_preset_1, pos))
        {
            setPreset(1);
        }
        if (cr_contains(ctr_preset_2, pos))
        {
            setPreset(2);
        }
        if (cr_contains(ctr_preset_3, pos))
        {
            setPreset(3);
        }
        if (cr_contains(ctr_preset_4, pos))
        {
            setPreset(4);
        }
        if (cr_contains(ctr_preset_5, pos))
        {
            setPreset(5);
        }
        if (sr_contains(preset_save, pos))
        {
            Serial.println("save preset");
            SaveCurrentPressuresToProfilePacket pkt(currentPreset - 1);
            sendRestPacket(&pkt);
            showDialog("Saved Preset!", lv_color_hex(0xBB86FC));
        }
        if (sr_contains(preset_load, pos))
        {
            Serial.println("load preset");
            AirupQuickPacket pkt(currentPreset - 1);
            sendRestPacket(&pkt);
            showDialog("Loaded Preset!", lv_color_hex(0x22bb33));
        }
    }
}

void ScrPresets::loop()
{
    Scr::loop();
}
