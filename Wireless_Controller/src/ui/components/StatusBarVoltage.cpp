#include "StatusBarVoltage.h"
#include <math.h>
#include "waveshare_3p5/axp2101_pmic.h"    // your wrapper (pmic::battery_voltage(), etc.)
#include "waveshare_3p5/pmic_cache.h" 

#ifndef DISPLAY_WIDTH
  #define DISPLAY_WIDTH  320
#endif

StatusBarVoltage::StatusBarVoltage() {
  col_vbus = lv_color_hex(0x4CAF50);  // green-ish
  col_bat  = lv_color_hex(0xFFC107);  // amber-ish
}

lv_obj_t* StatusBarVoltage::create(lv_obj_t* parent) {
  // container
  cont = lv_obj_create(parent);
  lv_obj_remove_style_all(cont);
  lv_obj_set_width(cont, LV_PCT(100));
  lv_obj_set_height(cont, 26);
  lv_obj_set_align(cont, LV_ALIGN_TOP_MID);
  lv_obj_set_style_bg_color(cont, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(cont, LV_OPA_80, 0);
  lv_obj_set_style_pad_left(cont, 8, 0);
  lv_obj_set_style_pad_right(cont, 8, 0);
  lv_obj_set_style_pad_top(cont, 4, 0);
  lv_obj_set_style_pad_bottom(cont, 4, 0);
  lv_obj_set_style_border_side(cont, LV_BORDER_SIDE_BOTTOM, 0);
  lv_obj_set_style_border_color(cont, lv_color_hex(0x202020), 0);
  lv_obj_set_style_border_width(cont, 1, 0);

  // label
  label = lv_label_create(cont);
  lv_label_set_text(label, "— V");
  lv_obj_set_style_text_color(label, lv_color_white(), 0);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_align(label, LV_ALIGN_LEFT_MID);

  // bring to front
  lv_obj_move_foreground(cont);

  // timer (stopped by default)
  timer = lv_timer_create(_timer_cb, period_ms, this);
  lv_timer_pause(timer);

  // initial paint
  update_now();
  return cont;
}

void StatusBarVoltage::set_mode(SBV_Source m)        { mode = m; update_now(); }
void StatusBarVoltage::set_period(uint32_t ms)       { period_ms = ms; if (timer) lv_timer_set_period(timer, period_ms); }
void StatusBarVoltage::set_height(lv_coord_t h)      { if (cont) lv_obj_set_height(cont, h); }
void StatusBarVoltage::set_padding(lv_coord_t lr, lv_coord_t tb) {
  if (!cont) return;
  lv_obj_set_style_pad_left(cont, lr, 0);
  lv_obj_set_style_pad_right(cont, lr, 0);
  lv_obj_set_style_pad_top(cont, tb, 0);
  lv_obj_set_style_pad_bottom(cont, tb, 0);
}
void StatusBarVoltage::set_bg(lv_color_t c, lv_opa_t o) {
  if (!cont) return;
  lv_obj_set_style_bg_color(cont, c, 0);
  lv_obj_set_style_bg_opa(cont, o, 0);
}
void StatusBarVoltage::set_text_colors(lv_color_t vbus, lv_color_t bat) { col_vbus = vbus; col_bat = bat; }

void StatusBarVoltage::start()      { if (timer) lv_timer_resume(timer); }
void StatusBarVoltage::stop()       { if (timer) lv_timer_pause(timer);  }
void StatusBarVoltage::update_now() { _update(); }

void StatusBarVoltage::_timer_cb(lv_timer_t *t) {
  auto *self = static_cast<StatusBarVoltage*>(lv_timer_get_user_data(t));
  if (self) self->_update();
}

// Decide which number to show (VBUS vs BAT) based on mode and validity.
// Returns volts; is_vbus tells which source was used.
float StatusBarVoltage::_pick_voltage(bool &is_vbus) const {
  PmicCache c = pmic_cache_get();

  // Prefer VBUS if present; else battery
  if (c.vbus_present) { is_vbus = true;  return c.vbus_v; }
  if (c.batt_present) { is_vbus = false; return c.batt_v; }

  // Fall back: pick any finite value if heuristics failed
  if (isfinite(c.vbus_v)) { is_vbus = true;  return c.vbus_v; }
  if (isfinite(c.batt_v)) { is_vbus = false; return c.batt_v; }
  is_vbus = false; return NAN;
}

static inline void sbv_fmt(char *buf, size_t n, const char *tag, float volts, int pct) {
  if (pct >= 0) snprintf(buf, n, "%s %.2f V  %d%%", tag, volts, pct);
  else          snprintf(buf, n, "%s %.2f V",      tag, volts);
}

void StatusBarVoltage::_update() {
  if (!label) return;

  bool is_vbus = false;
  float v = _pick_voltage(is_vbus);
  const int p = pmic_cache_get().batt_pct;

  if (!isfinite(v)) {
    lv_label_set_text(label, "N/A");
    lv_obj_set_style_text_color(label, lv_color_hex(0xAAAAAA), 0);
    return;
  }

  char txt[48];
  if (is_vbus) {
    sbv_fmt(txt, sizeof(txt), "USB", v, p);
    lv_obj_set_style_text_color(label, col_vbus, 0);
  } else {
    sbv_fmt(txt, sizeof(txt), "BAT", v, p);
    lv_obj_set_style_text_color(label, col_bat, 0);
  }
  lv_label_set_text(label, txt);
}
