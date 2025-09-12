#include <Arduino.h>
#include <Preferences.h>                 // must be included here for shared libs

#include <esp32_smartdisplay.h>
#include <ui/ui.h>

#include "utils/touch_lib.h"
#include "tasks/tasks.h"
#include "utils/util.h"


#if defined(WAVESHARE_BOARD)
  #include "waveshare/waveshare.h"
#endif

// --- I2C / TCA9554 (panel reset) -------------------------
// #include <Wire.h>
// #include <TCA9554.h>

// #ifndef I2C_SDA
//   #define I2C_SDA 8
// #endif
// #ifndef I2C_SCL
//   #define I2C_SCL 7
// #endif
// #ifndef TCA_ADDR
//   #define TCA_ADDR 0x20
// #endif
// #ifndef TCA_LCD_RSTBIT
//   #define TCA_LCD_RSTBIT 1    // EXPANDER P1 pin drives LCD RST on Waveshare Type-B
// #endif

// static void panel_reset_via_tca()
// {
//   Wire.begin(I2C_SDA, I2C_SCL);
//   Wire.setClock(400000);

//   TCA9554 tca(TCA_ADDR);
//   tca.begin();
//   tca.pinMode1(TCA_LCD_RSTBIT, OUTPUT);

//   // 1 → 0 → 1 pulse; AXS15231B needs a clean low pulse then long high
//   tca.write1(TCA_LCD_RSTBIT, 1); delay(10);
//   tca.write1(TCA_LCD_RSTBIT, 0); delay(10);
//   tca.write1(TCA_LCD_RSTBIT, 1); delay(180);
// }
// ---------------------------------------------------------

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

lv_obj_t *burnInRect;
bool doBurnInFix = false;

static inline void startBurnInFix()
{
  smartdisplay_lcd_set_backlight(0.01f);
  doBurnInFix = true;
  lv_obj_remove_flag(burnInRect, LV_OBJ_FLAG_HIDDEN);
}

static inline void stopBurnInFix()
{
  smartdisplay_lcd_set_backlight(0.8f);
  doBurnInFix = false;
  lv_obj_add_flag(burnInRect, LV_OBJ_FLAG_HIDDEN);
}

#define DIM_SCREEN_TIME   (60 * 1000 * getscreenDimTimeM())
unsigned long dimScreenTime = 0;
bool dimmed = false;

