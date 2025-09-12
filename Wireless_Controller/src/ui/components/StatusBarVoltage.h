#pragma once
#include <lvgl.h>
#include <stdint.h>

enum class SBV_Source : uint8_t { AUTO, VBUS, BATTERY };

class StatusBarVoltage {
public:
  StatusBarVoltage();

  // Create and attach to a parent (usually the root screen or a top layer)
  lv_obj_t* create(lv_obj_t* parent);

  // Controls
  void start();
  void stop();
  void update_now();

  // Config
  void set_mode(SBV_Source m);
  void set_period(uint32_t ms);
  void set_height(lv_coord_t h);
  void set_padding(lv_coord_t left_right, lv_coord_t top_bottom);
  void set_bg(lv_color_t color, lv_opa_t opa);
  void set_text_colors(lv_color_t vbus, lv_color_t bat);

  // Access
  lv_obj_t* container() const { return cont; }
  lv_obj_t* label_obj() const { return label; }

private:
  static void _timer_cb(lv_timer_t *t);
  void _update();
  float _pick_voltage(bool &is_vbus) const;

  lv_obj_t   *cont  = nullptr;
  lv_obj_t   *label = nullptr;
  lv_timer_t *timer = nullptr;

  SBV_Source  mode      = SBV_Source::AUTO;
  uint32_t    period_ms = 1000;

  // default colors
  lv_color_t  col_vbus;
  lv_color_t  col_bat;
};
