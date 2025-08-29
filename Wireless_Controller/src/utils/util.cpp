// util.cpp — resolution-independent, matches your util.h
#include "util.h"
#include <stdint.h>
#include <string.h>
#include <Arduino.h>
#include <lvgl.h>

#include "BTOas.h"
#include "preferencable.h"

extern "C" {
  #include "freertos/FreeRTOS.h"
  #include "freertos/semphr.h"
}

// -------------------------------------------------------------------------------------------------
// Resolution helpers
// -------------------------------------------------------------------------------------------------

static constexpr int REF_W = 240;
static constexpr int REF_H = 320;

static inline int disp_w() {
#ifdef DISPLAY_WIDTH
  return DISPLAY_WIDTH;
#else
  lv_display_t* d = lv_disp_get_default();
  return d ? (int)lv_display_get_horizontal_resolution(d) : REF_W;
#endif
}

static inline int disp_h() {
#ifdef DISPLAY_HEIGHT
  return DISPLAY_HEIGHT;
#else
  lv_display_t* d = lv_disp_get_default();
  return d ? (int)lv_display_get_vertical_resolution(d) : REF_H;
#endif
}

// ---- display -> author (for hit-tests) ----
static inline SimplePoint to_ref(SimplePoint p_disp) {
  SimplePoint q;
  q.x = ((double) p_disp.x) * (double)REF_W / (double)disp_w();
  q.y = ((double) p_disp.y) * (double)REF_H / (double)disp_h();
  return q;
}

// ---- author -> display (for drawing) ----
static inline double sx_d(double v_ref) { return v_ref * (double)disp_w() / (double)REF_W; }
static inline double sy_d(double v_ref) { return v_ref * (double)disp_h() / (double)REF_H; }

// Public drawing-time scaling helpers declared in util.h
CenterRect scaleCR(const CenterRect &c) {
  CenterRect out;
  out.cx = sx_d(c.cx);
  out.cy = sy_d(c.cy);
  out.w  = sx_d(c.w);   // uniform scaling in X/Y; matches authored aspect
  out.h  = sy_d(c.h);
  return out;
}
SimpleRect scaleSR(const SimpleRect &r) {
  SimpleRect out;
  out.x = sx_d(r.x);
  out.y = sy_d(r.y);
  out.w = sx_d(r.w);
  out.h = sy_d(r.h);
  return out;
}

// -------------------------------------------------------------------------------------------------
// Hit-testing (display-space point -> author-space rects)
// -------------------------------------------------------------------------------------------------

static inline int contains_raw(SimpleRect r, SimplePoint p_ref)
{
  return (p_ref.x >= r.x) && (p_ref.y >= r.y) &&
         (p_ref.x <  r.x + r.w) && (p_ref.y <  r.y + r.h);
}

int sr_contains(SimpleRect r, SimplePoint p_disp)
{
  SimplePoint p_ref = to_ref(p_disp);
  return contains_raw(r, p_ref);
}

int cr_contains(CenterRect cr, SimplePoint p_disp)
{
  SimplePoint p_ref = to_ref(p_disp);
  SimpleRect r = { cr.cx - (cr.w/2.0), cr.cy - (cr.h/2.0), cr.w, cr.h };
  return contains_raw(r, p_ref);
}

// -------------------------------------------------------------------------------------------------
// Authored rectangles (240x320) — unchanged from your layout
// -------------------------------------------------------------------------------------------------

// first column (left)
CenterRect ctr_row0col0up   = {  48,  89, ARROW_BUTTON_WIDTH, ARROW_BUTTON_HEIGHT };
CenterRect ctr_row0col0down = {  48, 137, ARROW_BUTTON_WIDTH, ARROW_BUTTON_HEIGHT };

CenterRect ctr_row1col0up   = {  48, 193, ARROW_BUTTON_WIDTH, ARROW_BUTTON_HEIGHT };
CenterRect ctr_row1col0down = {  48, 241, ARROW_BUTTON_WIDTH, ARROW_BUTTON_HEIGHT };

