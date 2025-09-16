// src/ui/components/ui_navbar.cpp
#include "ui_navbar.h"
#include "ui/ui.h"   // for changeScreen(SCREEN_*)

// -------- LVGL 8/9 safe alloc helpers --------
#if (defined(LVGL_VERSION_MAJOR) && (LVGL_VERSION_MAJOR >= 9)) \
 || (defined(LV_VERSION_MAJOR) && (LV_VERSION_MAJOR >= 9))
  #define NAV_MALLOC  lv_malloc
  #define NAV_FREE    lv_free
#else
  #define NAV_MALLOC  lv_mem_alloc
  #define NAV_FREE    lv_mem_free
#endif
// ---------------------------------------------

static constexpr lv_coord_t NAV_H   = 49;  // navbar height
static constexpr lv_coord_t IND_H   = 3;   // indicator height
static constexpr lv_coord_t IND_PAD = 0;   // side padding for the indicator (0 = full width)

// Theme-ish colors (swap to your THEME_* if you prefer)
static inline lv_color_t C_BG()      { return lv_color_hex(0x151515); }
static inline lv_color_t C_ACTIVE()  { return lv_color_hex(0xB48EFF); }
static inline lv_color_t C_TEXTDIM() { return lv_color_hex(0xB0B0B0); }

struct NavCtx {
    lv_obj_t* nav;
    lv_obj_t* indicator;
    lv_obj_t* slots[3];   // each slot is a container (active: plain obj, inactive: button)
    int       active;
};

static void nav_free_cb(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_DELETE) return;
    auto* ctx = (NavCtx*)lv_event_get_user_data(e);
    if (ctx) NAV_FREE(ctx);
}

static NavCtx* get_ctx(lv_obj_t* nav) {
    uint16_t n = lv_obj_get_event_count(nav);
    for (uint16_t i = 0; i < n; ++i) {
        lv_event_dsc_t* d = lv_obj_get_event_dsc(nav, i);
        if (d && lv_event_dsc_get_cb(d) == nav_free_cb)
            return (NavCtx*)lv_event_dsc_get_user_data(d);
    }
    return nullptr;
}

static void move_indicator(NavCtx* ctx) {
    int idx = ctx->active;
    lv_area_t a_slot{}, a_nav{};
    lv_obj_get_coords(ctx->slots[idx], &a_slot);
    lv_obj_get_coords(ctx->nav,       &a_nav);

    lv_coord_t bw = lv_obj_get_width(ctx->slots[idx]);               // full slot width
    lv_coord_t iw = LV_MAX((lv_coord_t)1, (lv_coord_t)(bw - 2*IND_PAD));
    lv_coord_t ix = (a_slot.x1 - a_nav.x1) + IND_PAD;

    lv_obj_set_size(ctx->indicator, iw, IND_H);
    lv_obj_set_pos (ctx->indicator, ix, 0);                           // top edge underline
    lv_obj_move_foreground(ctx->indicator);
}

static void nav_size_changed_cb(lv_event_t* e) {
    auto* nav = (lv_obj_t*)lv_event_get_target(e);
    if (auto* ctx = get_ctx(nav)) move_indicator(ctx);
}

static void on_btn(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    auto* btn = (lv_obj_t*)lv_event_get_target(e);
    auto* nav = lv_obj_get_parent(btn);
    auto* ctx = get_ctx(nav);
    if (!ctx) return;

    int idx = 0;
    for (int i = 0; i < 3; ++i) if (ctx->slots[i] == btn) { idx = i; break; }

    switch ((NavId)idx) {
        case NAV_HOME:     changeScreen(SCREEN_HOME);     break;
        case NAV_PRESETS:  changeScreen(SCREEN_PRESETS);  break;
        case NAV_SETTINGS: changeScreen(SCREEN_SETTINGS); break;
    }
}

