#include <Arduino.h>
#include <Preferences.h> // have to include it here or it isn't found in the shared libs

#if defined(OTA_SUPPORTED)
#include <directdownload.h>
#endif

#include <ui/ui.h>

#include "utils/touch_lib.h"
#include "tasks/tasks.h"

#include "utils/util.h"

SET_LOOP_TASK_STACK_SIZE(12*1024); // the default 1024*8 how now reached it's limit with lvgl. Need to increase accordingly. lv_timer_handler() is the culprit.

#define DIM_SCREEN_TIME 60 * 1000 * getscreenDimTimeM()
unsigned long dimScreenTime = 0;
bool dimmed = false;

// set pin number for the boot button
const int BootButtonPin = 0; 

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


    ui_init();

    setup_touchscreen_hook();

    dimScreenTime = millis() + DIM_SCREEN_TIME;

#if defined(OTA_SUPPORTED)
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
        case UPDATE_STATUS::UPDATE_STATUS_FAIL_ALREADY_UP_TO_DATE:
            showDialog("Update not needed", lv_color_hex(0xFFFF00));
            currentScr->showMsgBox("Update aborted", "You are already on the latest release", NULL, "OK", []() -> void {}, []() -> void {}, false);
            break;
        case UPDATE_STATUS::UPDATE_STATUS_SUCCESS:
            showDialog("Update success!", lv_color_hex(0x00FF00));
            char buf[170];
            snprintf(buf, sizeof(buf), "Welcome to version %s!\nPlease check the manifold update status in the update section of settings to verify the manifold was updated successfully too.", EVALUATE_AND_STRINGIFY(RELEASE_VERSION));
            currentScr->showMsgBox("Update success!", buf, NULL, "OK", []() -> void {}, []() -> void {}, false);
            break;
        }
        setupdateResult(0);
    }
#endif
    set_brightness(getBrightnessFloat());
    pinMode(BootButtonPin, INPUT); 
}

// auto lv_last_tick = millis();
static int lastFreeHeap = 0;
static unsigned long lastMemLog = 0;

// variables for storing the pushbutton status
int BootButtonState = 0;
unsigned long bootBtnLastPressed = 0;
unsigned long bootBtnLastReleased = 0;
unsigned long bootBtnHoldTime = 0;
bool bootButtonControllingAirUp = false;
bool bootButtonLoadPresetStarted = false;
int bootButtonPresetCount = 0;

void wakeScreenFromDim() {
    auto const now = millis();
    if (dimmed)
    {
        set_brightness(getBrightnessFloat());
        dimmed = false;
    }
    dimScreenTime = now + DIM_SCREEN_TIME;
}

int bootButtonCutoffTime = 500; // the cutoff time between a quick press and a long press
int beginAirUpAfterQuickPressActivationPeriod = 750; // the time after a quick press that we will watch for the second long press to begin the air up procedure
int beginPresetLoadingAfterNoInputPeriod = 1000; // the time after the last button press that we will start loading the preset procedure

