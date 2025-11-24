#include "ui_scrSettings.h"

LV_IMG_DECLARE(navbar_settings);
ScrSettings scrSettings(navbar_settings, false);

void alertValueUpdated()
{
    showDialog("Set value", lv_color_hex(0xFFFF00));
}

const char *test = "";
extern bool isConnectedToManifold(); // from ble.cpp
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
void ScrSettings::init()
{
    Scr::init();

    this->optionsContainer = lv_obj_create(this->scr);
    lv_obj_remove_style_all(this->optionsContainer);
    lv_obj_set_size(this->optionsContainer, DISPLAY_WIDTH, DISPLAY_HEIGHT - NAVBAR_HEIGHT);
    lv_obj_align(this->optionsContainer, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_layout(this->optionsContainer, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(this->optionsContainer, LV_FLEX_FLOW_COLUMN);

    OptionValue defaultCharVal;
    defaultCharVal.STRING = test; //{.STRING = test}
    new Option(this->optionsContainer, OptionType::HEADER, "Status");
    this->ui_s1 = new Option(this->optionsContainer, OptionType::TEXT_WITH_VALUE, "Compressor Frozen:", defaultCharVal);
    this->ui_s3 = new Option(this->optionsContainer, OptionType::TEXT_WITH_VALUE, "ACC Status:", defaultCharVal);
    this->ui_ebrakeStatus = new Option(this->optionsContainer, OptionType::TEXT_WITH_VALUE, "E-Brake Status:", defaultCharVal);
    // this->ui_s4 = new Option(this->optionsContainer, OptionType::TEXT_WITH_VALUE, "Timer Expired:", defaultCharVal);
    // this->ui_s5 = new Option(this->optionsContainer, OptionType::TEXT_WITH_VALUE, "Clock:", defaultCharVal);
    this->ui_s2 = new Option(this->optionsContainer, OptionType::ON_OFF, "Compressor Status:", defaultCharVal, [](void *data)
                             { 
                CompressorStatusPacket pkt(((bool)data));
                sendRestPacket(&pkt);
                log_i("Pressed compressor status %i", ((bool)data)); });

    this->ui_rebootbutton = new Option(this->optionsContainer, OptionType::BUTTON, "Reboot/Turn Off", defaultCharVal, [](void *data)
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

    new Option(this->optionsContainer, OptionType::SPACE, "");
    new Option(this->optionsContainer, OptionType::HEADER, "Game Controller");

    new Option(this->optionsContainer, OptionType::BUTTON, "Allow New Controller", defaultCharVal, [](void *data)
               { currentScr->showMsgBox("Confirm?", "After clicking this, OASMan will become pairable and the next controller to try to pair with OASMan will be allowed to pair and remembered by OASMan.\nMax saved devices is 20", "Confirm", "Cancel", []() -> void
                                        {
                                            BP32Packet pkt(BP32CMD::BP32CMD_ENABLE_NEW_CONN, true);
                                            sendRestPacket(&pkt);
                                            showDialog("Connect your controller!", lv_color_hex(0xFFFF00)); }, []() -> void {}, false); });

    new Option(this->optionsContainer, OptionType::BUTTON, "Un-pair All Controllers", defaultCharVal, [](void *data)
               { currentScr->showMsgBox("Confirm?", "After clicking this, all paired game controllers will be removed from memory, and actively connected ones will be disconnected. This also resets your saved devices back to 0.", "Confirm", "Cancel", []() -> void
                                        {
                                            BP32Packet pkt(BP32CMD::BP32CMD_FORGET_DEVICES, NULL);
                                            sendRestPacket(&pkt);
                                            showDialog("Controllers forgotten!", lv_color_hex(0xFFFF00)); }, []() -> void {}, false); });

    new Option(this->optionsContainer, OptionType::BUTTON, "Disconnect Controllers", defaultCharVal, [](void *data)
               { currentScr->showMsgBox("Confirm?", "Some devices may be difficult to disconnect on their own, this will disconnect them for you. Hint: Pressing the 'system' button on supporting controllers will disconnect them.", "Confirm", "Cancel", []() -> void
                                        {
                                            BP32Packet pkt(BP32CMD::BP32CMD_DISCONNECT_DEVICES, NULL);
                                            sendRestPacket(&pkt);
                                            showDialog("Controllers disconnected!", lv_color_hex(0xFFFF00)); }, []() -> void {}, false); });

    new Option(this->optionsContainer, OptionType::SPACE, "");
    new Option(this->optionsContainer, OptionType::HEADER, "ML/AI");
    this->ui_aiPercentage = new Option(this->optionsContainer, OptionType::TEXT_WITH_VALUE, "Learn Progress:", defaultCharVal);
    this->ui_aiReady = new Option(this->optionsContainer, OptionType::TEXT_WITH_VALUE, "Trained:", defaultCharVal);
    this->ui_aiEnabled = new Option(this->optionsContainer, OptionType::ON_OFF, "Enabled:", defaultCharVal, [](void *data)
                                    { 
                AIStatusPacket pkt(((bool)data));
                sendRestPacket(&pkt);
                log_i("Pressed ai status %i", ((bool)data)); });
    new Option(this->optionsContainer, OptionType::BUTTON, "Reset Learned Data", defaultCharVal, [](void *data)
               { currentScr->showMsgBox("Reset Learned AI data?", "Run this if ai has completed training and you are getting innacurate presets.", "Confirm", "Cancel", []() -> void
                                        {
                                                                
                                                                    ResetAIPacket pkt;
                                                                    sendRestPacket(&pkt);
                                                                    log_i("Pressed reset ai"); }, []() -> void {}, false); });

    new Option(this->optionsContainer, OptionType::SPACE, "");
    new Option(this->optionsContainer, OptionType::HEADER, "Basic settings");
    this->ui_maintainprssure = new Option(this->optionsContainer, OptionType::ON_OFF, "Maintain Preset", defaultCharVal, [](void *data)
                                          { 
                MaintainPressurePacket pkt(((bool)data));
                sendRestPacket(&pkt);
                log_i("Pressed maintain pressure %i", ((bool)data)); });
    this->ui_riseonstart = new Option(this->optionsContainer, OptionType::ON_OFF, "Rise on start", defaultCharVal, [](void *data)
                                      { 
                RiseOnStartPacket pkt(((bool)data));
                sendRestPacket(&pkt);
                log_i("Pressed riseonstart %i", ((bool)data)); });
#if ENABLE_AIR_OUT_ON_SHUTOFF
    this->ui_airoutonshutoff = new Option(this->optionsContainer, OptionType::ON_OFF, "Fall on shutdown", defaultCharVal, [](void *data)
                                          {
                FallOnShutdownPacket pkt(((bool)data));
                sendRestPacket(&pkt);
                log_i("Pressed fallonshutdown %i", ((bool)data)); });
#endif

    this->ui_safetymode = new Option(this->optionsContainer, OptionType::ON_OFF, "Safety Mode", defaultCharVal, [](void *data)
                                     { 
                SafetyModePacket pkt(((bool)data));
                sendRestPacket(&pkt);
                log_i("Pressed safetymode %i", ((bool)data)); });

    new Option(this->optionsContainer, OptionType::BUTTON, "Detect Pressure Sensors", defaultCharVal, [](void *data)
               { currentScr->showMsgBox("Detect Pressure Sensors?", "WARNING: YOUR CAR WILL BE AIRED OUT!!!! This routine will auto learn which pressure sensors go to which wheels.", "Confirm", "Cancel", []() -> void
                                        {
                                                                    DetectPressureSensorsPacket pkt;
                    sendRestPacket(&pkt);
                    log_i("Pressed detected pressure sensors");
                    showDialog("Doing detection routine", lv_color_hex(0xFFFF00)); }, []() -> void {}, false); });

    // new Option(this->optionsContainer, OptionType::BUTTON, "Learn System Calibration", defaultCharVal, [](void *data)
    //            { currentScr->showMsgBox("Learn System Calibration?", "WARNING: YOUR CAR WILL BE AIRED OUT!!!! This routine will take some values to give you smoother presets. Your compressor will fill to full and then take some values.", "Confirm", "Cancel", []() -> void
    //                                     {
    //                 CalibratePacket pkt;
    //                sendRestPacket(&pkt);
    //                log_i("Pressed learn system calibration");
    //                showDialog("Doing calibration routine", lv_color_hex(0xFFFF00)); }, []() -> void {}, false); });

    new Option(this->optionsContainer, OptionType::SPACE, "");
    new Option(this->optionsContainer, OptionType::HEADER, "Levelling Mode");

    const char *levelTypeRadioText[2] = {"Pressure Sensor", "Level Sensor"};
    option_event_cb_t levelTypeRadioCB = [](void *data)
    {
        HeightSensorModePacket pkt(((bool)data));
        sendRestPacket(&pkt);
    };
    this->ui_heightsensormode = new RadioOption(this->optionsContainer, levelTypeRadioText, 2, levelTypeRadioCB);

    new Option(this->optionsContainer, OptionType::SPACE, "");
    new Option(this->optionsContainer, OptionType::HEADER, "Units");

    const char *unitsRadioText[2] = {"PSI", "Bar"}; // aligns with the enum UNITS_MODE
    option_event_cb_t unitsRadioCB = [](void *data)
    { setunitsMode((int)data); };
    new RadioOption(this->optionsContainer, unitsRadioText, 2, unitsRadioCB, getunitsMode());

    new Option(this->optionsContainer, OptionType::SPACE, "");
    new Option(this->optionsContainer, OptionType::HEADER, "Controller Settings");

    new Option(this->optionsContainer, OptionType::KEYBOARD_INPUT_NUMBER, "Dim Screen (Minutes)", {.INT = getscreenDimTimeM()}, [](void *data)
               { log_i("Pressed %i", ((uint32_t)data));
        setscreenDimTimeM((uint32_t)data); });

    ui_brightnessSlider = new Option(this->optionsContainer, OptionType::SLIDER, "Brightness", {.INT = getbrightness()}, [](void *data)
                                     { log_i("Brightness %i", ((uint32_t)data));
        setbrightness((uint32_t)data);
        smartdisplay_lcd_set_backlight(getBrightnessFloat()); });
    ui_brightnessSlider->setSliderParams(1, 100, false, LV_EVENT_VALUE_CHANGED);

    new Option(this->optionsContainer, OptionType::SPACE, "", defaultCharVal);
    new Option(this->optionsContainer, OptionType::HEADER, "Config");

    this->ui_config1 = new Option(this->optionsContainer, OptionType::SLIDER, "Bag Max PSI" /*"MAX_PRESSURE_SAFETY"*/, {.INT = 200}, [](void *data)
                                  { log_i("Pressed %i", ((uint32_t)data));
        *util_configValues._bagMaxPressure() = (uint32_t)data;
        sendConfigValuesPacket(true);
    alertValueUpdated(); });
    this->ui_config1->setSliderParams(1, 256, true, LV_EVENT_RELEASED);

    new Option(this->optionsContainer, OptionType::KEYBOARD_INPUT_NUMBER, "Bluetooth Passkey (6 digits)" /*"BLE_PASSKEY"*/, {.INT = getblePasskey()}, [](void *data)
               { log_i("Pressed %i", (data));
        setblePasskey((uint32_t)data); // save locally
        if (isConnectedToManifold()) {
            AuthPacket pkt(getblePasskey(), AuthResult::AUTHRESULT_UPDATEKEY); // save on manifold
            sendRestPacket(&pkt); 
        } else {
            authblacklist.clear();
        }
        alertValueUpdated(); });

    this->ui_config2 = new Option(this->optionsContainer, OptionType::KEYBOARD_INPUT_NUMBER, "Shutoff Time (Minutes)" /*"SYSTEM_SHUTOFF_TIME_MS"*/, {.INT = 0}, [](void *data)
                                  { log_i("Pressed %i", ((uint32_t)data)); 
        *util_configValues._systemShutoffTimeM() = (uint32_t)data;
        sendConfigValuesPacket(true);
    alertValueUpdated(); });

    this->ui_config3 = new Option(this->optionsContainer, OptionType::KEYBOARD_INPUT_NUMBER, "Compressor On PSI" /*"COMPRESSOR_ON_BELOW_PSI"*/, {.INT = 0}, [](void *data)
                                  { log_i("Pressed %i", ((uint32_t)data)); 
        *util_configValues._compressorOnPSI() = (uint32_t)data;
        sendConfigValuesPacket(true);
    alertValueUpdated(); });

    this->ui_config4 = new Option(this->optionsContainer, OptionType::KEYBOARD_INPUT_NUMBER, "Compressor Off PSI" /*"COMPRESSOR_MAX_PSI"*/, {.INT = 0}, [](void *data)
                                  { log_i("Pressed %i", ((uint32_t)data)); 
        *util_configValues._compressorOffPSI() = (uint32_t)data;
        sendConfigValuesPacket(true);
    alertValueUpdated(); });

    this->ui_config5 = new Option(this->optionsContainer, OptionType::KEYBOARD_INPUT_NUMBER, "Pressure Sensor Rating PSI" /*"pressuretransducermaxPSI"*/, {.INT = 0}, [](void *data)
                                  { log_i("Pressed %i", ((uint32_t)data)); 
        *util_configValues._pressureSensorMax() = (uint32_t)data;
        sendConfigValuesPacket(true);
    alertValueUpdated(); });

    this->ui_config6 = new Option(this->optionsContainer, OptionType::SLIDER, "Bag Volume Percentage", {.INT = 100}, [](void *data)
                                  { log_i("Pressed %i", ((uint32_t)data)); 
        *util_configValues._bagVolumePercentage() = (uint32_t)data;
        sendConfigValuesPacket(true);
    alertValueUpdated(); });
    this->ui_config6->setSliderParams(10, 600, true, LV_EVENT_RELEASED);

    new Option(this->optionsContainer, OptionType::SPACE, "", defaultCharVal);
    new Option(this->optionsContainer, OptionType::HEADER, "Wifi / Update");

    char buf[50];
    strncpy(buf, getwifiSSID().c_str(), sizeof(buf));
    OptionValue wifiOptionValue;
    wifiOptionValue.STRING = buf;

    new Option(this->optionsContainer,
               OptionType::KEYBOARD_INPUT_TEXT,
               "SSID",
               wifiOptionValue,
               [](void *data)
               {
                   log_i("Typed %s", ((char *)data));
                   setwifiSSID(((char *)data));
                   alertValueUpdated();
                   ((ScrSettings *)currentScr)->updateUpdateButtonVisbility(); // lazy reference
               });

    strncpy(buf, getwifiPassword().c_str(), sizeof(buf));
    wifiOptionValue.STRING = buf; // actually shouldn't be necessary

    Option *pass = new Option(this->optionsContainer,
                              OptionType::KEYBOARD_INPUT_TEXT,
                              "PASS",
                              wifiOptionValue,
                              [](void *data)
                              {
                                  log_i("Typed %s", ((char *)data));
                                  setwifiPassword(((char *)data));
                                  alertValueUpdated();
                                  ((ScrSettings *)currentScr)->updateUpdateButtonVisbility(); // lazy reference
                              });

    lv_textarea_set_password_mode(pass->rightHandObj, true);
    lv_textarea_set_password_show_time(pass->rightHandObj, 10000);

    this->ui_updateBtn = new Option(this->optionsContainer, OptionType::BUTTON, "Start Software Update", defaultCharVal, [](void *data)
                                    { currentScr->showMsgBox("Begin update wifi service?", "This will use the wifi credentials you entered above to download and install the latest firmware update on both the manifold and controller. If an issue occurs, please go to http://oasman.dev and flash manually. Continue?", "Start", "Cancel", []() -> void // "This will start a wifi network on your esp32 that you can connect to from your phone that has an update page.\nAir suspension features will not be available during update mode.\nRestart the car/manifold to abort update mode."
                                                             {
                                                                 StartwebPacket pkt(getwifiSSID(), getwifiPassword());
                                                                 sendRestPacket(&pkt);
                                                                 log_i("Starting web service");
#if defined(OTA_SUPPORTED)
                                                                 runNextFrame([]() -> void
                                                                              {
                                                                                  currentScr->showMsgBox("Updating in progress...", "Both the manifold & controller are installing their updates. Both will reboot when completed.", NULL, "OK", []() -> void {}, []() -> void {}, false); // Open your phone and go to http://oasman.dev and download the latest manifold firmware.bin, then connect to the OASMAN-XXXXX wifi network. Then open your web browser and go to the website\nhttp://oasman.local to upload the firmware.bin

                                                                                  runNextFrame([]() -> void
                                                                                               { delay(250);setupdateMode(true);
                                                                    runNextFrame([]() -> void
                                                                                { ESP.restart(); }); });

                                                                                  Serial.println("Attempted to download update"); });

#else
                                                                 currentScr->showMsgBox("Updating in progress...", "The manifold is installing the latest update. Your controller does not support OTA updates. Please go to http://oasman.dev on your computer to flash the latest update to your controller.", NULL, "OK", []() -> void
                                                                                        { ESP.restart(); }, []() -> void
                                                                                        { ESP.restart(); }, true); // Open your phone and go to http://oasman.dev and download the latest manifold firmware.bin, then connect to the OASMAN-XXXXX wifi network. Then open your web browser and go to the website\nhttp://oasman.local to upload the firmware.bin

#endif
                                                             },
                                                             []() -> void {}, false); });

    updateUpdateButtonVisbility();

    this->ui_manifoldUpdateStatus = new Option(this->optionsContainer, OptionType::TEXT_WITH_VALUE, "Manifold:", defaultCharVal);

    // add space before qr code
    new Option(this->optionsContainer, OptionType::SPACE, "", defaultCharVal);

    lv_obj_t *qrCodeParent = lv_obj_create(this->optionsContainer);
    lv_obj_remove_style_all(qrCodeParent);
    // lv_obj_set_style_bg_opa(qrCodeParent, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_size(qrCodeParent, DISPLAY_WIDTH, 100);
    // lv_obj_set_style_bg_color(qrCodeParent, lv_color_hex(THEME_COLOR_LIGHT), LV_PART_MAIN | LV_STATE_DEFAULT);
    // lv_obj_set_x(qrCodeParent, DISPLAY_WIDTH - 100 / 2);
    // lv_obj_set_align(qrCodeParent, LV_ALIGN_TOP_MID);

    // To use third party libraries, enable the define in lv_conf.h: #define LV_USE_QRCODE 1
    this->ui_qrcode = lv_qrcode_create(qrCodeParent);
    lv_qrcode_set_size(this->ui_qrcode, 100);
    lv_qrcode_set_dark_color(this->ui_qrcode, lv_color_black());
    lv_qrcode_set_light_color(this->ui_qrcode, lv_color_white());
    const char *qr_data = "https://oasman.dev";
    lv_qrcode_update(this->ui_qrcode, qr_data, strlen(qr_data));
    // lv_obj_set_align(this->ui_qrcode, LV_ALIGN_TOP_MID);
    lv_obj_set_x(this->ui_qrcode, DISPLAY_WIDTH / 2 - 50);
    //  lv_obj_center(this->ui_qrcode);
    //  lv_obj_set_y(this->ui_qrcode, DISPLAY_HEIGHT - 10);

    new Option(this->optionsContainer, OptionType::SPACE, "", defaultCharVal);

    OptionValue versionValue;
#ifdef OFFICIAL_RELEASE
    versionValue.STRING = EVALUATE_AND_STRINGIFY(RELEASE_VERSION);
#else
    versionValue.STRING = "DEVELOPMENT";
#endif
    new Option(this->optionsContainer, OptionType::TEXT_WITH_VALUE, "Version:", versionValue);

    OptionValue macValue;
    macValue.STRING = ble_getMAC();
    this->ui_mac = new Option(this->optionsContainer, OptionType::TEXT_WITH_VALUE, "Manifold:", macValue);

#if defined(WAVESHARE_BOARD)
    OptionValue voltsValue;
    voltsValue.STRING = getBatteryVoltageString();
    this->ui_volts = new Option(this->optionsContainer, OptionType::TEXT_WITH_VALUE, "Battery:", voltsValue);
#endif

    // add space at end of list
    new Option(this->optionsContainer, OptionType::SPACE, "", defaultCharVal);

    sendConfigValuesPacket(false);
}

void ScrSettings::runTouchInput(SimplePoint pos, bool down)
{
    Scr::runTouchInput(pos, down);
}

void ScrSettings::loop()
{
    Scr::loop();
    this->ui_s1->setRightHandText(statusBittset & (1 << StatusPacketBittset::COMPRESSOR_FROZEN) ? "Yes" : "No");
    // this->ui_s2->setRightHandText(statusBittset & (1 << StatusPacketBittset::COMPRESSOR_STATUS_ON) ? "On" : "Off");
    this->ui_s3->setRightHandText(statusBittset & (1 << StatusPacketBittset::ACC_STATUS_ON) ? "On" : "Off");
    this->ui_ebrakeStatus->setRightHandText(statusBittset & (1 << StatusPacketBittset::EBRAKE_STATUS_ON) ? "On" : "Off");

    if (statusBittset & (1 << StatusPacketBittset::ACC_STATUS_ON))
    {
        this->ui_rebootbutton->setRightHandText("Reboot");
    }
    else
    {
        this->ui_rebootbutton->setRightHandText("Shut Down");
    }
    // this->ui_s4->setRightHandText(statusBittset & (1 << TIMER_STATUS_EXPIRED) ? "Yes" : "No");
    // this->ui_s5->setRightHandText(statusBittset & (1 << CLOCK) ? "1" : "0");
    this->ui_riseonstart->setBooleanValue(statusBittset & (1 << StatusPacketBittset::RISE_ON_START));
    this->ui_maintainprssure->setBooleanValue(statusBittset & (1 << StatusPacketBittset::MAINTAIN_PRESSURE));
#if ENABLE_AIR_OUT_ON_SHUTOFF
    this->ui_airoutonshutoff->setBooleanValue(statusBittset & (1 << StatusPacketBittset::AIR_OUT_ON_SHUTOFF));
#endif
    this->ui_heightsensormode->setSelectedOption((statusBittset & (1 << StatusPacketBittset::HEIGHT_SENSOR_MODE)) != 0 ? 1 : 0);

    this->ui_safetymode->setBooleanValue(statusBittset & (1 << StatusPacketBittset::SAFETY_MODE));

    this->ui_s2->setBooleanValue(statusBittset & (1 << StatusPacketBittset::COMPRESSOR_STATUS_ON));
    this->ui_aiEnabled->setBooleanValue(statusBittset & (1 << StatusPacketBittset::AI_STATUS_ENABLED));

    if (util_statusRequestPacket._setStatus)
    {
        util_statusRequestPacket._setStatus = false;
        this->ui_manifoldUpdateStatus->setRightHandText(util_statusRequestPacket.getStatus().c_str());
    }

    // int aiPacked = statusBittset >> 21;
    // uint8_t AIReadyBittset = aiPacked & 0b1111;
    // uint8_t AIPercentage = aiPacked >> 4;
    //  Serial.println(AIReadyBittset);

    char buf[40];
    snprintf(buf, sizeof(buf), "%i%%", AIPercentage);
    this->ui_aiPercentage->setRightHandText(buf);
    snprintf(buf, sizeof(buf), "UF:  %c UR:  %c\nDF: %c DR: %c", (AIReadyBittset & 0b1) ? 'Y' : 'n', (AIReadyBittset & 0b10 >> 1) ? 'Y' : 'n', (AIReadyBittset & 0b100 >> 2) ? 'Y' : 'n', (AIReadyBittset & 0b1000 >> 3) ? 'Y' : 'n');
    this->ui_aiReady->setRightHandText(buf);

    this->ui_mac->setRightHandText(ble_getMAC());

#if defined(WAVESHARE_BOARD)
    this->ui_volts->setRightHandText(getBatteryVoltageString());
#endif

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