// second column (center)
CenterRect ctr_row0col1up   = { 118,  78, ARROW_BUTTON_WIDTH, ARROW_BUTTON_HEIGHT };
CenterRect ctr_row0col1down = { 118, 126, ARROW_BUTTON_WIDTH, ARROW_BUTTON_HEIGHT };

CenterRect ctr_row1col1up   = { 118, 204, ARROW_BUTTON_WIDTH, ARROW_BUTTON_HEIGHT };
CenterRect ctr_row1col1down = { 118, 252, ARROW_BUTTON_WIDTH, ARROW_BUTTON_HEIGHT };

// third column (right)
CenterRect ctr_row0col2up   = { 189,  89, ARROW_BUTTON_WIDTH, ARROW_BUTTON_HEIGHT };
CenterRect ctr_row0col2down = { 189, 137, ARROW_BUTTON_WIDTH, ARROW_BUTTON_HEIGHT };

CenterRect ctr_row1col2up   = { 189, 193, ARROW_BUTTON_WIDTH, ARROW_BUTTON_HEIGHT };
CenterRect ctr_row1col2down = { 189, 241, ARROW_BUTTON_WIDTH, ARROW_BUTTON_HEIGHT };

// bottom nav
SimpleRect navbarbtn_home     = {   0, 291,  80, 29 };
SimpleRect navbarbtn_presets  = {  80, 291,  80, 29 };
SimpleRect navbarbtn_settings = { 160, 291,  80, 29 };

// presets buttons
CenterRect ctr_preset_1 = { 48 / 2 + 48 * 0, 182, 48, 48 };
CenterRect ctr_preset_2 = { 48 / 2 + 48 * 1, 182, 48, 48 };
CenterRect ctr_preset_3 = { 48 / 2 + 48 * 2, 182, 48, 48 };
CenterRect ctr_preset_4 = { 48 / 2 + 48 * 3, 182, 48, 48 };
CenterRect ctr_preset_5 = { 48 / 2 + 48 * 4, 182, 48, 48 };

// preset save and load
SimpleRect preset_save = {  18, 235,  91 - 18, 251 - 235 };
SimpleRect preset_load = { 110, 225, 221 - 110, 256 - 225 };

// -------------------------------------------------------------------------------------------------
// Pressure/state globals
// -------------------------------------------------------------------------------------------------

int      currentPressures[5];
uint32_t statusBittset = 0;
uint8_t  AIPercentage = 0;
uint8_t  AIReadyBittset = 0;
uint8_t  manifoldUpdateStatus = 0;
int      profilePressures[5][4];
bool     profileUpdated = false;
int      currentPreset = -1;

void requestPreset()
{
  PresetPacket pkt(currentPreset - 1, 0, 0, 0, 0);
  sendRestPacket(&pkt);
}

// -------------------------------------------------------------------------------------------------
// Config save/load
// -------------------------------------------------------------------------------------------------

ConfigValuesPacket util_configValues(0, 0, 0, 0, 0, 0, 0);

void sendConfigValuesPacket(bool saveToManifold)
{
  *util_configValues._setValues() = saveToManifold;
  sendRestPacket(&util_configValues);
}

SaveData _SaveData;
void beginSaveData()
{
  _SaveData.unitsMode.load("units", UNITS_MODE::PSI);
  _SaveData.blePasskey.load("blePasskey", 202777);
  _SaveData.screenDimTimeM.load("screenDimTimeM", 3);
  _SaveData.updateMode.load("updateMode", false);
  _SaveData.wifiSSID.loadString("wifiSSID", "");
  _SaveData.wifiPassword.loadString("wifiPassword", "");
  _SaveData.updateResult.load("updateResult", 0);
}

createSaveFuncInt(unitsMode, int);
createSaveFuncInt(blePasskey, uint32_t);
createSaveFuncInt(screenDimTimeM, uint32_t);
createSaveFuncInt(updateMode, bool);
createSaveFuncString(wifiSSID);
createSaveFuncString(wifiPassword);
createSaveFuncInt(updateResult, byte);