void bootButtonFunctionality() {
    auto const now = millis();
    if (digitalRead(BootButtonPin) == LOW && BootButtonState == 0) {
        wakeScreenFromDim();
        BootButtonState = 1;
        Serial.println("Boot button pressed");
        // upon button press, check if the previous press was a quick press (<500ms) and that we aren't in the preset procedure
        // if so, start air up
        if (bootBtnHoldTime < bootButtonCutoffTime && !bootButtonLoadPresetStarted) {
            // if the current press is within 750ms of the last press, start the air up procesure
            if (now - bootBtnLastPressed < beginAirUpAfterQuickPressActivationPeriod) {
                bootButtonControllingAirUp = true;
                showDialog("Airing up while held", lv_color_hex(0x00FF00));
                // air up
                setValveBit(FRONT_DRIVER_IN);
                setValveBit(FRONT_PASSENGER_IN);
                setValveBit(REAR_DRIVER_IN);
                setValveBit(REAR_PASSENGER_IN);
            }
        }
        if (bootButtonLoadPresetStarted) {
            bootButtonPresetCount++;
            if (bootButtonPresetCount > 5) {
                showDialog("Preset loading cancelled", lv_color_hex(0xFF0000));
                bootButtonLoadPresetStarted = false;
            } else {
                static char buf[14];
                snprintf(buf, sizeof(buf), "Preset %i...", bootButtonPresetCount);
                showDialog(buf, lv_color_hex(0xFFFF00));
            }
        }
        bootBtnLastPressed = now;
    } else if (digitalRead(BootButtonPin) == HIGH && BootButtonState == 1) {
        wakeScreenFromDim();
        BootButtonState = 0;
        Serial.println("Boot button released");
        bootBtnLastReleased = now;
        bootBtnHoldTime = bootBtnLastReleased - bootBtnLastPressed;
        // if long pressed and the long press wasn't for the emergency air up, start loading the preset procedure
        if (bootBtnHoldTime > bootButtonCutoffTime && !bootButtonLoadPresetStarted && !bootButtonControllingAirUp) {
            bootButtonLoadPresetStarted = true;
            bootButtonPresetCount = 0;
            showDialog("Select preset...", lv_color_hex(0xFFFF00), 30000);
        }
        // upon button release, stop air up if it was being controlled
        if (bootButtonControllingAirUp) {
            bootButtonControllingAirUp = false;
            showDialog("Airing up stopped", lv_color_hex(0xFFFF00));
            // stop air up
            unsetValveBit(FRONT_DRIVER_IN);
            unsetValveBit(FRONT_PASSENGER_IN);
            unsetValveBit(REAR_DRIVER_IN);
            unsetValveBit(REAR_PASSENGER_IN);
        }
    }

    // check if button is currently being held
    if (bootBtnLastPressed > bootBtnLastReleased) {
        // check if held longer than 500ms, and that we aren't in the preset procedure or air up procedure
        if (now - bootBtnLastPressed > bootButtonCutoffTime && !bootButtonLoadPresetStarted && !bootButtonControllingAirUp) {
            showDialog("Select preset...", lv_color_hex(0xFFFF00), 30000);
        }
    }

    // check if preset procedure is started
    if (bootButtonLoadPresetStarted) {
        // check if button is released for longer than 1000ms (stopped changing preset numbers)
        if (now - bootBtnLastReleased > beginPresetLoadingAfterNoInputPeriod) {
            bootButtonLoadPresetStarted = false;
            if (bootButtonPresetCount >= 1 && bootButtonPresetCount <= 5) {
                // send the preset first to the manifold
                AirupQuickPacket pkt(bootButtonPresetCount - 1);
                sendRestPacket(&pkt);

                // update presets page if preset changes
                if (currentPreset != bootButtonPresetCount) {
                    scrPresets.setPreset(bootButtonPresetCount);
                }

                // show dialog of selected preset
                static char buf[20];
                snprintf(buf, sizeof(buf), "Preset %i loaded!", bootButtonPresetCount);
                showDialog(buf, lv_color_hex(0x00FF00));
            } else {
                showDialog("Preset loading cancelled", lv_color_hex(0xFF0000));
            }
        }
    }
}

void loop()
{
    auto const now = millis();

    bootButtonFunctionality();

    // Dev memory logging every 5 seconds
    if (now - lastMemLog >= 5000) {
        lastMemLog = now;
        int currentFreeHeap = ESP.getFreeHeap();
        int delta = (lastFreeHeap > 0) ? (currentFreeHeap - lastFreeHeap) : 0;
        lastFreeHeap = currentFreeHeap;

        // Stack monitoring - get high water mark (minimum free stack space ever seen)
        // Low values (< 512 bytes) indicate risk of stack overflow
        UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);

        printf("[MEM] Heap Free: %d | Min: %lu | MaxAlloc: %lu | Delta: %d | Stack Free: %u bytes\n",
              currentFreeHeap, ESP.getMinFreeHeap(), ESP.getMaxAllocHeap(), delta, stackHighWaterMark);
    }

    if (isTouched())
    {
        wakeScreenFromDim();
    }

    if (dimScreenTime < now && dimmed == false)
    {
        set_brightness(0.01f);
        dimmed = true;
    }

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