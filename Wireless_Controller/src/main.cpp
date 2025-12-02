#include <Arduino.h>
#include <Preferences.h> // have to include it here or it isn't found in the shared libs

#include <ui/ui.h>

#include "utils/touch_lib.h"
#include "tasks/tasks.h"

#include "utils/util.h"

void OnAddOneClicked(lv_event_t *e)
{
    static uint32_t cnt = 0;
    cnt++;
    lv_label_set_text_fmt(scrMain.ui_lblCountValue, "%u", cnt);
}

void OnRotateClicked(lv_event_t *e)
{
    auto disp = lv_disp_get_default();
    auto rotation = (lv_display_rotation_t)((lv_disp_get_rotation(disp) + 1) % (LV_DISPLAY_ROTATION_270 + 1));
    lv_display_set_rotation(disp, rotation);
}

#define DIM_SCREEN_TIME 60 * 1000 * getscreenDimTimeM()
unsigned long dimScreenTime = 0;
bool dimmed = false;
void setup()
{

    Serial.begin(115200);
    // esp_bt_controller_mem_release(ESP_BT_MODE_BLE);
    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT); // should free a tad bit of memory
    // Serial.setDebugOutput(true);
    log_i("Board: %s", BOARD_NAME);
    log_i("CPU: %s rev%d, CPU Freq: %d Mhz, %d core(s)", ESP.getChipModel(), ESP.getChipRevision(), getCpuFrequencyMhz(), ESP.getChipCores());
    log_i("Free heap: %d bytes", ESP.getFreeHeap());
    log_i("Free PSRAM: %d bytes", ESP.getPsramSize());
    log_i("SDK version: %s", ESP.getSdkVersion());

    setupRestSemaphore();

    beginSaveData();

    // Check if in update mode and ignore everything else and just start the web server.
    if (getupdateMode())
    {
        setupdateMode(false);
        Serial.println("Gonna try to download update");
#if defined(OTA_SUPPORTED)
        downloadUpdate(getwifiSSID(), getwifiPassword());
#endif
        return;
    }

    setup_tasks();

    board_drivers_init();

    __attribute__((unused)) auto disp = lv_disp_get_default();
    // lv_disp_set_rotation(disp, LV_DISP_ROT_90);
    // lv_disp_set_rotation(disp, LV_DISP_ROT_180);
    // lv_disp_set_rotation(disp, LV_DISP_ROT_270);

    ui_init();

    setup_touchscreen_hook();

    dimScreenTime = millis() + DIM_SCREEN_TIME;

    byte updateResult = getupdateResult();
    if (updateResult != UPDATE_STATUS::UPDATE_STATUS_NONE)
    {
        switch (updateResult)
        {
        case UPDATE_STATUS::UPDATE_STATUS_FAIL_FILE_REQUEST:
            showDialog("Update failed (file request)", lv_color_hex(0xFF0000));
            currentScr->showMsgBox("Update failed", "There was an issue obtaining the firmware file", NULL, "OK", []() -> void {}, []() -> void {}, false);
            break;
        case UPDATE_STATUS::UPDATE_STATUS_FAIL_GENERIC:
            showDialog("Update failed (generic)", lv_color_hex(0xFF0000));
            currentScr->showMsgBox("Update failed", "Unknown error installing update", NULL, "OK", []() -> void {}, []() -> void {}, false);
            break;
        case UPDATE_STATUS::UPDATE_STATUS_FAIL_VERSION_REQUEST:
            showDialog("Update failed (version request)", lv_color_hex(0xFF0000));
            currentScr->showMsgBox("Update failed", "Could not retreive the version info", NULL, "OK", []() -> void {}, []() -> void {}, false);
            break;
        case UPDATE_STATUS::UPDATE_STATUS_FAIL_WIFI_CONNECTION:
            showDialog("Update failed (wifi connection)", lv_color_hex(0xFF0000));
            currentScr->showMsgBox("Update failed", "Could not connect to wifi network. Please check your wifi SSID and password", NULL, "OK", []() -> void {}, []() -> void {}, false);
            break;
        case UPDATE_STATUS::UPDATE_STATUS_SUCCESS:
            showDialog("Update success!", lv_color_hex(0x00FF00));
            char buf[170];
            snprintf(buf, sizeof(buf), "You are now on version %s!\nPlease check the manifold update status in the update section of settings to verify the manifold was updated successfully too.", EVALUATE_AND_STRINGIFY(RELEASE_VERSION));
            currentScr->showMsgBox("Update success!", buf, NULL, "OK", []() -> void {}, []() -> void {}, false);
            break;
        }
        setupdateResult(0);
    }
    set_brightness(getBrightnessFloat());
}

// auto lv_last_tick = millis();
void loop()
{
    auto const now = millis();

    if (isTouched())
    {
        if (dimmed)
        {
            set_brightness(getBrightnessFloat());
            dimmed = false;
        }
        dimScreenTime = now + DIM_SCREEN_TIME;
    }

    if (dimScreenTime < now && dimmed == false)
    {
        set_brightness(0.01f);
        dimmed = true;
    }

    // if (isJustPressed())
    // {
    //     log_i("Just Pressed %d %d ", touchX(), touchY());
    // }
    // if (isJustReleased())
    // {
    //     log_i("Just Released %d %d ", touchX(), touchY());
    // }


    // screen code
    screenLoop();
    dialogLoop();
    safetyModeMsgBoxCheck();


    // Update the ticker
    // lv_tick_inc(now - lv_last_tick);
    // lv_last_tick = now;
    // Update the UI
    lv_timer_handler();
    vTaskDelay(pdMS_TO_TICKS(5));
}