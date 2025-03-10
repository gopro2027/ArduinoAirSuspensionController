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

SimpleRect fender1Offset = {40, 37, 72 - 40, 63 - 37};
SimpleRect fender2Offset = {166, 35, 199 - 166, 60 - 35};

// square 1: 40,37 -> 71, 63
// square 2: 166,35 -> 198, 60

void car_anim_func(lv_obj_t *obj, int32_t y)
{

    lv_obj_set_y(obj, y);
    lv_obj_set_y(scrPresets.ww1, y + fender1Offset.y);
    lv_obj_set_y(scrPresets.ww2, y + fender2Offset.y);
    // lv_obj_move_foreground(scrPresets.wheels);
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

    // wheel well 1
    this->ww1 = lv_obj_create(this->scr);
    lv_obj_remove_style_all(this->ww1);
    lv_obj_get_style_border_width(this->ww1, 0);
    lv_obj_set_size(this->ww1, fender1Offset.w, fender1Offset.h);
    lv_obj_set_style_bg_color(this->ww1, lv_color_hex(0x0), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_remove_flag(this->ww1, (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE)); /// Flags
    lv_obj_set_style_bg_opa(this->ww1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_x(this->ww1, car_x + fender1Offset.x);
    lv_obj_set_y(this->ww1, car_y_1 + fender1Offset.y);

    // wheel well 2
    this->ww2 = lv_obj_create(this->scr);
    lv_obj_remove_style_all(this->ww2);
    lv_obj_get_style_border_width(this->ww2, 0);
    lv_obj_set_size(this->ww2, fender2Offset.w, fender2Offset.h);
    lv_obj_set_style_bg_color(this->ww2, lv_color_hex(0x0), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_remove_flag(this->ww2, (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE)); /// Flags
    lv_obj_set_style_bg_opa(this->ww2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_x(this->ww2, car_x + fender2Offset.x);
    lv_obj_set_y(this->ww2, car_y_1 + fender2Offset.y);

    // wheels
    this->wheels = lv_image_create(this->scr);
    lv_image_set_src(this->wheels, &img_wheels);
    lv_obj_set_x(this->wheels, wheels_x);
    lv_obj_set_y(this->wheels, wheels_y);

    // car
    this->car = lv_image_create(this->scr);
    lv_image_set_src(this->car, &img_car);
    // lv_obj_set_align(this->car, LV_ALIGN_CENTER);
    lv_obj_set_x(this->car, car_x);
    lv_obj_set_y(this->car, car_y_1);

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
    this->setPreset(3);
}

void ScrPresets::hideSelectors()
{
    lv_obj_add_flag(btnPreset1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(btnPreset2, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(btnPreset3, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(btnPreset4, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(btnPreset5, LV_OBJ_FLAG_HIDDEN);
};
void ScrPresets::setPreset(int num)
{
    if (currentPreset == num)
    {
        this->showPresetDialog();
    }
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
    requestPreset();
}

void ScrPresets::showPresetDialog()
{
    static char text[60];
    static char title[10];
    snprintf(text, sizeof(text), "fp: %i, rp: %i, fd: %i, rd: %i", profilePressures[currentPreset - 1][WHEEL_FRONT_PASSENGER], profilePressures[currentPreset - 1][WHEEL_REAR_PASSENGER], profilePressures[currentPreset - 1][WHEEL_FRONT_DRIVER], profilePressures[currentPreset - 1][WHEEL_REAR_DRIVER]);
    snprintf(title, sizeof(title), "Preset %i", currentPreset);
    this->showMsgBox(title, text, NULL, "OK", []() -> void {});
}

void ScrPresets::runTouchInput(SimplePoint pos, bool down)
{
    Scr::runTouchInput(pos, down);
    if (down == false) // just released on button
    {
        if (!this->isMsgBoxDisplayed())
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
                static char buf[40];
                snprintf(buf, sizeof(buf), "Save current height to preset %i?", currentPreset);
                this->showMsgBox(buf, NULL, "Confirm", "Cancel", []() -> void
                                 {
                                     Serial.println("save preset");
                                     SaveCurrentPressuresToProfilePacket pkt(currentPreset - 1);
                                     sendRestPacket(&pkt);
                                     showDialog("Saved Preset!", lv_color_hex(THEME_COLOR_LIGHT));
                                     requestPreset(); // update data not that it's saved
                                 });
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
}

void ScrPresets::loop()
{
    Scr::loop();
}
