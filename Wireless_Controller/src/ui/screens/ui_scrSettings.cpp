#include "ui_scrSettings.h"

LV_IMG_DECLARE(navbar_settings);
ScrSettings scrSettings(navbar_settings, false);

void alertValueUpdated()
{
    showDialog("Set value", lv_color_hex(0xFFFF00));
}

const char *test = "";
extern bool isConnectedToManifold(); // from ble.cpp
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
    new Option(this->optionsContainer, OptionType::HEADER, "ML/AI");
    this->ui_aiPercentage = new Option(this->optionsContainer, OptionType::TEXT_WITH_VALUE, "Learn Progress:", defaultCharVal);
    this->ui_aiReady = new Option(this->optionsContainer, OptionType::TEXT_WITH_VALUE, "Trained:", defaultCharVal);
    this->ui_aiEnabled = new Option(this->optionsContainer, OptionType::ON_OFF, "Enabled:", defaultCharVal, [](void *data)
                             { 
                AIStatusPacket pkt(((bool)data));
                sendRestPacket(&pkt);
                log_i("Pressed ai status %i", ((bool)data)); });

    new Option(this->optionsContainer, OptionType::SPACE, "");
    new Option(this->optionsContainer, OptionType::HEADER, "Basic settings");
    this->ui_maintainprssure = new Option(this->optionsContainer, OptionType::ON_OFF, "Maintain pressure", defaultCharVal, [](void *data)
                                          { 
                MaintainPressurePacket pkt(((bool)data));
                sendRestPacket(&pkt);
                log_i("Pressed maintain pressure %i", ((bool)data)); });
    this->ui_riseonstart = new Option(this->optionsContainer, OptionType::ON_OFF, "Rise on start", defaultCharVal, [](void *data)
                                      { 
                RiseOnStartPacket pkt(((bool)data));
                sendRestPacket(&pkt);
                log_i("Pressed riseonstart %i", ((bool)data)); });
    // this->ui_airoutonshutoff = new Option(this->optionsContainer, OptionType::ON_OFF, "Fall on shutdown", defaultCharVal, [](void *data)
    //                                       {
    //             FallOnShutdownPacket pkt(((bool)data));
    //             sendRestPacket(&pkt);
    //             log_i("Pressed fallonshutdown %i", ((bool)data)); });

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

    new Option(this->optionsContainer, OptionType::SPACE, "", defaultCharVal);
    new Option(this->optionsContainer, OptionType::HEADER, "Config");

    this->ui_config1 = new Option(this->optionsContainer, OptionType::KEYBOARD_INPUT_NUMBER, "Bag Max PSI" /*"MAX_PRESSURE_SAFETY"*/, {.INT = 0}, [](void *data)
                                  { log_i("Pressed %i", ((uint32_t)data));
        *util_configValues._bagMaxPressure() = (uint32_t)data;
        sendConfigValuesPacket(true);
    alertValueUpdated(); });

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

    this->ui_config6 = new Option(this->optionsContainer, OptionType::KEYBOARD_INPUT_NUMBER, "Bag Volume Percentage", {.INT = 0}, [](void *data)
                                  { log_i("Pressed %i", ((uint32_t)data)); 
        *util_configValues._bagVolumePercentage() = (uint32_t)data;
        sendConfigValuesPacket(true);
    alertValueUpdated(); });

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
    const char *qr_data = "https://github.com/gopro2027/ArduinoAirSuspensionController";
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
    // this->ui_airoutonshutoff->setBooleanValue(statusBittset & (1 << StatusPacketBittset::AIR_OUT_ON_SHUTOFF));
    this->ui_heightsensormode->setSelectedOption((statusBittset & (1 << StatusPacketBittset::HEIGHT_SENSOR_MODE)) != 0 ? 1 : 0);

    this->ui_safetymode->setBooleanValue(statusBittset & (1 << StatusPacketBittset::SAFETY_MODE));

    this->ui_s2->setBooleanValue(statusBittset & (1 << StatusPacketBittset::COMPRESSOR_STATUS_ON));
    this->ui_aiEnabled->setBooleanValue(statusBittset & (1 << StatusPacketBittset::AI_STATUS_ENABLED));

    int aiPacked = statusBittset >> 21;
    uint8_t AIReadyBittset = aiPacked & 0b1111;
    uint8_t AIPercentage = aiPacked >> 4;
    //Serial.println(AIReadyBittset);

    char buf[40];
    snprintf(buf,sizeof(buf),"%i%%",AIPercentage);
    this->ui_aiPercentage->setRightHandText(buf);
    snprintf(buf,sizeof(buf),"UF:  %c UR:  %c\nDF: %c DR: %c", (AIReadyBittset & 0b1)?'Y':'n', (AIReadyBittset & 0b10 >> 1)?'Y':'n', (AIReadyBittset & 0b100 >> 2)?'Y':'n',(AIReadyBittset & 0b1000 >> 3)?'Y':'n');
    this->ui_aiReady->setRightHandText(buf);

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
