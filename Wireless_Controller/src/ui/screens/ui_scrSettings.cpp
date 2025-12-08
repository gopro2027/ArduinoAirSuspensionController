#include "ui_scrSettings.h"

LV_IMG_DECLARE(navbar_settings);
ScrSettings scrSettings(navbar_settings, false);

void alertValueUpdated()
{
    showDialog("Set value", lv_color_hex(0xFFFF00));
}

const char *test = "";
extern bool isConnectedToManifold();

// Current page tracking
static lv_obj_t *current_page = NULL;
static lv_obj_t *menu_container = NULL;
static const char *section_names[] = {
    "Status", "Game Controller", "ML/AI", "Basic settings", 
    "Levelling Mode", "Units", "Controller Settings", "Config", "Wifi / Update"
};
static const int NUM_SECTIONS = 9;

// Helper function to style dropdown list
static void style_dropdown_list(lv_obj_t *dropdown)
{
    lv_obj_t *list = lv_dropdown_get_list(dropdown);
    if (list) {
        // Style the list container
        lv_obj_set_style_bg_color(list, lv_color_hex(THEME_COLOR_DARK), 0);
        lv_obj_set_style_bg_opa(list, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(list, lv_color_hex(THEME_COLOR_LIGHT), 0);
        lv_obj_set_style_border_width(list, 2, 0);
        lv_obj_set_style_radius(list, 0, 0); // Remove rounded corners
        lv_obj_set_style_text_font(list, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(list, lv_color_white(), 0);
        
        // Style selected item
        lv_obj_set_style_bg_color(list, lv_color_hex(THEME_COLOR_MEDIUM), LV_PART_SELECTED | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(list, LV_OPA_COVER, LV_PART_SELECTED | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(list, lv_color_white(), LV_PART_SELECTED | LV_STATE_DEFAULT);

        int dropdown_height = lv_obj_get_height(dropdown);
        
        // Style list items
        uint32_t child_cnt = lv_obj_get_child_cnt(list);
        for (uint32_t i = 0; i < child_cnt; i++) {
            lv_obj_t *child = lv_obj_get_child(list, i);
            if (child) {
                lv_obj_set_style_bg_opa(child, LV_OPA_TRANSP, 0);
                lv_obj_set_style_text_font(child, &lv_font_montserrat_20, 0);
                lv_obj_set_style_text_color(child, lv_color_white(), 0);
                // lv_obj_align(child, LV_ALIGN_TOP_LEFT, 0, 0);
                lv_obj_set_style_pad_all(child, 10, dropdown_height);
            }
        }
    }
}

// Dropdown event handler
static void section_dropdown_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *dropdown = lv_event_get_target_obj(e);
    
    if (code == LV_EVENT_VALUE_CHANGED) {
        uint32_t id = lv_dropdown_get_selected(dropdown);
        ScrSettings *settings = (ScrSettings *)lv_event_get_user_data(e);
        
        // Hide current page
        if (current_page) {
            lv_obj_add_flag(current_page, LV_OBJ_FLAG_HIDDEN);
        }
        
        // Show selected page
        current_page = (lv_obj_t *)settings->pages[id];
        if (current_page) {
            lv_obj_remove_flag(current_page, LV_OBJ_FLAG_HIDDEN);
        }
    }
    else if (code == LV_EVENT_READY) {
        // Style the list when dropdown opens
        style_dropdown_list(dropdown);
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

static void ai_status_handler(void *data)
{
    bool value = (bool)data;
    AIStatusPacket pkt(value);
    sendRestPacket(&pkt);
    log_i("Pressed ai status %i", value);
}

static void maintain_pressure_handler(void *data)
{
    bool value = (bool)data;
    MaintainPressurePacket pkt(value);
    sendRestPacket(&pkt);
    log_i("Pressed maintain pressure %i", value);
}

static void rise_on_start_handler(void *data)
{
    bool value = (bool)data;
    RiseOnStartPacket pkt(value);
    sendRestPacket(&pkt);
    log_i("Pressed riseonstart %i", value);
}

static void fall_on_shutdown_handler(void *data)
{
    bool value = (bool)data;
    FallOnShutdownPacket pkt(value);
    sendRestPacket(&pkt);
    log_i("Pressed fallonshutdown %i", value);
}

static void safety_mode_handler(void *data)
{
    bool value = (bool)data;
    SafetyModePacket pkt(value);
    sendRestPacket(&pkt);
    log_i("Pressed safetymode %i", value);
}

void ScrSettings::updateUpdateButtonVisbility()
{
    if (getwifiSSID().length() > 0 && getwifiPassword().length() > 0)
    {
        lv_obj_remove_flag(((Option *)this->ui_updateBtn)->root, (lv_obj_flag_t)(LV_OBJ_FLAG_HIDDEN));
    }
    else
    {
        lv_obj_add_flag(((Option *)this->ui_updateBtn)->root, (lv_obj_flag_t)(LV_OBJ_FLAG_HIDDEN));
    }
}

void ScrSettings::init()
{
    Scr::init();

    // Create main container for settings (not scrollable)
    menu_container = lv_obj_create(this->scr);
    lv_obj_remove_style_all(menu_container);
    lv_obj_set_size(menu_container, LCD_WIDTH, LCD_HEIGHT - NAVBAR_HEIGHT);
    lv_obj_align(menu_container, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(menu_container, LV_OPA_TRANSP, 0);
    lv_obj_set_layout(menu_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(menu_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(menu_container, LV_OBJ_FLAG_SCROLLABLE); // Prevent scrolling on main container

    // Create top menu bar with dropdown (fixed, not scrollable)
    lv_obj_t *menu_bar = lv_obj_create(menu_container);
    lv_obj_remove_style_all(menu_bar);
    lv_obj_set_size(menu_bar, LCD_WIDTH, 50 * SCALE_Y);
    lv_obj_set_style_bg_color(menu_bar, lv_color_hex(THEME_COLOR_DARK), 0);
    lv_obj_set_style_bg_opa(menu_bar, LV_OPA_COVER, 0);
    lv_obj_set_layout(menu_bar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(menu_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(menu_bar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(menu_bar, 5, 0);
    lv_obj_clear_flag(menu_bar, LV_OBJ_FLAG_SCROLLABLE); // Prevent scrolling on menu bar

    // Create dropdown for section selection (square corners, no rounded corners)
    lv_obj_t *dropdown = lv_dropdown_create(menu_bar);
    lv_dropdown_set_options(dropdown, "Status\nGame Controller\nML/AI\nBasic settings\nLevelling Mode\nUnits\nGame Controller\nConfig\nWifi / Update");
    lv_obj_set_width(dropdown, LCD_WIDTH - 10);
    lv_obj_set_height(dropdown, 40 * SCALE_Y); // Reasonable height for easy pressing
    lv_obj_set_style_bg_color(dropdown, lv_color_hex(THEME_COLOR_LIGHT), 0);
    lv_obj_set_style_text_color(dropdown, lv_color_white(), 0);
    lv_obj_set_style_text_font(dropdown, &lv_font_montserrat_20, 0);
    lv_obj_set_style_radius(dropdown, 0, 0); // Remove rounded corners - perfect square corners
    lv_obj_set_style_pad_all(dropdown, 10, 0);
    // lv_obj_set_style_transform_angle(dropdown, 1800, LV_PART_INDICATOR | LV_STATE_CHECKED);
    
    // Add event handlers for value changed and when dropdown opens
    lv_obj_add_event_cb(dropdown, section_dropdown_event_cb, LV_EVENT_VALUE_CHANGED, this);
    lv_obj_add_event_cb(dropdown, section_dropdown_event_cb, LV_EVENT_READY, this);

    // Create pages container (scrollable area for content only)
    lv_obj_t *pages_container = lv_obj_create(menu_container);
    lv_obj_remove_style_all(pages_container);
    lv_obj_set_size(pages_container, LCD_WIDTH, LCD_HEIGHT - NAVBAR_HEIGHT - 50 * SCALE_Y); // Adjusted for menu bar only
    lv_obj_set_style_bg_opa(pages_container, LV_OPA_TRANSP, 0);
    lv_obj_set_layout(pages_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(pages_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollbar_mode(pages_container, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(pages_container, LV_DIR_VER); // Only vertical scrolling

    // Initialize pages array
    for (int i = 0; i < NUM_SECTIONS; i++) {
        this->pages[i] = NULL;
    }

    // Create Status page
    lv_obj_t *status_page = lv_obj_create(pages_container);
    lv_obj_remove_style_all(status_page);
    lv_obj_set_size(status_page, LCD_WIDTH, LV_SIZE_CONTENT);
    lv_obj_set_layout(status_page, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(status_page, LV_FLEX_FLOW_COLUMN);
    // lv_obj_set_style_pad_all(status_page, 5, 0);
    this->pages[0] = status_page;
    this->ui_s1 = new Option(status_page, OptionType::TEXT_WITH_VALUE, "Compressor Frozen:", {.STRING = test});
    this->ui_s3 = new Option(status_page, OptionType::TEXT_WITH_VALUE, "ACC Status:", {.STRING = test});
    this->ui_ebrakeStatus = new Option(status_page, OptionType::TEXT_WITH_VALUE, AIR_OUT_ON_SHUTOFF_DOUBLE_LOCK_MODE == true ? "Door Lock Status:" : "E-Brake Status:", {.STRING = test});
    this->ui_s2 = new Option(status_page, OptionType::ON_OFF, "Compressor Status:", {.STRING = test}, compressor_status_handler);
    this->ui_rebootbutton = new Option(status_page, OptionType::BUTTON, "Reboot/Turn Off", {.STRING = test}, [](void *data)
                                       { currentScr->showMsgBox("Reboot/Turn Off?", NULL, "Confirm", "Cancel", []() -> void
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
                                                                    } }, []() -> void {}, false); });

    // Create Game Controller page
    lv_obj_t *game_controller_page = lv_obj_create(pages_container);
    lv_obj_remove_style_all(game_controller_page);
    lv_obj_set_size(game_controller_page, LCD_WIDTH, LV_SIZE_CONTENT);
    lv_obj_set_layout(game_controller_page, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(game_controller_page, LV_FLEX_FLOW_COLUMN);
    // lv_obj_set_style_pad_all(game_controller_page, 5, 0);
    lv_obj_add_flag(game_controller_page, LV_OBJ_FLAG_HIDDEN);
    this->pages[1] = game_controller_page;
    new Option(game_controller_page, OptionType::BUTTON, "Allow New Controller", {.STRING = test}, [](void *data)
               { currentScr->showMsgBox("Confirm?", "After clicking this, OASMan will become pairable and the next controller to try to pair with OASMan will be allowed to pair and remembered by OASMan.\nMax saved devices is 20", "Confirm", "Cancel", []() -> void
                                        {
                                            BP32Packet pkt(BP32CMD::BP32CMD_ENABLE_NEW_CONN, true);
                                            sendRestPacket(&pkt);
                                            showDialog("Connect your controller!", lv_color_hex(0xFFFF00)); }, []() -> void {}, false); });
    new Option(game_controller_page, OptionType::BUTTON, "Un-pair All Controllers", {.STRING = test}, [](void *data)
               { currentScr->showMsgBox("Confirm?", "After clicking this, all paired game controllers will be removed from memory, and actively connected ones will be disconnected. This also resets your saved devices back to 0.", "Confirm", "Cancel", []() -> void
                                        {
                                            BP32Packet pkt(BP32CMD::BP32CMD_FORGET_DEVICES, NULL);
                                            sendRestPacket(&pkt);
                                            showDialog("Controllers forgotten!", lv_color_hex(0xFFFF00)); }, []() -> void {}, false); });
    new Option(game_controller_page, OptionType::BUTTON, "Disconnect Controllers", {.STRING = test}, [](void *data)
               { currentScr->showMsgBox("Confirm?", "Some devices may be difficult to disconnect on their own, this will disconnect them for you. Hint: Pressing the 'system' button on supporting controllers will disconnect them.", "Confirm", "Cancel", []() -> void
                                        {
                                            BP32Packet pkt(BP32CMD::BP32CMD_DISCONNECT_DEVICES, NULL);
                                            sendRestPacket(&pkt);
                                            showDialog("Controllers disconnected!", lv_color_hex(0xFFFF00)); }, []() -> void {}, false); });

    // Create ML/AI page
    lv_obj_t *ml_ai_page = lv_obj_create(pages_container);
    lv_obj_remove_style_all(ml_ai_page);
    lv_obj_set_size(ml_ai_page, LCD_WIDTH, LV_SIZE_CONTENT);
    lv_obj_set_layout(ml_ai_page, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(ml_ai_page, LV_FLEX_FLOW_COLUMN);
    // lv_obj_set_style_pad_all(ml_ai_page, 5, 0);
    lv_obj_add_flag(ml_ai_page, LV_OBJ_FLAG_HIDDEN);
    this->pages[2] = ml_ai_page;
    this->ui_aiPercentage = new Option(ml_ai_page, OptionType::TEXT_WITH_VALUE, "Learn Progress:", {.STRING = test});
    this->ui_aiReady = new Option(ml_ai_page, OptionType::TEXT_WITH_VALUE, "Trained:", {.STRING = test});
    this->ui_aiEnabled = new Option(ml_ai_page, OptionType::ON_OFF, "Enabled:", {.STRING = test}, ai_status_handler);
    new Option(ml_ai_page, OptionType::BUTTON, "Reset Learned Data", {.STRING = test}, [](void *data)
               { currentScr->showMsgBox("Reset Learned AI data?", "Run this if ai has completed training and you are getting innacurate presets.", "Confirm", "Cancel", []() -> void
                                        {
                                            ResetAIPacket pkt;
                                            sendRestPacket(&pkt);
                                            log_i("Pressed reset ai"); }, []() -> void {}, false); });

    // Create Basic settings page
    lv_obj_t *basic_settings_page = lv_obj_create(pages_container);
    lv_obj_remove_style_all(basic_settings_page);
    lv_obj_set_size(basic_settings_page, LCD_WIDTH, LV_SIZE_CONTENT);
    lv_obj_set_layout(basic_settings_page, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(basic_settings_page, LV_FLEX_FLOW_COLUMN);
    // lv_obj_set_style_pad_all(basic_settings_page, 5, 0);
    lv_obj_add_flag(basic_settings_page, LV_OBJ_FLAG_HIDDEN);
    this->pages[3] = basic_settings_page;
    this->ui_maintainprssure = new Option(basic_settings_page, OptionType::ON_OFF, "Maintain Preset", {.STRING = test}, maintain_pressure_handler);
    this->ui_riseonstart = new Option(basic_settings_page, OptionType::ON_OFF, "Rise on start", {.STRING = test}, rise_on_start_handler);
#if ENABLE_AIR_OUT_ON_SHUTOFF
    this->ui_airoutonshutoff = new Option(basic_settings_page, OptionType::ON_OFF, "Fall on shutdown", {.STRING = test}, fall_on_shutdown_handler);
#endif
    this->ui_safetymode = new Option(basic_settings_page, OptionType::ON_OFF, "Safety Mode", {.STRING = test}, safety_mode_handler);
    new Option(basic_settings_page, OptionType::BUTTON, "Detect Pressure Sensors", {.STRING = test}, [](void *data)
               { currentScr->showMsgBox("Detect Pressure Sensors?", "WARNING: YOUR CAR WILL BE AIRED OUT!!!! This routine will auto learn which pressure sensors go to which wheels.", "Confirm", "Cancel", []() -> void
                                        {
                                            DetectPressureSensorsPacket pkt;
                                            sendRestPacket(&pkt);
                                            log_i("Pressed detected pressure sensors");
                                            showDialog("Doing detection routine", lv_color_hex(0xFFFF00)); }, []() -> void {}, false); });

    // Create Levelling Mode page
    lv_obj_t *levelling_page = lv_obj_create(pages_container);
    lv_obj_remove_style_all(levelling_page);
    lv_obj_set_size(levelling_page, LCD_WIDTH, LV_SIZE_CONTENT);
    lv_obj_set_layout(levelling_page, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(levelling_page, LV_FLEX_FLOW_COLUMN);
    // lv_obj_set_style_pad_all(levelling_page, 5, 0);
    lv_obj_add_flag(levelling_page, LV_OBJ_FLAG_HIDDEN);
    this->pages[4] = levelling_page;
    const char *levelTypeRadioText[2] = {"Pressure Sensor", "Level Sensor"};
    option_event_cb_t levelTypeRadioCB = [](void *data)
    {
        HeightSensorModePacket pkt(((bool)data));
        sendRestPacket(&pkt);
    };
    this->ui_heightsensormode = new RadioOption(levelling_page, levelTypeRadioText, 2, levelTypeRadioCB);

    // Create Units page
    lv_obj_t *units_page = lv_obj_create(pages_container);
    lv_obj_remove_style_all(units_page);
    lv_obj_set_size(units_page, LCD_WIDTH, LV_SIZE_CONTENT);
    lv_obj_set_layout(units_page, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(units_page, LV_FLEX_FLOW_COLUMN);
    // lv_obj_set_style_pad_all(units_page, 5, 0);
    lv_obj_add_flag(units_page, LV_OBJ_FLAG_HIDDEN);
    this->pages[5] = units_page;
    const char *unitsRadioText[2] = {"PSI", "Bar"};
    option_event_cb_t unitsRadioCB = [](void *data)
    { setunitsMode((int)data); };
    new RadioOption(units_page, unitsRadioText, 2, unitsRadioCB, getunitsMode());

    // Create Controller Settings page
    lv_obj_t *controller_settings_page = lv_obj_create(pages_container);
    lv_obj_remove_style_all(controller_settings_page);
    lv_obj_set_size(controller_settings_page, LCD_WIDTH, LV_SIZE_CONTENT);
    lv_obj_set_layout(controller_settings_page, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(controller_settings_page, LV_FLEX_FLOW_COLUMN);
    // lv_obj_set_style_pad_all(controller_settings_page, 5, 0);
    lv_obj_add_flag(controller_settings_page, LV_OBJ_FLAG_HIDDEN);
    this->pages[6] = controller_settings_page;
    new Option(controller_settings_page, OptionType::KEYBOARD_INPUT_NUMBER, "Dim Screen (Minutes)", {.INT = getscreenDimTimeM()}, [](void *data)
               { log_i("Pressed %i", ((uint32_t)data));
        setscreenDimTimeM((uint32_t)data); });
    this->ui_brightnessSlider = new Option(controller_settings_page, OptionType::SLIDER, "Brightness", {.INT = getbrightness()}, [](void *data)
                                           { log_i("Brightness %i", ((uint32_t)data));
        setbrightness((uint32_t)data);
        set_brightness(getBrightnessFloat()); });
    ((Option *)this->ui_brightnessSlider)->setSliderParams(1, 100, false, LV_EVENT_VALUE_CHANGED);

    // Create Config page
    lv_obj_t *config_page = lv_obj_create(pages_container);
    lv_obj_remove_style_all(config_page);
    lv_obj_set_size(config_page, LCD_WIDTH, LV_SIZE_CONTENT);
    lv_obj_set_layout(config_page, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(config_page, LV_FLEX_FLOW_COLUMN);
    // lv_obj_set_style_pad_all(config_page, 5, 0);
    lv_obj_add_flag(config_page, LV_OBJ_FLAG_HIDDEN);
    this->pages[7] = config_page;
    this->ui_config1 = new Option(config_page, OptionType::SLIDER, "Bag Max PSI", {.INT = 200}, [](void *data)
                                   { log_i("Pressed %i", ((uint32_t)data));
        *util_configValues._bagMaxPressure() = (uint32_t)data;
        sendConfigValuesPacket(true);
        alertValueUpdated(); });
    ((Option *)this->ui_config1)->setSliderParams(1, 256, true, LV_EVENT_RELEASED);
    new Option(config_page, OptionType::KEYBOARD_INPUT_NUMBER, "Bluetooth Passkey (6 digits)", {.INT = getblePasskey()}, [](void *data)
               { log_i("Pressed %i", (data));
        setblePasskey((uint32_t)data);
        if (isConnectedToManifold()) {
            AuthPacket pkt(getblePasskey(), AuthResult::AUTHRESULT_UPDATEKEY);
            sendRestPacket(&pkt); 
        } else {
            authblacklist.clear();
        }
        alertValueUpdated(); });
    this->ui_config2 = new Option(config_page, OptionType::KEYBOARD_INPUT_NUMBER, "Shutoff Time (Minutes)", {.INT = 0}, [](void *data)
                                   { log_i("Pressed %i", ((uint32_t)data)); 
        *util_configValues._systemShutoffTimeM() = (uint32_t)data;
        sendConfigValuesPacket(true);
        alertValueUpdated(); });
    this->ui_config3 = new Option(config_page, OptionType::KEYBOARD_INPUT_NUMBER, "Compressor On PSI", {.INT = 0}, [](void *data)
                                  { log_i("Pressed %i", ((uint32_t)data)); 
        *util_configValues._compressorOnPSI() = (uint32_t)data;
        sendConfigValuesPacket(true);
        alertValueUpdated(); });
    this->ui_config4 = new Option(config_page, OptionType::KEYBOARD_INPUT_NUMBER, "Compressor Off PSI", {.INT = 0}, [](void *data)
                                  { log_i("Pressed %i", ((uint32_t)data)); 
        *util_configValues._compressorOffPSI() = (uint32_t)data;
        sendConfigValuesPacket(true);
        alertValueUpdated(); });
    this->ui_config5 = new Option(config_page, OptionType::KEYBOARD_INPUT_NUMBER, "Pressure Sensor Rating PSI", {.INT = 0}, [](void *data)
                                  { log_i("Pressed %i", ((uint32_t)data)); 
        *util_configValues._pressureSensorMax() = (uint32_t)data;
        sendConfigValuesPacket(true);
        alertValueUpdated(); });
    this->ui_config6 = new Option(config_page, OptionType::SLIDER, "Bag Volume Percentage", {.INT = 100}, [](void *data)
                                  { log_i("Pressed %i", ((uint32_t)data)); 
        *util_configValues._bagVolumePercentage() = (uint32_t)data;
        sendConfigValuesPacket(true);
        alertValueUpdated(); });
    ((Option *)this->ui_config6)->setSliderParams(10, 600, true, LV_EVENT_RELEASED);

    // Create Wifi / Update page
    lv_obj_t *wifi_update_page = lv_obj_create(pages_container);
    lv_obj_remove_style_all(wifi_update_page);
    lv_obj_set_size(wifi_update_page, LCD_WIDTH, LV_SIZE_CONTENT);
    lv_obj_set_layout(wifi_update_page, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(wifi_update_page, LV_FLEX_FLOW_COLUMN);
    // lv_obj_set_style_pad_all(wifi_update_page, 5, 0);
    lv_obj_add_flag(wifi_update_page, LV_OBJ_FLAG_HIDDEN);
    this->pages[8] = wifi_update_page;
    char buf[50];
    strncpy(buf, getwifiSSID().c_str(), sizeof(buf));
    OptionValue wifiOptionValue;
    wifiOptionValue.STRING = buf;
    new Option(wifi_update_page, OptionType::KEYBOARD_INPUT_TEXT, "SSID", wifiOptionValue, [](void *data)
               {
                   log_i("Typed %s", ((char *)data));
                   setwifiSSID(((char *)data));
                   alertValueUpdated();
                   ((ScrSettings *)currentScr)->updateUpdateButtonVisbility();
               });
    strncpy(buf, getwifiPassword().c_str(), sizeof(buf));
    wifiOptionValue.STRING = buf;
    Option *pass = new Option(wifi_update_page, OptionType::KEYBOARD_INPUT_TEXT, "PASS", wifiOptionValue, [](void *data)
                              {
                                  log_i("Typed %s", ((char *)data));
                                  setwifiPassword(((char *)data));
                                  alertValueUpdated();
                                  ((ScrSettings *)currentScr)->updateUpdateButtonVisbility();
                              });
    lv_textarea_set_password_mode(pass->rightHandObj, true);
    lv_textarea_set_password_show_time(pass->rightHandObj, 10000);
    this->ui_updateBtn = (Option *)new Option(wifi_update_page, OptionType::BUTTON, "Start Software Update", {.STRING = test}, [](void *data)
                                    { currentScr->showMsgBox("Begin update wifi service?", "This will use the wifi credentials you entered above to download and install the latest firmware update on both the manifold and controller. If an issue occurs, please go to http://oasman.dev and flash manually. Continue?", "Start", "Cancel", []() -> void
                                                             {
                                                                 StartwebPacket pkt(getwifiSSID(), getwifiPassword());
                                                                 sendRestPacket(&pkt);
                                                                 log_i("Starting web service");
#if defined(OTA_SUPPORTED)
                                                                 runNextFrame([]() -> void
                                                                              {
                                                                                  currentScr->showMsgBox("Updating in progress...", "Both the manifold & controller are installing their updates. Both will reboot when completed.", NULL, "OK", []() -> void {}, []() -> void {}, false);
                                                                                  runNextFrame([]() -> void
                                                                                               { delay(250); setupdateMode(true);
                                                                    runNextFrame([]() -> void
                                                                                { ESP.restart(); }); });
                                                                                  Serial.println("Attempted to download update"); });
#else
                                                                 currentScr->showMsgBox("Updating in progress...", "The manifold is installing the latest update. Your controller does not support OTA updates. Please go to http://oasman.dev on your computer to flash the latest update to your controller.", NULL, "OK", []() -> void
                                                                                        { ESP.restart(); }, []() -> void
                                                                                        { ESP.restart(); }, true);
#endif
                                                             },
                                                             []() -> void {}, false); });
    updateUpdateButtonVisbility();
    this->ui_manifoldUpdateStatus = new Option(wifi_update_page, OptionType::TEXT_WITH_VALUE, "Manifold:", {.STRING = test});

    // QR Code
    lv_obj_t *qrCodeParent = lv_obj_create(wifi_update_page);
    lv_obj_remove_style_all(qrCodeParent);
    lv_obj_set_size(qrCodeParent, LCD_WIDTH, 100);
    this->ui_qrcode = lv_qrcode_create(qrCodeParent);
    lv_qrcode_set_size(this->ui_qrcode, 100);
    lv_qrcode_set_dark_color(this->ui_qrcode, lv_color_black());
    lv_qrcode_set_light_color(this->ui_qrcode, lv_color_white());
    const char *qr_data = "https://oasman.dev";
    lv_qrcode_update(this->ui_qrcode, qr_data, strlen(qr_data));
    lv_obj_set_x(this->ui_qrcode, LCD_WIDTH / 2 - 50);

    // Version and info
    OptionValue versionValue;
#ifdef OFFICIAL_RELEASE
    versionValue.STRING = EVALUATE_AND_STRINGIFY(RELEASE_VERSION);
#else
    versionValue.STRING = "DEVELOPMENT";
#endif
    new Option(wifi_update_page, OptionType::TEXT_WITH_VALUE, "Version:", versionValue);
    this->ui_mac = new Option(wifi_update_page, OptionType::TEXT_WITH_VALUE, "Manifold:", {.STRING = ble_getMAC()});
    this->ui_volts = new Option(wifi_update_page, OptionType::TEXT_WITH_VALUE, "Battery:", {.STRING = getBatteryVoltageString()});

    // Set initial page
    current_page = status_page;
    lv_dropdown_set_selected(dropdown, 0);

    sendConfigValuesPacket(false);
}

void ScrSettings::runTouchInput(SimplePoint pos, bool down)
{
    Scr::runTouchInput(pos, down);
}

void ScrSettings::loop()
{
    Scr::loop();

    // Update status text labels
    this->ui_s1->setRightHandText(statusBittset & (1 << StatusPacketBittset::COMPRESSOR_FROZEN) ? "Yes" : "No");
    this->ui_s3->setRightHandText(statusBittset & (1 << StatusPacketBittset::ACC_STATUS_ON) ? "On" : "Off");
    this->ui_ebrakeStatus->setRightHandText(statusBittset & (1 << StatusPacketBittset::EBRAKE_STATUS_ON) ? "On" : "Off");

    // Update reboot button text
    this->ui_rebootbutton->setRightHandText(statusBittset & (1 << StatusPacketBittset::ACC_STATUS_ON) ? "Reboot" : "Shut Down");

    // Update switches
    this->ui_s2->setBooleanValue(statusBittset & (1 << StatusPacketBittset::COMPRESSOR_STATUS_ON), false);
    this->ui_riseonstart->setBooleanValue(statusBittset & (1 << StatusPacketBittset::RISE_ON_START), false);
    this->ui_maintainprssure->setBooleanValue(statusBittset & (1 << StatusPacketBittset::MAINTAIN_PRESSURE), false);
#if ENABLE_AIR_OUT_ON_SHUTOFF
    this->ui_airoutonshutoff->setBooleanValue(statusBittset & (1 << StatusPacketBittset::AIR_OUT_ON_SHUTOFF), false);
#endif
    this->ui_safetymode->setBooleanValue(statusBittset & (1 << StatusPacketBittset::SAFETY_MODE), false);
    this->ui_aiEnabled->setBooleanValue(statusBittset & (1 << StatusPacketBittset::AI_STATUS_ENABLED), false);

    this->ui_heightsensormode->setSelectedOption((statusBittset & (1 << StatusPacketBittset::HEIGHT_SENSOR_MODE)) != 0 ? 1 : 0);

    // Update AI status
    if (util_statusRequestPacket._setStatus)
    {
        util_statusRequestPacket._setStatus = false;
        this->ui_manifoldUpdateStatus->setRightHandText(util_statusRequestPacket.getStatus().c_str());
    }

    char buf[40];
    snprintf(buf, sizeof(buf), "%i%%", AIPercentage);
    this->ui_aiPercentage->setRightHandText(buf);
    snprintf(buf, sizeof(buf), "UF:  %c UR:  %c\nDF: %c DR: %c", (AIReadyBittset & 0b1) ? 'Y' : 'n', (AIReadyBittset & 0b10 >> 1) ? 'Y' : 'n', (AIReadyBittset & 0b100 >> 2) ? 'Y' : 'n', (AIReadyBittset & 0b1000 >> 3) ? 'Y' : 'n');
    this->ui_aiReady->setRightHandText(buf);

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
    }
}