void setup()
{
#ifdef ARDUINO_USB_CDC_ON_BOOT
  // delay(5000);
#endif
  Serial.begin(115200);
  // Serial.setDebugOutput(true);
  log_i("Board: %s", BOARD_NAME);
  log_i("CPU: %s rev%d, CPU Freq: %d MHz, %d core(s)", ESP.getChipModel(), ESP.getChipRevision(), getCpuFrequencyMhz(), ESP.getChipCores());
  log_i("Free heap: %d bytes", ESP.getFreeHeap());
  log_i("Free PSRAM: %d bytes", ESP.getPsramSize());
  log_i("SDK version: %s", ESP.getSdkVersion());

  setupRestSemaphore();
  beginSaveData();

  // Update-only boot path
  if (getupdateMode()) {
    setupdateMode(false);
    Serial.println("Gonna try to download update");
    downloadUpdate(getwifiSSID(), getwifiPassword());
    return;
  }

#if defined(WAVESHARE_BOARD)
  waveshare_init();
#endif

  setup_tasks();

  // ---- KEY CHANGE: ensure panel is hard-reset before SmartDisplay init ----
  // panel_reset_via_tca();

  // Optional: force BL ON during bring-up; SmartDisplay will attach LEDC later
#ifdef DISPLAY_BCKL
  pinMode(DISPLAY_BCKL, OUTPUT);
  digitalWrite(DISPLAY_BCKL, HIGH);
#endif

  // This will now bind to YOUR custom panel (lvgl_panel_axs15231b_arduino_gfx.cpp)
  // because that TU provides lvgl_lcd_init() which SmartDisplay calls.
  smartdisplay_init();

  __attribute__((unused)) auto disp = lv_disp_get_default();

  ui_init();

  burnInRect = lv_obj_create(scrHome.scr);
  lv_obj_remove_style_all(burnInRect);
  lv_obj_set_style_bg_opa(burnInRect, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_size(burnInRect, DISPLAY_WIDTH, DISPLAY_HEIGHT);
  lv_obj_center(burnInRect);
  lv_obj_set_style_bg_color(burnInRect, lv_color_hex(esp_random()), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_flag(burnInRect, LV_OBJ_FLAG_HIDDEN);

  // startBurnInFix();
  stopBurnInFix();

// #ifdef BOARD_HAS_TOUCH
  setup_touchscreen_hook();
// #endif

  dimScreenTime = millis() + DIM_SCREEN_TIME;

  // Post-update result dialog flow (unchanged)
  byte updateResult = getupdateResult();
  if (updateResult != UPDATE_STATUS::UPDATE_STATUS_NONE)
  {
    switch (updateResult)
    {
      case UPDATE_STATUS::UPDATE_STATUS_FAIL_FILE_REQUEST:
        showDialog("Update failed (file request)", lv_color_hex(0xFF0000));
        currentScr->showMsgBox("Update failed", "There was an issue obtaining the firmware file", NULL, "OK", []{}, []{}, false);
        break;
      case UPDATE_STATUS::UPDATE_STATUS_FAIL_GENERIC:
        showDialog("Update failed (generic)", lv_color_hex(0xFF0000));
        currentScr->showMsgBox("Update failed", "Unknown error installing update", NULL, "OK", []{}, []{}, false);
        break;
      case UPDATE_STATUS::UPDATE_STATUS_FAIL_VERSION_REQUEST:
        showDialog("Update failed (version request)", lv_color_hex(0xFF0000));
        currentScr->showMsgBox("Update failed", "Could not retreive the version info", NULL, "OK", []{}, []{}, false);
        break;
      case UPDATE_STATUS::UPDATE_STATUS_FAIL_WIFI_CONNECTION:
        showDialog("Update failed (wifi connection)", lv_color_hex(0xFF0000));
        currentScr->showMsgBox("Update failed", "Could not connect to wifi network. Please check your wifi SSID and password", NULL, "OK", []{}, []{}, false);
        break;
      case UPDATE_STATUS::UPDATE_STATUS_SUCCESS:
      {
        showDialog("Update success!", lv_color_hex(0x00FF00));
        char buf[160];
        snprintf(buf, sizeof(buf),
                 "You are now on version %s!\nPlease check the manifold update status in the update section of settings to verify the manifold was updated successfully too.",
                 EVALUATE_AND_STRINGIFY(RELEASE_VERSION));
        currentScr->showMsgBox("Update success!", buf, NULL, "OK", []{}, []{}, false);
        break;
      }
    }
    setupdateResult(0);
  }
}

auto lv_last_tick = millis();

void loop()
{
  auto const now = millis();

  if (isTouched()) {
    if (dimmed) {
      smartdisplay_lcd_set_backlight(0.8f);
      dimmed = false;
    }
    dimScreenTime = now + DIM_SCREEN_TIME;
  }

  if (dimScreenTime < now && !dimmed) {
    smartdisplay_lcd_set_backlight(0.01f);
    dimmed = true;
  }

  if (doBurnInFix) {
    lv_obj_set_style_bg_color(burnInRect, lv_color_hex(esp_random()), LV_PART_MAIN | LV_STATE_DEFAULT);
    if (isJustPressed()) {
      stopBurnInFix();
    }
  } else {
    // Normal UI loops
    screenLoop();
    dialogLoop();
    safetyModeMsgBoxCheck();
  }

#if defined(WAVESHARE_BOARD)
  waveshare_loop();
#endif

  // LVGL timing
  lv_tick_inc(now - lv_last_tick);
  lv_last_tick = now;

  lv_timer_handler();
}