// -------------------------------------------------------------------------------------------------
// Dialog broadcast across screens
// -------------------------------------------------------------------------------------------------

Scr *screens[3];
Scr *currentScr = NULL;

unsigned long dialogEndTime = 0;
lv_color_t    dialogColor;
char          dialogText[50];
bool          updateDialog = false;

void showDialog(const char *text, lv_color_t color, unsigned long durationMS)
{
  strncpy(dialogText, text, sizeof(dialogText));
  dialogText[sizeof(dialogText)-1] = '\0';
  memcpy(&dialogColor, &color, sizeof(lv_color_t));
  dialogEndTime = millis() + durationMS;
  updateDialog = true;
}

void dialogLoop()
{
  if (updateDialog)
  {
    screens[0]->alert->show(dialogColor, dialogText, dialogEndTime);
    screens[1]->alert->show(dialogColor, dialogText, dialogEndTime);
    screens[2]->alert->show(dialogColor, dialogText, dialogEndTime);
    updateDialog = false;
  }
}

// -------------------------------------------------------------------------------------------------
// Valve control helpers
// -------------------------------------------------------------------------------------------------

static unsigned int valveControlValue = 0;

unsigned int getValveControlValue() { return valveControlValue; }
void setValveBit(int bit)           { valveControlValue |= (1u << bit); }
void closeValves()                  { valveControlValue = 0; }

// -------------------------------------------------------------------------------------------------
// UI helpers
// -------------------------------------------------------------------------------------------------