lv_obj_t* navbar_create(lv_obj_t* parent, NavId active) {
    // ---------- styles (static, init once) ----------
    static lv_style_t st_nav, st_lbl_active, st_indicator;
    static bool inited = false;
    if (!inited) {
        lv_style_init(&st_nav);
        lv_style_set_bg_color(&st_nav, C_BG());
        lv_style_set_bg_opa  (&st_nav, LV_OPA_COVER);
        lv_style_set_border_width(&st_nav, 0);
        lv_style_set_pad_all (&st_nav, 0);
        lv_style_set_pad_row (&st_nav, 0);
        lv_style_set_pad_column(&st_nav, 0);

        lv_style_init(&st_lbl_active);
        lv_style_set_text_color (&st_lbl_active, C_ACTIVE());
        lv_style_set_text_align (&st_lbl_active, LV_TEXT_ALIGN_CENTER);
        lv_style_set_text_decor (&st_lbl_active, LV_TEXT_DECOR_NONE);

        lv_style_init(&st_indicator);
        lv_style_set_bg_color (&st_indicator, C_ACTIVE());
        lv_style_set_bg_opa   (&st_indicator, LV_OPA_COVER);
        lv_style_set_border_width(&st_indicator, 0);
        lv_style_set_radius   (&st_indicator, LV_RADIUS_CIRCLE);
        inited = true;
    }

    // ---------- container ----------
    lv_obj_t* nav = lv_obj_create(parent);
    lv_obj_add_style(nav, &st_nav, 0);
    lv_obj_set_width (nav, lv_pct(100));
    lv_obj_set_height(nav, NAV_H);
    lv_obj_align(nav, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_clear_flag(nav, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(nav, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(nav,
        LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // ctx stored on nav; freed on delete
    auto* ctx = (NavCtx*)NAV_MALLOC(sizeof(NavCtx));
    ctx->nav = nav;
    ctx->active = (int)active;
    ctx->indicator = nullptr;

    lv_obj_add_event_cb(nav, nav_free_cb,         LV_EVENT_DELETE,       ctx);
    lv_obj_add_event_cb(nav, nav_size_changed_cb, LV_EVENT_SIZE_CHANGED, nullptr);

    // ---------- indicator (ignore layout so we can position it absolutely) ----------
    ctx->indicator = lv_obj_create(nav);
    lv_obj_add_style(ctx->indicator, &st_indicator, 0);
    lv_obj_set_size(ctx->indicator, 1, IND_H);
    lv_obj_add_flag(ctx->indicator, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_pos (ctx->indicator, 0, 0);

    // ---------- helpers to create slots ----------
    auto add_active_slot = [&](const char* txt)->lv_obj_t* {
        // plain container for active tab
        lv_obj_t* slot = lv_obj_create(nav);
        lv_obj_remove_style_all(slot);
        lv_obj_set_width (slot, lv_pct(33));
        lv_obj_set_height(slot, NAV_H);
        lv_obj_clear_flag(slot, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* l = lv_label_create(slot);
        lv_label_set_text(l, txt);
        lv_obj_add_style(l, &st_lbl_active, 0);
        lv_obj_center(l);                               // true center in the slot

        return slot;
    };

    auto add_inactive_slot = [&](const char* txt)->lv_obj_t* {
        // button for inactive tab (but visually flat)
        lv_obj_t* slot = lv_button_create(nav);

        // strip all theme visuals
        lv_obj_remove_style_all(slot);
        lv_obj_clear_flag(slot, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(slot, LV_OBJ_FLAG_CLICK_FOCUSABLE);

        lv_obj_set_width (slot, lv_pct(33));
        lv_obj_set_height(slot, NAV_H);

        // inner label
        lv_obj_t* l = lv_label_create(slot);
        lv_label_set_text(l, txt);
        lv_obj_set_style_text_color(l, C_TEXTDIM(), 0);
        lv_obj_center(l);

        lv_obj_add_event_cb(slot, on_btn, LV_EVENT_CLICKED, nullptr);
        return slot;
    };

    // ---------- build three slots ----------
    ctx->slots[0] = (active == NAV_HOME)     ? add_active_slot("Home")
                                             : add_inactive_slot("Home");
    ctx->slots[1] = (active == NAV_PRESETS)  ? add_active_slot("Presets")
                                             : add_inactive_slot("Presets");
    ctx->slots[2] = (active == NAV_SETTINGS) ? add_active_slot("Settings")
                                             : add_inactive_slot("Settings");

    // place indicator after layout is known
    lv_obj_update_layout(nav);
    move_indicator(ctx);

    lv_obj_move_foreground(nav); // keep nav above your content if needed
    return nav;
}
