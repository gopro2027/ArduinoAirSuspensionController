#include "device_lib_exports.h"
#include "ui_scrSettings.h"
#include <stdint.h>

#ifndef SCREEN_MODE_CIRCLE
#include "ui/components/navbar.h"
#include "ui/components/statusbar.h"
#include "custom_car_storage.h"
#include "serial_image_upload.h"
#else
#include "ui_circle/components/circle_statusbar.h"
#endif

ScrSettings scrSettings(false);

lv_obj_t *ScrSettings::addSettingsPage(lv_obj_t *pages_container, bool hiddenInitially)
{
    if (this->settingsPageCount >= kSettingsMaxPages) {
        log_e("ScrSettings::addSettingsPage overflow (max %i)", (int)kSettingsMaxPages);
        return nullptr;
    }
    lv_obj_t *page = lv_obj_create(pages_container);
    lv_obj_remove_style_all(page);
    lv_obj_set_size(page, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(page, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(page, LV_FLEX_FLOW_COLUMN);
    if (hiddenInitially)
        lv_obj_add_flag(page, LV_OBJ_FLAG_HIDDEN);
    this->pages[this->settingsPageCount++] = page;
    return page;
}

void alertValueUpdated()
{
    showDialog("Set value", lv_color_hex(0xFFFF00));
}

const char *test = "";
extern bool isConnectedToManifold();

// Right-align the SSID dropdown's open list to the dropdown's right edge and clamp its width
// to the screen, so long network names don't run off the right side. LVGL opens the list
// left-aligned (LV_ALIGN_OUT_BOTTOM_LEFT) with a content-wide list, which overflows for long
// SSIDs; call this right after the list opens to fix it.
static void alignWifiSsidList(lv_obj_t *dropdown)
{
    lv_obj_t *list = lv_dropdown_get_list(dropdown);
    if (!list)
        return;

    lv_obj_update_layout(list);

    const int margin = scaledX(10);
    const int maxW = getScreenWidth() - margin * 2;
    int w = lv_obj_get_width(list);
    if (w > maxW)
        w = maxW;
    const int ddW = lv_obj_get_width(dropdown);
    if (w < ddW)
        w = ddW;
    lv_obj_set_width(list, w);

    // Preserve whether LVGL decided to drop the list up or down (compare absolute coords,
    // since the list is parented to the screen but the dropdown is not).
    lv_area_t listCoords, ddCoords;
    lv_obj_get_coords(list, &listCoords);
    lv_obj_get_coords(dropdown, &ddCoords);
    const bool openedUp = listCoords.y1 < ddCoords.y1;
    lv_obj_align_to(list, dropdown, openedUp ? LV_ALIGN_OUT_TOP_RIGHT : LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 0);
}

// Current page tracking
static lv_obj_t *current_page = NULL;
static int saved_page_index = 0;  // Remember page selection across reinits
static lv_obj_t *menu_container = NULL;
static const char *section_names[] = {
    "Status", "Game Controller", "Basic settings",
    "Levelling Mode", "Auxillary Output", "Units", "Screen Settings", "Config", "Wifi / Update"
};
static constexpr int kSettingsSectionNameCount = sizeof(section_names) / sizeof(section_names[0]);
static char s_sectionDropdownOpts[512];

static void fillSectionDropdownOptionsFromNames(void)
{
    int pos = 0;
    for (int i = 0; i < kSettingsSectionNameCount; i++) {
        if (i > 0 && pos < (int)sizeof(s_sectionDropdownOpts) - 1)
            s_sectionDropdownOpts[pos++] = '\n';
        for (const char *p = section_names[i]; *p && pos < (int)sizeof(s_sectionDropdownOpts) - 1; p++)
            s_sectionDropdownOpts[pos++] = *p;
    }
    s_sectionDropdownOpts[pos] = '\0';
}

// Dropdown event handler
static void section_dropdown_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *dropdown = lv_event_get_target_obj(e);

    if (code == LV_EVENT_VALUE_CHANGED) {
        uint32_t id = lv_dropdown_get_selected(dropdown);
        ScrSettings *settings = (ScrSettings *)lv_event_get_user_data(e);
        if (settings->settingsPageCount <= 0 || id >= (uint32_t)settings->settingsPageCount)
            return;

        // Save page selection for reinit
        saved_page_index = (int)id;

        // Hide current page
        if (current_page) {
            lv_obj_add_flag(current_page, LV_OBJ_FLAG_HIDDEN);
        }

        // Show selected page
        current_page = (lv_obj_t *)settings->pages[id];
        if (current_page) {
            lv_obj_remove_flag(current_page, LV_OBJ_FLAG_HIDDEN);
            lv_obj_scroll_to_y(lv_obj_get_parent(current_page), 0, LV_ANIM_OFF);
        }
    }
    else if (code == LV_EVENT_READY) {
        Option::styleDropdownList(dropdown);
    }
}

// Event handlers
static void compressor_status_handler(void *data)
{
    bool value = (bool)data;
    CompressorStatusPacket pkt(value);
    sendRestPacket(&pkt);
    log_i("Pressed compressor status %i", value);
}

static void maintain_pressure_handler(void *data)
{
    bool value = (bool)data;
    setManifoldConfigValuesFlag(ConfigFlagsBit::CONFIG_MAINTAIN_PRESSURE, value);
    log_i("Pressed maintain pressure %i", value);
}

static void sensorless_leveling_handler(void *data)
{
    bool value = (bool)data;
    setManifoldConfigValuesFlag(ConfigFlagsBit::CONFIG_SENSORLESS_LEVELING, value);
    log_i("Pressed sensorless leveling %i", value);
}

static void rise_on_start_handler(void *data)
{
    bool value = (bool)data;
    setManifoldConfigValuesFlag(ConfigFlagsBit::CONFIG_RISE_ON_START, value);
    log_i("Pressed riseonstart %i", value);
}

static void fall_on_shutdown_handler(void *data)
{
    bool value = (bool)data;
#if ENABLE_AIR_OUT_ON_SHUTOFF
    setManifoldConfigValuesFlag(ConfigFlagsBit::CONFIG_AIR_OUT_ON_SHUTOFF, value);
#endif
    log_i("Pressed fallonshutdown %i", value);
}

static void safety_mode_handler(void *data)
{
    bool value = (bool)data;
    setManifoldConfigValuesFlag(ConfigFlagsBit::CONFIG_SAFETY_MODE, value);
    log_i("Pressed safetymode %i", value);
}


static void aux_output_switch_handler(void *data)
{
    bool on = (bool)data;
    AuxillaryOutputControlPacket pkt(on);
    sendRestPacket(&pkt);
}

static void aux_hold_output_handler(void *data)
{
    bool on = ((uintptr_t)data != 0u);
    AuxillaryOutputControlPacket pkt(on);
    sendRestPacket(&pkt);
    if (!on && scrSettings.ui_auxOutputSwitch != NULL) {
        scrSettings.ui_auxOutputSwitch->setBooleanValue(false, false);
    }
}

static void aux_time_unit_handler(void *data)
{
    uintptr_t sel = (uintptr_t)data;
    AuxillaryOutputModePayload *cfg = util_configValues._auxillaryOutputConfig();
    if (sel > (uintptr_t)AUX_MODE_TIME_HOURS)
        return;
    if (cfg->timeUnit == (AuxillaryOutputModeTimeUnit)sel)
        return;
    cfg->timeUnit = (AuxillaryOutputModeTimeUnit)sel;
    sendConfigValuesPacket(true);
    alertValueUpdated();
}

static void aux_mode_startup_handler(void *data)
{
    bool on = (bool)data;
    AuxillaryOutputModePayload *cfg = util_configValues._auxillaryOutputConfig();
    uint8_t m = (uint8_t)cfg->mode;
    if (on)
        m |= (uint8_t)(1u << AUX_MODE_STARTUP_TIMED);
    else
        m &= (uint8_t)~(1u << AUX_MODE_STARTUP_TIMED);
    cfg->mode = (AuxillaryOutputMode)m;
    sendConfigValuesPacket(true);
    alertValueUpdated();
}

static void aux_mode_shutdown_handler(void *data)
{
    bool on = (bool)data;
    AuxillaryOutputModePayload *cfg = util_configValues._auxillaryOutputConfig();
    uint8_t m = (uint8_t)cfg->mode;
    if (on)
        m |= (uint8_t)(1u << AUX_MODE_SHUTDOWN_TIMED);
    else
        m &= (uint8_t)~(1u << AUX_MODE_SHUTDOWN_TIMED);
    cfg->mode = (AuxillaryOutputMode)m;
    sendConfigValuesPacket(true);
    alertValueUpdated();
}

void ScrSettings::updateLevelModeOptionsVisibility(bool isLevelMode)
{
    if (isLevelMode) {
        lv_obj_remove_flag(this->ui_calibrateMinHeight->root, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(this->ui_calibrateMaxHeight->root, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(this->ui_calibrateMinRideHeight->root, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(this->ui_sensorlessleveling->root, LV_OBJ_FLAG_HIDDEN);
        if (this->ui_config7) lv_obj_add_flag(this->ui_config7->root, LV_OBJ_FLAG_HIDDEN);
        if (this->ui_config8) lv_obj_add_flag(this->ui_config8->root, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(this->ui_maintainprssure->text, "Maintain Height");
    } else {
        lv_obj_add_flag(this->ui_calibrateMinHeight->root, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(this->ui_calibrateMaxHeight->root, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(this->ui_calibrateMinRideHeight->root, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(this->ui_sensorlessleveling->root, LV_OBJ_FLAG_HIDDEN);
        if (this->ui_config7) lv_obj_remove_flag(this->ui_config7->root, LV_OBJ_FLAG_HIDDEN);
        if (this->ui_config8) lv_obj_remove_flag(this->ui_config8->root, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(this->ui_maintainprssure->text, "Maintain Pressure");
    }
}

void ScrSettings::updateUpdateButtonVisbility()
{
    if (getwifiSSID().length() > 0 && getwifiPassword().length() > 0)
    {
        lv_obj_remove_flag(this->ui_updateBtn->root, (lv_obj_flag_t)(LV_OBJ_FLAG_HIDDEN));
    }
    else
    {
        lv_obj_add_flag(this->ui_updateBtn->root, (lv_obj_flag_t)(LV_OBJ_FLAG_HIDDEN));
    }
}

void styleSettingsSectionDropdown(lv_obj_t *dd)
{
    lv_obj_set_style_bg_color(dd, lv_color_hex(0x161A1F), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(dd, LV_OPA_COVER, LV_PART_MAIN);

    lv_obj_set_style_border_width(dd, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(dd, lv_color_hex(0x2A313A), LV_PART_MAIN);

    lv_obj_set_style_radius(dd, 12, LV_PART_MAIN);

    lv_obj_set_style_text_color(dd, lv_color_hex(0xF2F4F7), LV_PART_MAIN);
#ifdef SCREEN_MODE_CIRCLE
    lv_obj_set_style_text_font(dd, getScaledFont(14), LV_PART_MAIN);
    lv_obj_set_style_pad_left(dd, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_right(dd, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_top(dd, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(dd, 8, LV_PART_MAIN);
#else
    lv_obj_set_style_text_font(dd, getScaledFont(20), LV_PART_MAIN);
    lv_obj_set_style_pad_left(dd, 14, LV_PART_MAIN);
    lv_obj_set_style_pad_right(dd, 14, LV_PART_MAIN);
    lv_obj_set_style_pad_top(dd, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(dd, 10, LV_PART_MAIN);
#endif

    lv_obj_set_style_text_color(dd, lv_color_hex(0xC9D0D8), LV_PART_INDICATOR);
}

void ScrSettings::init(lv_obj_t *parent)
{
    Scr::init(parent);

    // Clear tracking vectors for fresh init
    allOptions.clear();
    allRadioOptions.clear();

    // Reset header style to pick up new scale values after rotation
    Option::resetHeaderStyle();

    int scrW = getScreenWidth();
    int scrH = getScreenHeight();

#ifdef SCREEN_MODE_CIRCLE
    const int topInset = 44;
    const int bottomInset = 40;
    const int usableH = scrH - topInset - bottomInset;
    const int containerW = scrW - 40;
    const int menuBarHeight = 46;
#else
    const int topInset = STATUSBAR_HEIGHT;
    const int usableH = scrH - NAVBAR_HEIGHT - STATUSBAR_HEIGHT;
    const int containerW = scrW;
    const int menuBarHeight = scaledY(54);
#endif

    menu_container = lv_obj_create(this->scr);
    lv_obj_remove_style_all(menu_container);
    lv_obj_set_size(menu_container, containerW, usableH);
    lv_obj_align(menu_container, LV_ALIGN_TOP_MID, 0, topInset);
    lv_obj_set_style_bg_opa(menu_container, LV_OPA_TRANSP, 0);
    lv_obj_set_layout(menu_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(menu_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(menu_container, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *menu_bar = lv_obj_create(menu_container);
    lv_obj_remove_style_all(menu_bar);
    lv_obj_set_size(menu_bar, LV_PCT(100), menuBarHeight);
    lv_obj_set_style_bg_opa(menu_bar, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_layout(menu_bar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(menu_bar, LV_FLEX_FLOW_ROW);
#ifdef SCREEN_MODE_CIRCLE
    lv_obj_set_flex_align(menu_bar, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_hor(menu_bar, 40, 0);
    lv_obj_set_style_pad_ver(menu_bar, 2, 0);
#else
    lv_obj_set_flex_align(menu_bar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(menu_bar, 6, 0);
#endif
    lv_obj_clear_flag(menu_bar, LV_OBJ_FLAG_SCROLLABLE);

    fillSectionDropdownOptionsFromNames();
    lv_obj_t *dropdown = lv_dropdown_create(menu_bar);
    lv_dropdown_set_options(dropdown, s_sectionDropdownOpts);
#ifdef SCREEN_MODE_CIRCLE
    lv_obj_set_width(dropdown, scrW - 100);
    lv_obj_set_height(dropdown, 40);
#else
    lv_obj_set_width(dropdown, scrW - scaledX(12));
    lv_obj_set_height(dropdown, scaledY(44));
#endif
    styleSettingsSectionDropdown(dropdown);

    lv_obj_add_event_cb(dropdown, section_dropdown_event_cb, LV_EVENT_VALUE_CHANGED, this);
    lv_obj_add_event_cb(dropdown, section_dropdown_event_cb, LV_EVENT_READY, this);

    lv_obj_t *pages_container = lv_obj_create(menu_container);
    lv_obj_remove_style_all(pages_container);
    lv_obj_set_size(pages_container, LV_PCT(100), usableH - menuBarHeight);
    lv_obj_align(pages_container, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(pages_container, LV_OPA_TRANSP, 0);
    lv_obj_set_layout(pages_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(pages_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollbar_mode(pages_container, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(pages_container, LV_DIR_VER);

    this->settingsPageCount = 0;
    for (int i = 0; i < kSettingsMaxPages; i++) {
        this->pages[i] = NULL;
    }

    // --- Status page ---
    lv_obj_t *status_page = this->addSettingsPage(pages_container, false);

    this->ui_s1 = new Option(status_page, OptionType::TEXT_WITH_VALUE, "Compressor Frozen:", {.STRING = test});
    this->ui_s3 = new Option(status_page, OptionType::TEXT_WITH_VALUE, "ACC Status:", {.STRING = test});
    this->ui_ebrakeStatus = new Option(status_page, OptionType::TEXT_WITH_VALUE,
        AIR_OUT_ON_SHUTOFF_DOUBLE_LOCK_MODE == true ? "Door Lock Status:" : "E-Brake Status:", {.STRING = test});
    this->ui_s2 = new Option(status_page, OptionType::ON_OFF, "Compressor Status:", {.INT = 0}, compressor_status_handler);

    this->ui_rebootbutton = new Option(status_page, OptionType::BUTTON, "Reboot/Turn Off", {.STRING = test}, [](void *data)
    {
        currentScr->showMsgBox("Reboot/Turn Off?", NULL, "Confirm", "Cancel",
            []() -> void
            {
                if (statusBittset & (1 << StatusPacketBittset::ACC_STATUS_ON))
                {
                    RebootPacket pkt;
                    sendRestPacket(&pkt);
                    log_i("Pressed reboot");
                    showDialog("Rebooting...", lv_color_hex(0xFFFF00));
                }
                else
                {
                    TurnOffPacket pkt;
                    sendRestPacket(&pkt);
                    log_i("Pressed turn off");
                    showDialog("Shutting down", lv_color_hex(0xFFFF00));
                }
            },
            []() -> void {}, false);
    });

    // --- Game Controller page ---
    lv_obj_t *game_controller_page = this->addSettingsPage(pages_container, true);

    allOptions.push_back(new Option(game_controller_page, OptionType::BUTTON, "Allow New Controller", {.STRING = test}, [](void *data)
    {
        currentScr->showMsgBox("Confirm?",
            "After clicking this, OASMan will become pairable and the next controller to try to pair with OASMan will be allowed to pair and remembered by OASMan.\nMax saved devices is 20",
            "Confirm", "Cancel",
            []() -> void
            {
                BP32Packet pkt(BP32CMD::BP32CMD_ENABLE_NEW_CONN, true);
                sendRestPacket(&pkt);
                showDialog("Connect your controller!", lv_color_hex(0xFFFF00));
            },
            []() -> void {}, false);
    }));

    allOptions.push_back(new Option(game_controller_page, OptionType::BUTTON, "Un-pair All Controllers", {.STRING = test}, [](void *data)
    {
        currentScr->showMsgBox("Confirm?",
            "After clicking this, all paired game controllers will be removed from memory, and actively connected ones will be disconnected. This also resets your saved devices back to 0.",
            "Confirm", "Cancel",
            []() -> void
            {
                BP32Packet pkt(BP32CMD::BP32CMD_FORGET_DEVICES, NULL);
                sendRestPacket(&pkt);
                showDialog("Controllers forgotten!", lv_color_hex(0xFFFF00));
            },
            []() -> void {}, false);
    }));

    allOptions.push_back(new Option(game_controller_page, OptionType::BUTTON, "Disconnect Controllers", {.STRING = test}, [](void *data)
    {
        currentScr->showMsgBox("Confirm?",
            "Some devices may be difficult to disconnect on their own, this will disconnect them for you. Hint: Pressing the 'system' button on supporting controllers will disconnect them.",
            "Confirm", "Cancel",
            []() -> void
            {
                BP32Packet pkt(BP32CMD::BP32CMD_DISCONNECT_DEVICES, NULL);
                sendRestPacket(&pkt);
                showDialog("Controllers disconnected!", lv_color_hex(0xFFFF00));
            },
            []() -> void {}, false);
    }));

    // --- Basic settings page ---
    lv_obj_t *basic_settings_page = this->addSettingsPage(pages_container, true);

    this->ui_maintainprssure = new Option(basic_settings_page, OptionType::ON_OFF, "Maintain Pressure", {.INT = 0}, maintain_pressure_handler);
    this->ui_sensorlessleveling = new Option(basic_settings_page, OptionType::ON_OFF, "Height levelling", {.INT = 0}, sensorless_leveling_handler);
    this->ui_riseonstart = new Option(basic_settings_page, OptionType::ON_OFF, "Rise on start", {.INT = 0}, rise_on_start_handler);

#if ENABLE_AIR_OUT_ON_SHUTOFF
    this->ui_airoutonshutoff = new Option(basic_settings_page, OptionType::ON_OFF, "Fall on shutdown", {.INT = 0}, fall_on_shutdown_handler);
#endif

    this->ui_safetymode = new Option(basic_settings_page, OptionType::ON_OFF, "Safety Mode", {.INT = 0}, safety_mode_handler);

#if ENABLE_DETECT_PRESSURE_SENSORS_BUTTON
    allOptions.push_back(new Option(basic_settings_page, OptionType::BUTTON, "Detect Pressure Sensors", {.STRING = test}, [](void *data)
    {
        currentScr->showMsgBox("Detect Pressure Sensors?",
            "WARNING: YOUR CAR WILL BE AIRED OUT!!!! This routine will auto learn which pressure sensors go to which wheels.",
            "Confirm", "Cancel",
            []() -> void
            {
                DetectPressureSensorsPacket pkt;
                sendRestPacket(&pkt);
                log_i("Pressed detected pressure sensors");
                showDialog("Doing detection routine", lv_color_hex(0xFFFF00));
            },
            []() -> void {}, false);
    }));
#endif

    allOptions.push_back(new Option(basic_settings_page, OptionType::HEADER, "Key Fob Settings", {.STRING = test}));
    allOptions.push_back(new Option(basic_settings_page, OptionType::BUTTON, "Unlearn Fob", {.STRING = test}, [](void *data)
    {
        currentScr->showMsgBox("Unlearn key fob?",
            "WARNING: Your key fob will be unlearned. This requires you have an OASMan Key Fob Receiver installed (RX480E receiver)",
            "Confirm", "Cancel",
            []() -> void
            {
                RfCommandPacket pkt(RfCommandType::RF_COMMAND_CHIP_CMD, RfCommandChipNumber::RF_CMD_DELETE, 0);
                sendRestPacket(&pkt);
                log_i("Pressed unlearn key fob");
                showDialog("Unlearning key fob...", lv_color_hex(0xFFFF00));
            },
            []() -> void {}, false);
    }));

    allOptions.push_back(new Option(basic_settings_page, OptionType::BUTTON, "Learn Fob", {.STRING = test}, [](void *data)
    {
        currentScr->showMsgBox("Learn fob?",
            "This requires you have an OASMan Key Fob Receiver installed (RX480E receiver)",
            "Confirm", "Cancel",
            []() -> void
            {
                RfCommandPacket pkt(RfCommandType::RF_COMMAND_CHIP_CMD, RfCommandChipNumber::RF_CMD_LEARN_MOMENTARY, 0);
                sendRestPacket(&pkt);
                log_i("Pressed learn key fob");
                showDialog("Learning key fob mode...", lv_color_hex(0xFFFF00));
            },
            []() -> void {}, false);
    }));

    this->ui_rfbuttonA = new Option(basic_settings_page, OptionType::SLIDER, "Button A Preset Number", {.INT = 0}, [](void *data)
    {
        RfCommandPacket pkt(RfCommandType::RF_COMMAND_BUTTON_ASSIGN, RfCommandButtonNumber::RF_BUTTON_A, (uint32_t)data - 1);
        sendRestPacket(&pkt);
    });
    ((Option *)this->ui_rfbuttonA)->setSliderParams(1, 5, true, LV_EVENT_RELEASED);
    this->ui_rfbuttonB = new Option(basic_settings_page, OptionType::SLIDER, "Button B Preset Number", {.INT = 0}, [](void *data)
    {
        RfCommandPacket pkt(RfCommandType::RF_COMMAND_BUTTON_ASSIGN, RfCommandButtonNumber::RF_BUTTON_B, (uint32_t)data - 1);
        sendRestPacket(&pkt);
    });
    ((Option *)this->ui_rfbuttonB)->setSliderParams(1, 5, true, LV_EVENT_RELEASED);
    this->ui_rfbuttonC = new Option(basic_settings_page, OptionType::SLIDER, "Button C Preset Number", {.INT = 0}, [](void *data)
    {
        RfCommandPacket pkt(RfCommandType::RF_COMMAND_BUTTON_ASSIGN, RfCommandButtonNumber::RF_BUTTON_C, (uint32_t)data - 1);
        sendRestPacket(&pkt);
    });
    ((Option *)this->ui_rfbuttonC)->setSliderParams(1, 5, true, LV_EVENT_RELEASED);
    this->ui_rfbuttonD = new Option(basic_settings_page, OptionType::SLIDER, "Button D Preset Number", {.INT = 0}, [](void *data)
    {
        RfCommandPacket pkt(RfCommandType::RF_COMMAND_BUTTON_ASSIGN, RfCommandButtonNumber::RF_BUTTON_D, (uint32_t)data - 1);
        sendRestPacket(&pkt);
    });
    ((Option *)this->ui_rfbuttonD)->setSliderParams(1, 5, true, LV_EVENT_RELEASED);

    // ML/AI rows (moved here from the retired ML/AI page; kept at the very end of Basic settings)
    this->ui_aiPercentage = new Option(basic_settings_page, OptionType::TEXT_WITH_VALUE, "Sample Learn Progress:", {.STRING = test});

    allOptions.push_back(new Option(basic_settings_page, OptionType::BUTTON, "Reset Learned Data", {.STRING = test}, [](void *data)
    {
        currentScr->showMsgBox("Reset Learned AI data?", "Run this if ai has completed training and you are getting innacurate presets.",
            "Confirm", "Cancel",
            []() -> void
            {
                ResetAIPacket pkt;
                sendRestPacket(&pkt);
                log_i("Pressed reset ai");
            },
            []() -> void {}, false);
    }));

    // --- Levelling Mode page ---
    lv_obj_t *levelling_page = this->addSettingsPage(pages_container, true);

    const char *levelTypeRadioText[2] = {"Pressure Sensor", "Level Sensor"};
    option_event_cb_t levelTypeRadioCB = [](void *data)
    {
        setManifoldConfigValuesFlag(ConfigFlagsBit::CONFIG_HEIGHT_SENSOR_MODE, ((bool)data));
    };
    this->ui_heightsensormode = new RadioOption(levelling_page, levelTypeRadioText, 2, levelTypeRadioCB);

    this->ui_calibrateMinHeight = new Option(levelling_page, OptionType::BUTTON, "Calibrate Min Height", {.STRING = test}, [](void *data)
    {
        currentScr->showMsgBox("Calibrate Min Height?",
            "Please air out your car to the lowest it goes before you click ok",
            "OK", "Cancel",
            []() -> void
            {
                CalibrateHeightSensorsPacket pkt(HEIGHT_CAL_MIN);
                sendRestPacket(&pkt);
                log_i("Pressed calibrate min height");
                showDialog("Calibrated min height", lv_color_hex(0xFFFF00));
            },
            []() -> void {}, false);
    });

    this->ui_calibrateMaxHeight = new Option(levelling_page, OptionType::BUTTON, "Calibrate Max Height", {.STRING = test}, [](void *data)
    {
        currentScr->showMsgBox("Calibrate Max Height?",
            "Raise your vehicle as high as it can go before you click ok",
            "OK", "Cancel",
            []() -> void
            {
                CalibrateHeightSensorsPacket pkt(HEIGHT_CAL_MAX);
                sendRestPacket(&pkt);
                log_i("Pressed calibrate max height");
                showDialog("Calibrated max height", lv_color_hex(0xFFFF00));
            },
            []() -> void {}, false);
    });

    this->ui_calibrateMinRideHeight = new Option(levelling_page, OptionType::BUTTON, "Calibrate Min Ride Height", {.STRING = test}, [](void *data)
    {
        currentScr->showMsgBox("Calibrate Minimum Ride Height?",
            "Set your vehicle to the lowest ride height you want to allow before you click ok. This is used for maintain height (height sensor only). Only use this after calibrating min and max.",
            "OK", "Cancel",
            []() -> void
            {
                CalibrateHeightSensorsPacket pkt(HEIGHT_CAL_MIN_RIDE_HEIGHT);
                sendRestPacket(&pkt);
                log_i("Pressed calibrate min ride height");
                showDialog("Calibrated min ride height", lv_color_hex(0xFFFF00));
            },
            []() -> void {}, false);
    });

    this->updateLevelModeOptionsVisibility(false);

    // --- Auxillary Output page ---
    lv_obj_t *aux_page = this->addSettingsPage(pages_container, true);

    // allOptions.push_back(new Option(aux_page, OptionType::HEADER, "Manual control", {.STRING = test}));

    this->ui_auxHoldButton = new Option(aux_page, OptionType::BUTTON, "Hold: Aux output on", {.STRING = test}, aux_hold_output_handler,
        (void *)(uintptr_t)Option::kButtonMomentaryExtraTag);

    this->ui_auxOutputSwitch = new Option(aux_page, OptionType::ON_OFF, "Aux output", {.INT = 0}, aux_output_switch_handler);

    // allOptions.push_back(new Option(aux_page, OptionType::HEADER, "Saved to manifold", {.STRING = test}));
    this->ui_auxModeStartup = new Option(aux_page, OptionType::ON_OFF, "Timed pulse on startup", {.INT = 0}, aux_mode_startup_handler);
    this->ui_auxModeShutdown = new Option(aux_page, OptionType::ON_OFF, "Timed pulse on shutdown", {.INT = 0}, aux_mode_shutdown_handler);

    static const char auxTimeUnitOpts[] = "Deciseconds\nSeconds\nMinutes\nHours";
    this->ui_auxTimeUnit = new Option(aux_page, OptionType::DROPDOWN_SELECT, "Duration unit:", {.INT = 0},
        aux_time_unit_handler, (void *)auxTimeUnitOpts);

    this->ui_auxPulseDuration = new Option(aux_page, OptionType::KEYBOARD_INPUT_NUMBER, "Pulse duration", {.INT = 0}, [](void *data)
    {
        uint32_t v = (uint32_t)data;
        if (v > 255U)
            v = 255U;
        util_configValues._auxillaryOutputConfig()->time = (uint8_t)v;
        sendConfigValuesPacket(true);
        alertValueUpdated();
    });

    this->ui_auxIntervalCycles = new Option(aux_page, OptionType::KEYBOARD_INPUT_NUMBER, "Interval (cycles)", {.INT = 0}, [](void *data)
    {
        uint32_t v = (uint32_t)data;
        if (v > 255U)
            v = 255U;
        util_configValues._auxillaryOutputConfig()->interval = (uint8_t)v;
        sendConfigValuesPacket(true);
        alertValueUpdated();
    });

    // --- Units page ---
    lv_obj_t *units_page = this->addSettingsPage(pages_container, true);

    const char *unitsRadioText[2] = {"PSI", "Bar"};
    option_event_cb_t unitsRadioCB = [](void *data) { setunitsMode((int)data); };
    allRadioOptions.push_back(new RadioOption(units_page, unitsRadioText, 2, unitsRadioCB, getunitsMode()));

    // --- Screen Settings page ---
    lv_obj_t *screen_settings_page = this->addSettingsPage(pages_container, true);

    allOptions.push_back(new Option(screen_settings_page, OptionType::KEYBOARD_INPUT_NUMBER, "Dim Screen (Minutes)", {.INT = (int)getscreenDimTimeM()}, [](void *data)
    {
        log_i("Pressed %i", ((uint32_t)data));
        setscreenDimTimeM((uint32_t)data);
    }));

    this->ui_brightnessSlider = new Option(screen_settings_page, OptionType::SLIDER, "Brightness", {.INT = getbrightness()}, [](void *data)
    {
        log_i("Brightness %i", ((uint32_t)data));
        setbrightness((uint32_t)data);
        set_brightness(getBrightnessFloat());
    });
    ((Option *)this->ui_brightnessSlider)->setSliderParams(1, 100, false, LV_EVENT_VALUE_CHANGED);

    allOptions.push_back(new Option(screen_settings_page, OptionType::HEADER, "Status Bar", {.STRING = ""}));
    allOptions.push_back(new Option(screen_settings_page, OptionType::ON_OFF, "Show Battery", {.INT = getshowBattery() ? 1 : 0}, [](void *data)
    {
        bool enabled = (bool)data;
        setshowBattery(enabled);
#ifdef SCREEN_MODE_CIRCLE
        circleStatusbarMini.setBatteryVisible(enabled);
#else
        globalStatusbar.setBatteryVisible(enabled);
#endif
    }));

#ifndef SCREEN_MODE_CIRCLE
    allOptions.push_back(new Option(screen_settings_page, OptionType::HEADER, "Navigation", {.STRING = ""}));
    allOptions.push_back(new Option(screen_settings_page, OptionType::ON_OFF, "Swipe Navigation", {.INT = getswipeNavigation() ? 1 : 0}, [](void *data)
    {
        bool enabled = (bool)data;
        setswipeNavigation(enabled);
        globalNavbar.setSwipeEnabled(enabled);
    }));
#endif

    #if SUPPORTS_ROTATION == 1
    
    // Screen rotation setting - single button that toggles between Portrait/Landscape
    allOptions.push_back(new Option(screen_settings_page, OptionType::HEADER, "Screen Orientation", {.STRING = ""}));

    
    this->ui_screenRotation = new Option(screen_settings_page, OptionType::BUTTON,
        getscreenRotation() == 0 ? "Switch to Landscape" : "Switch to Portrait",
        {.STRING = ""}, [](void *data)
    {
        byte currentRotation = getscreenRotation();
        byte newRotation = (currentRotation == 0) ? 1 : 0;
        setscreenRotation(newRotation);
        ScrSettings *settings = (ScrSettings *)currentScr;
        settings->ui_screenRotation->setRightHandText(newRotation == 0 ? "Switch to Landscape" : "Switch to Portrait");
        // Schedule screen reinit for next frame to allow rotation to complete
        runNextFrame([]() -> void {
            reinitializeScreens();
        });
    });
    #endif

    // Theme colors setting
    allOptions.push_back(new Option(screen_settings_page, OptionType::HEADER, "Theme Colors", {.STRING = ""}));

    const char *themePresetText[4] = {"Ocean Blue", "Plump Purple", "Forest Green", "Desert Sand"};
    option_event_cb_t themePresetCB = [](void *data)
    {
        int presetId = (int)(intptr_t)data;
        applyThemePreset((ThemePreset)presetId);
        runNextFrame([]() { reinitializeScreens(); });
    };
    this->ui_themePreset = new RadioOption(screen_settings_page, themePresetText, 4, themePresetCB, getCurrentThemePreset() >= 0 ? getCurrentThemePreset() : 0);

    // Custom color picker button
    allOptions.push_back(new Option(screen_settings_page, OptionType::BUTTON, "Custom Color Picker", {.STRING = ""}, [](void *data) {
        scrSettings.showColorPickerModal();
    }));

    #ifndef SCREEN_MODE_CIRCLE
    new Option(screen_settings_page, OptionType::BUTTON, "Upload custom car (USB)", {.STRING = test}, [](void *data)
    {
        currentScr->showMsgBox("Upload custom car images?",
            "Wait on this screen and open the OAS-Man Car Creator web tool (oasman.dev/controllerCarCreator) in your browser and follow the instructions on the final step there. Click 'Start' when it says to.",
            "Start", "Cancel",
            []() -> void
            {
                log_i("Car image upload mode — waiting for Web Serial");
                serialImageUploadRun();
                delay(250);
                ESP.restart();
            },
            []() -> void {}, false);
    });

    new Option(screen_settings_page, OptionType::BUTTON, "Clear custom car images", {.STRING = test}, [](void *data)
    {
        currentScr->showMsgBox("Clear custom car?",
            "This removes USB-uploaded car and wheel images from flash. Default preset graphics will be used after reboot.",
            "Clear", "Cancel",
            []() -> void
            {
                customCarClear();
                runNextFrame([]() -> void
                {
                    delay(250);
                    ESP.restart();
                });
            },
            []() -> void {}, false);
    });
#endif

    // --- Config page ---
    lv_obj_t *config_page = this->addSettingsPage(pages_container, true);

    this->ui_config1 = new Option(config_page, OptionType::SLIDER, "Bag Max PSI", {.INT = 200}, [](void *data)
    {
        log_i("Pressed %i", ((uint32_t)data));
        *util_configValues._bagMaxPressure() = (uint32_t)data;
        sendConfigValuesPacket(true);
        alertValueUpdated();
    });
    ((Option *)this->ui_config1)->setSliderParams(1, 256, true, LV_EVENT_RELEASED);

    allOptions.push_back(new Option(config_page, OptionType::KEYBOARD_INPUT_NUMBER, "Bluetooth Passkey (6 digits)", {.INT = (int)getblePasskey()}, [](void *data)
    {
        log_i("Pressed %i", (data));
        setblePasskey((uint32_t)data);
        if (isConnectedToManifold()) {
            AuthPacket pkt(getblePasskey(), AuthResult::AUTHRESULT_UPDATEKEY);
            sendRestPacket(&pkt);
        } else {
            authblacklist.clear();
        }
        alertValueUpdated();
    }));

    this->ui_config2 = new Option(config_page, OptionType::KEYBOARD_INPUT_NUMBER, "Shutoff Time (Minutes)", {.INT = 0}, [](void *data)
    {
        log_i("Pressed %i", ((uint32_t)data));
        *util_configValues._systemShutoffTimeM() = (uint32_t)data;
        sendConfigValuesPacket(true);
        alertValueUpdated();
    });

    this->ui_config3 = new Option(config_page, OptionType::KEYBOARD_INPUT_NUMBER, "Compressor On PSI", {.INT = 0}, [](void *data)
    {
        log_i("Pressed %i", ((uint32_t)data));
        *util_configValues._compressorOnPSI() = (uint32_t)data;
        sendConfigValuesPacket(true);
        alertValueUpdated();
    });

    this->ui_config4 = new Option(config_page, OptionType::KEYBOARD_INPUT_NUMBER, "Compressor Off PSI", {.INT = 0}, [](void *data)
    {
        log_i("Pressed %i", ((uint32_t)data));
        *util_configValues._compressorOffPSI() = (uint32_t)data;
        sendConfigValuesPacket(true);
        alertValueUpdated();
    });

    this->ui_config5 = new Option(config_page, OptionType::KEYBOARD_INPUT_NUMBER, "Pressure Sensor Rating PSI", {.INT = 0}, [](void *data)
    {
        log_i("Pressed %i", ((uint32_t)data));
        *util_configValues._pressureSensorMax() = (uint32_t)data;
        sendConfigValuesPacket(true);
        alertValueUpdated();
    });

    this->ui_config6 = new Option(config_page, OptionType::SLIDER, "Bag Volume Percentage", {.INT = 100}, [](void *data)
    {
        log_i("Pressed %i", ((uint32_t)data));
        *util_configValues._bagVolumePercentage() = (uint32_t)data;
        sendConfigValuesPacket(true);
        alertValueUpdated();
    });
    ((Option *)this->ui_config6)->setSliderParams(10, 600, true, LV_EVENT_RELEASED);

    this->ui_config7 = new Option(config_page, OptionType::KEYBOARD_INPUT_NUMBER, "Bag Stretch Below PSI", {.INT = 0}, [](void *data)
    {
        log_i("Pressed %i", ((uint32_t)data));
        *util_configValues._AirUpBagStretchTriggerBelowPressure() = (uint32_t)data;
        sendConfigValuesPacket(true);
        alertValueUpdated();
    });

    this->ui_config8 = new Option(config_page, OptionType::KEYBOARD_INPUT_NUMBER, "Bag Stretch PSI", {.INT = 0}, [](void *data)
    {
        log_i("Pressed %i", ((uint32_t)data));
        *util_configValues._AirUpBagStretchPressure() = (uint32_t)data;
        sendConfigValuesPacket(true);
        alertValueUpdated();
    });

    // --- Wifi / Update page ---
    lv_obj_t *wifi_update_page = this->addSettingsPage(pages_container, true);

    char buf[50];

    // SSID selection is a dropdown that scans for nearby networks when opened.
    // The first option is always the currently saved SSID (so the closed value stays put when
    // the scan results change), or a "Select network" placeholder when nothing is saved yet.
    this->scannedSSIDs.clear();
    static char ssidInitOpts[64];
    // note on these below, we are not attaching a value to the 'scanning...' options, but if you click 'select network' it will actually reset it to blank.
    if (getwifiSSID().length() > 0)
    {
        snprintf(ssidInitOpts, sizeof(ssidInitOpts), "%s\nUnselect network\nScanning...", getwifiSSID().c_str());
        this->scannedSSIDs.push_back(getwifiSSID());
        this->scannedSSIDs.push_back(String(""));
    }
    else
    {
        snprintf(ssidInitOpts, sizeof(ssidInitOpts), "Select network\nScanning...");
    }

    this->ui_wifiSSID = new Option(wifi_update_page, OptionType::DROPDOWN_SELECT, "SSID", {.INT = 0}, [](void *data)
    {
        uint32_t idx = (uint32_t)(uintptr_t)data;
        ScrSettings *s = (ScrSettings *)currentScr;
        // Ignore the "Select network" placeholder (empty string); only real SSIDs are saved.
        if (idx < s->scannedSSIDs.size())
        {
            log_i("Selected SSID %s", s->scannedSSIDs[idx].c_str());
            setwifiSSID(s->scannedSSIDs[idx].c_str());
            alertValueUpdated();
            s->updateUpdateButtonVisbility();
        }
    }, (void *)ssidInitOpts);
    allOptions.push_back(this->ui_wifiSSID);

    // Widen the SSID dropdown: give the short "SSID" label a small fixed area and let the
    // dropdown take the rest of the row (keeps its right edge at the -MARGIN alignment set
    // by the Option constructor).
    {
        const int margin = scaledX(10);
        const int labelW = scaledX(60);
        lv_obj_set_width(this->ui_wifiSSID->text, labelW);
        lv_obj_set_width(this->ui_wifiSSID->rightHandObj, getScreenWidth() - margin * 3 - labelW);
    }

    // Right-align the open list after LVGL opens it (this user callback runs after the
    // dropdown's own class handler, which opens the list on release).
    lv_obj_add_event_cb(this->ui_wifiSSID->rightHandObj, [](lv_event_t *e)
    {
        lv_obj_t *dd = lv_event_get_target_obj(e);
        if (lv_dropdown_is_open(dd))
            alignWifiSsidList(dd);
    }, LV_EVENT_RELEASED, this);

    // Kick off an async WiFi scan when the dropdown is opened.
    lv_obj_add_event_cb(this->ui_wifiSSID->rightHandObj, [](lv_event_t *e)
    {
        ScrSettings *s = (ScrSettings *)currentScr;
        if (s->wifiScanInProgress)
            return;
        s->wifiScanInProgress = true;
        //lv_dropdown_set_options(s->ui_wifiSSID->rightHandObj, "Scanning...");
        WiFi.mode(WIFI_STA);
        WiFi.scanNetworks(true /* async */, false /* show hidden */);
    }, LV_EVENT_CLICKED, this);

    strncpy(buf, getwifiPassword().c_str(), sizeof(buf));
    OptionValue wifiOptionValue;
    wifiOptionValue.STRING = buf;

    Option *pass = new Option(wifi_update_page, OptionType::KEYBOARD_INPUT_TEXT, "PASS", wifiOptionValue, [](void *data)
    {
        log_i("Typed %s", ((char *)data));
        setwifiPassword(((char *)data));
        alertValueUpdated();
        ((ScrSettings *)currentScr)->updateUpdateButtonVisbility();
    });
    allOptions.push_back(pass);

    lv_textarea_set_password_mode(pass->rightHandObj, true);
    lv_textarea_set_password_show_time(pass->rightHandObj, 10000);

    this->ui_updateBtn = new Option(wifi_update_page, OptionType::BUTTON, "Start Software Update", {.STRING = test}, [](void *data)
    {
        currentScr->showMsgBox("Begin update wifi service?",
            "This will use the wifi credentials you entered above to download and install the latest firmware update on both the manifold and controller. If an issue occurs, please go to http://oasman.dev and flash manually. Continue?",
            "Start", "Cancel",
            []() -> void
            {
                StartwebPacket pkt(getwifiSSID(), getwifiPassword());
                sendRestPacket(&pkt);
                log_i("Starting web service");
#if defined(OTA_SUPPORTED)
                runNextFrame([]() -> void
                {
                    currentScr->showMsgBox("Updating in progress...",
                        "Both the manifold & controller are installing their updates. Both will reboot when completed.",
                        NULL, "OK", []() -> void {}, []() -> void {}, false);
                    runNextFrame([]() -> void
                    {
                        delay(250);
                        setupdateMode(true);
                        runNextFrame([]() -> void { ESP.restart(); });
                    });
                    log_i("Attempted to download update");
                });
#else
                currentScr->showMsgBox("Updating in progress...",
                    "The manifold is installing the latest update. Your controller does not support OTA updates. Please go to http://oasman.dev on your computer to flash the latest update to your controller.",
                    NULL, "OK",
                    []() -> void { ESP.restart(); },
                    []() -> void { ESP.restart(); }, true);
#endif
            },
            []() -> void {}, false);
    });

    updateUpdateButtonVisbility();
    this->ui_manifoldUpdateStatus = new Option(wifi_update_page, OptionType::TEXT_WITH_VALUE, "Manifold:", {.STRING = test});

    // QR Code - scaled for display size
    const int qrSize = scaledX(100);
    lv_obj_t *qrCodeParent = lv_obj_create(wifi_update_page);
    lv_obj_remove_style_all(qrCodeParent);
    lv_obj_set_size(qrCodeParent, scrW, qrSize);

    this->ui_qrcode = lv_qrcode_create(qrCodeParent);
    lv_qrcode_set_size(this->ui_qrcode, qrSize);
    lv_qrcode_set_dark_color(this->ui_qrcode, lv_color_black());
    lv_qrcode_set_light_color(this->ui_qrcode, lv_color_white());

    const char *qr_data = "https://oasman.dev";
    lv_qrcode_update(this->ui_qrcode, qr_data, strlen(qr_data));
    lv_obj_set_x(this->ui_qrcode, scrW / 2 - qrSize / 2);

    // Version and info
    OptionValue versionValue;
#ifdef OFFICIAL_RELEASE
    versionValue.STRING = EVALUATE_AND_STRINGIFY(RELEASE_VERSION);
#else
    versionValue.STRING = "DEVELOPMENT";
#endif
    allOptions.push_back(new Option(wifi_update_page, OptionType::TEXT_WITH_VALUE, "Version:", versionValue));
    this->ui_mac = new Option(wifi_update_page, OptionType::TEXT_WITH_VALUE, "Manifold:", {.STRING = ble_getMAC()});
    this->ui_volts = new Option(wifi_update_page, OptionType::TEXT_WITH_VALUE, "Battery:", {.STRING = getBatteryVoltageString()});

    // Restore previously selected page (or default to Status)
    if (saved_page_index < 0 || saved_page_index >= this->settingsPageCount) {
        saved_page_index = 0;
    }
    if (this->settingsPageCount != kSettingsSectionNameCount) {
        log_e("ScrSettings: page count %i != section name count %i",
            this->settingsPageCount, kSettingsSectionNameCount);
    }
    lv_dropdown_set_selected(dropdown, saved_page_index);
    current_page = this->pages[saved_page_index];

    // Hide all pages except the selected one
    for (int i = 0; i < this->settingsPageCount; i++) {
        if (this->pages[i] != NULL) {
            if (i == saved_page_index) {
                lv_obj_remove_flag(this->pages[i], LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_add_flag(this->pages[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
    }

    sendConfigValuesPacket(false);
}


void ScrSettings::loop()
{
    Scr::loop();

    // Poll the async WiFi scan started when the SSID dropdown was opened.
    if (this->wifiScanInProgress)
    {
        int n = WiFi.scanComplete();
        if (n >= 0)
        {
            this->wifiScanInProgress = false;
            this->scannedSSIDs.clear();
            String opts;
            String saved = getwifiSSID();

            // First option is always the saved SSID (kept selected below so the displayed
            // value doesn't jump when scan results change), or a placeholder when blank.
            if (saved.length() > 0)
            {
                opts = saved;
            } else {
                opts = "Select network";
            }
            this->scannedSSIDs.push_back(saved);

            for (int i = 0; i < n; i++)
            {
                String ss = WiFi.SSID(i);
                if (ss.length() == 0)
                    continue; // skip hidden networks
                bool dup = false;
                for (size_t j = 0; j < this->scannedSSIDs.size(); j++)
                {
                    if (this->scannedSSIDs[j] == ss)
                    {
                        dup = true;
                        break;
                    }
                }
                if (dup)
                    continue; // already listed (incl. the saved SSID pinned first)
                
                opts += "\n";
                opts += ss;
                this->scannedSSIDs.push_back(ss);
            }

            // no results found and no saved ssid
            if (this->scannedSSIDs.size() == 1)
            {
                opts += "No networks found!";
            }

            lv_dropdown_set_options(this->ui_wifiSSID->rightHandObj, opts.c_str());
            // Keep the pinned saved SSID / placeholder (index 0) selected so the closed value stays put.
            lv_dropdown_set_selected(this->ui_wifiSSID->rightHandObj, 0);

            // Refresh the open list so the scanned results replace "Scanning...".
            if (lv_dropdown_is_open(this->ui_wifiSSID->rightHandObj))
            {
                lv_dropdown_close(this->ui_wifiSSID->rightHandObj);
                lv_dropdown_open(this->ui_wifiSSID->rightHandObj);
                alignWifiSsidList(this->ui_wifiSSID->rightHandObj);
            }

            WiFi.scanDelete();
            // Release the WiFi driver's internal RAM / coexistence load so the BLE link to
            // the manifold stays stable between scans. A fresh scan re-inits on next open.
            WiFi.mode(WIFI_OFF);
        }
    }

    // Update status text labels
    this->ui_s1->setRightHandText(statusBittset & (1 << StatusPacketBittset::COMPRESSOR_FROZEN) ? "Yes" : "No");
    this->ui_s3->setRightHandText(statusBittset & (1 << StatusPacketBittset::ACC_STATUS_ON) ? "On" : "Off");
    this->ui_ebrakeStatus->setRightHandText(statusBittset & (1 << StatusPacketBittset::EBRAKE_STATUS_ON) ? "On" : "Off");

    // Update reboot button text
    this->ui_rebootbutton->setRightHandText(statusBittset & (1 << StatusPacketBittset::ACC_STATUS_ON) ? "Reboot" : "Shut Down");

    // Update compressor status (live status from bstatusBittset; override command compressor packet)
    this->ui_s2->setBooleanValue(statusBittset & (1 << StatusPacketBittset::COMPRESSOR_STATUS_ON), false);


    // Update AI status
    if (util_statusRequestPacket._setStatus)
    {
        util_statusRequestPacket._setStatus = false;
        this->ui_manifoldUpdateStatus->setRightHandText(util_statusRequestPacket.getStatus().c_str());
    }

    char buf[40];
    snprintf(buf, sizeof(buf), "%i%%", AIPercentage);
    this->ui_aiPercentage->setRightHandText(buf);

    this->ui_mac->setRightHandText(ble_getMAC());
    this->ui_volts->setRightHandText(getBatteryVoltageString());

    // Update config values
    if (*util_configValues._setValues())
    {
        *util_configValues._setValues() = false;
        this->ui_config1->setRightHandText(itoa(*util_configValues._bagMaxPressure(), buf, 10));
        this->ui_config2->setRightHandText(itoa(*util_configValues._systemShutoffTimeM(), buf, 10));
        this->ui_config3->setRightHandText(itoa(*util_configValues._compressorOnPSI(), buf, 10));
        this->ui_config4->setRightHandText(itoa(*util_configValues._compressorOffPSI(), buf, 10));
        this->ui_config5->setRightHandText(itoa(*util_configValues._pressureSensorMax(), buf, 10));
        this->ui_config6->setRightHandText(itoa(*util_configValues._bagVolumePercentage(), buf, 10));
        this->ui_config7->setRightHandText(itoa(*util_configValues._AirUpBagStretchTriggerBelowPressure(), buf, 10));
        this->ui_config8->setRightHandText(itoa(*util_configValues._AirUpBagStretchPressure(), buf, 10));

        this->ui_rfbuttonA->setRightHandText(itoa(*util_configValues._rfButtonA() + 1, buf, 10));
        this->ui_rfbuttonB->setRightHandText(itoa(*util_configValues._rfButtonB() + 1, buf, 10));
        this->ui_rfbuttonC->setRightHandText(itoa(*util_configValues._rfButtonC() + 1, buf, 10));
        this->ui_rfbuttonD->setRightHandText(itoa(*util_configValues._rfButtonD() + 1, buf, 10));

        uint8_t flags = *util_configValues._configFlagsBits();
        this->ui_riseonstart->setBooleanValue((flags & (1 << ConfigFlagsBit::CONFIG_RISE_ON_START)) != 0, false);
        this->ui_maintainprssure->setBooleanValue((flags & (1 << ConfigFlagsBit::CONFIG_MAINTAIN_PRESSURE)) != 0, false);
        this->ui_sensorlessleveling->setBooleanValue((flags & (1 << ConfigFlagsBit::CONFIG_SENSORLESS_LEVELING)) != 0, false);
#if ENABLE_AIR_OUT_ON_SHUTOFF
        this->ui_airoutonshutoff->setBooleanValue((flags & (1 << ConfigFlagsBit::CONFIG_AIR_OUT_ON_SHUTOFF)) != 0, false);
#endif
        this->ui_safetymode->setBooleanValue((flags & (1 << ConfigFlagsBit::CONFIG_SAFETY_MODE)) != 0, false);
        bool heightSensorMode = (flags & (1 << ConfigFlagsBit::CONFIG_HEIGHT_SENSOR_MODE)) != 0;
        this->ui_heightsensormode->setSelectedOption(heightSensorMode ? 1 : 0);
        this->updateLevelModeOptionsVisibility(heightSensorMode);

        AuxillaryOutputModePayload *aux = util_configValues._auxillaryOutputConfig();
        this->ui_auxModeStartup->setBooleanValue((aux->mode & (1u << AUX_MODE_STARTUP_TIMED)) != 0, false);
        this->ui_auxModeShutdown->setBooleanValue((aux->mode & (1u << AUX_MODE_SHUTDOWN_TIMED)) != 0, false);

        unsigned tu = (unsigned)aux->timeUnit;
        if (tu > (unsigned)AUX_MODE_TIME_HOURS)
            tu = 0;
        this->ui_auxTimeUnit->setDropdownSelectedIndex(tu);

        this->ui_auxPulseDuration->setRightHandText(itoa(aux->time, buf, 10));
        this->ui_auxIntervalCycles->setRightHandText(itoa(aux->interval, buf, 10));
    }
}

void ScrSettings::cleanup()
{
    // Ensure any WiFi scan started from the SSID dropdown is torn down when leaving the
    // screen, so WiFi doesn't keep holding internal RAM / coexistence load.
    if (this->wifiScanInProgress)
    {
        WiFi.scanDelete();
        WiFi.mode(WIFI_OFF);
        this->wifiScanInProgress = false;
    }

    // Call base class cleanup first (deletes Alert)
    Scr::cleanup();

    // Delete stored Option/RadioOption members
    delete ui_s1;
    delete ui_s2;
    delete ui_s3;
    delete ui_ebrakeStatus;
    delete ui_rebootbutton;
    delete ui_aiPercentage;
    delete ui_maintainprssure;
    delete ui_sensorlessleveling;
    delete ui_riseonstart;
#if ENABLE_AIR_OUT_ON_SHUTOFF
    delete ui_airoutonshutoff;
#endif
    delete ui_safetymode;
    delete ui_heightsensormode;
    delete ui_calibrateMinHeight;
    delete ui_calibrateMaxHeight;
    delete ui_calibrateMinRideHeight;
    delete ui_config1;
    delete ui_config2;
    delete ui_config3;
    delete ui_config4;
    delete ui_config5;
    delete ui_config6;
    delete ui_updateBtn;
    delete ui_manifoldUpdateStatus;
    delete ui_mac;
    delete ui_volts;
    delete ui_brightnessSlider;
#if SUPPORTS_ROTATION == 1
    delete ui_screenRotation;
#endif
    delete ui_themePreset;
    delete ui_rfbuttonA;
    delete ui_rfbuttonB;
    delete ui_rfbuttonC;
    delete ui_rfbuttonD;

    delete ui_auxHoldButton;
    delete ui_auxOutputSwitch;
    delete ui_auxModeStartup;
    delete ui_auxModeShutdown;
    delete ui_auxTimeUnit;
    delete ui_auxPulseDuration;
    delete ui_auxIntervalCycles;

    // Delete vector-tracked objects (non-stored Options/RadioOptions)
    for (Option* opt : allOptions) {
        delete opt;
    }
    for (RadioOption* opt : allRadioOptions) {
        delete opt;
    }
    allOptions.clear();
    allRadioOptions.clear();
}