void setupPressureLabel(lv_obj_t *parent, lv_obj_t **label, int x, int y, lv_align_t align, const char *defaultText)
{
  *label = lv_label_create(parent);
  lv_obj_set_style_text_color(*label, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_width(*label, LV_SIZE_CONTENT);
  lv_obj_set_height(*label, LV_SIZE_CONTENT);
  lv_obj_set_x(*label, x);
  lv_obj_set_y(*label, y);
  lv_obj_set_align(*label, align);
  lv_label_set_text(*label, defaultText);
  lv_obj_move_foreground(*label);
}

// -------------------------------------------------------------------------------------------------
// BLE “REST” packet ring + semaphore
// -------------------------------------------------------------------------------------------------

#define BTOASPACKETCOUNT 10
struct PacketEntry {
  bool taken;
  BTOasPacket packet;
};

static PacketEntry       packets[BTOASPACKETCOUNT];
static SemaphoreHandle_t restMutex;

void setupRestSemaphore()
{
  restMutex = xSemaphoreCreateMutex();
  memset(packets, 0, sizeof(packets));
}

static inline void waitRestSemaphore()
{
  while (xSemaphoreTake(restMutex, 1) != pdTRUE) { delay(1); }
}

static inline void giveRestSemaphore() { xSemaphoreGive(restMutex); }

void clearPackets()
{
  waitRestSemaphore();
  for (int i = 0; i < BTOASPACKETCOUNT; i++) packets[i].taken = false;
  giveRestSemaphore();
}

bool getBTRestPacketToSend(BTOasPacket *copyTo)
{
  bool ret = false;
  waitRestSemaphore();
  for (int i = 0; i < BTOASPACKETCOUNT; i++) {
    if (packets[i].taken) {
      packets[i].taken = false;
      memcpy(copyTo, &packets[i].packet, BTOAS_PACKET_SIZE);
      ret = true;
      break;
    }
  }
  giveRestSemaphore();
  return ret;
}

void sendRestPacket(BTOasPacket *packet)
{
  waitRestSemaphore();
  for (int i = 0; i < BTOASPACKETCOUNT; i++) {
    if (!packets[i].taken) {
      packets[i].taken = true;
      memcpy(&packets[i].packet, packet, BTOAS_PACKET_SIZE);
      break;
    }
  }
  giveRestSemaphore();
}

// -------------------------------------------------------------------------------------------------
// LVGL keyboard helpers
// -------------------------------------------------------------------------------------------------

static lv_obj_t *kb = NULL;

static void defocus(Option *option)
{
  lv_obj_send_event(option->root, LV_EVENT_FOCUSED, NULL);
  lv_obj_send_event(option->rightHandObj, LV_EVENT_DEFOCUSED, NULL);
  lv_obj_remove_state(option->rightHandObj, LV_STATE_FOCUSED);
  lv_group_focus_obj(option->root);
}

static void kb_event_cb(lv_event_t *e)
{
  lv_event_code_t event_code = lv_event_get_code(e);
  Option *option = (Option *)lv_event_get_user_data(e);

  if (event_code == LV_EVENT_CANCEL) {
    if (option) defocus(option);
    if (kb) {
      lv_keyboard_set_textarea(kb, NULL);
      lv_obj_del(kb);
      kb = NULL;
    }
  }
  if (event_code == LV_EVENT_READY) {
    if (option) {
      defocus(option);
      Serial.println(lv_textarea_get_text(option->rightHandObj));
      if (option->type == OptionType::KEYBOARD_INPUT_NUMBER) {
        option->event_cb((void *)atoi(lv_textarea_get_text(option->rightHandObj)));
      } else {
        option->event_cb((void *)lv_textarea_get_text(option->rightHandObj));
      }
    }
    if (kb) {
      lv_keyboard_set_textarea(kb, NULL);
      lv_obj_del(kb);
      kb = NULL;
    }
  }
}

void initKB(Option *option)
{
  if (kb) {
    lv_keyboard_set_textarea(kb, NULL);
    lv_obj_del(kb);
    kb = NULL;
  }
  kb = lv_keyboard_create(lv_scr_act());
  if (option->type == OptionType::KEYBOARD_INPUT_NUMBER) {
    lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_NUMBER);
  } else {
    lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_TEXT_LOWER);
  }
  lv_obj_add_event_cb(kb, kb_event_cb, LV_EVENT_ALL, option);
  lv_obj_set_style_bg_color(kb, lv_color_hex(THEME_COLOR_DARK),   LV_PART_MAIN  | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(kb, lv_color_hex(THEME_COLOR_LIGHT),  LV_PART_ITEMS | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(kb, lv_color_hex(THEME_COLOR_LIGHT),  LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_set_style_bg_color(kb, lv_color_hex(THEME_COLOR_MEDIUM), LV_PART_ITEMS | LV_STATE_FOCUSED);
}

void ta_event_cb(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *ta = (lv_obj_t *)lv_event_get_target(e);
  Option *option = (Option *)lv_event_get_user_data(e);

  if (code == LV_EVENT_FOCUSED) {
    initKB(option);
    lv_keyboard_set_textarea(kb, ta);
  }
  if (code == LV_EVENT_DEFOCUSED) {
    if (kb) {
      lv_keyboard_set_textarea(kb, NULL);
      lv_obj_del(kb);
      kb = NULL;
    }
  }
}

bool isKeyboardHidden()
{
  return kb == NULL;
}

// -------------------------------------------------------------------------------------------------
// Next-frame runner
// -------------------------------------------------------------------------------------------------

std::function<void()> functionToRunOnNextFrame = NULL;
bool doRunFunctionNextFrame = false;

void runNextFrame(std::function<void()> function)
{
  functionToRunOnNextFrame = function;
  doRunFunctionNextFrame  = true;
}

void handleFunctionRunOnNextFrame()
{
  if (doRunFunctionNextFrame) {
    doRunFunctionNextFrame = false;
    functionToRunOnNextFrame();
  }
}

// -------------------------------------------------------------------------------------------------
// BLE connection completed
// -------------------------------------------------------------------------------------------------

void onBLEConnectionCompleted()
{
  sendConfigValuesPacket(false); // request manifold's saved data
  requestPreset();               // request current preset values
}
