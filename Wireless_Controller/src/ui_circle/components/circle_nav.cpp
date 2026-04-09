#include "device_lib_exports.h"

#ifdef SCREEN_MODE_CIRCLE

#include "circle_nav.h"
#include "utils/util.h"
#include "ui/ui.h"
#include "ui/components/alert.h"
#include "waveshare/waveshare.h"

CircleMenu circleMenu;

extern bool isConnectedToManifold();

static void handle_click_cb(lv_event_t *e)
{
    circleMenu.open();
}

static void gesture_cb(lv_event_t *e)
{
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
    if (dir == LV_DIR_TOP && !circleMenu.isOpen) {
        circleMenu.open();
    }
}

static void status_gesture_cb(lv_event_t *e)
{
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
    if (dir == LV_DIR_BOTTOM && !circleMenu.isOpen && !circleMenu.statusOverlay) {
        circleMenu.showStatus();
    }
}

static void status_handle_click_cb(lv_event_t *e)
{
    if (!circleMenu.isOpen && !circleMenu.statusOverlay) {
        circleMenu.showStatus();
    }
}

static void overlay_click_cb(lv_event_t *e)
{
    if (lv_event_get_target_obj(e) == circleMenu.overlay)
        circleMenu.close();
}

static void menu_btn_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    circleMenu.close();
    if (idx == 0)
        changeScreen(SCREEN_HOME, false);
    else if (idx == 1)
        changeScreen(SCREEN_SETTINGS, false);
    else if (idx == 2)
        circleMenu.showStatus();
}

static void status_dismiss_cb(lv_event_t *e)
{
    circleMenu.hideStatus();
}

void CircleMenu::create(lv_obj_t *parent)
{
    parent_ = parent;

    /* ── Large invisible gesture zone covering the bottom of the display.
     *    Detects both taps and swipe-up to open the menu.
     *    A small visible pill inside serves as a visual hint. ── */
    handle = lv_obj_create(parent);
    lv_obj_remove_style_all(handle);
    lv_obj_set_size(handle, 240, 60);
    lv_obj_set_style_bg_opa(handle, LV_OPA_TRANSP, 0);
    lv_obj_align(handle, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(handle, (lv_obj_flag_t)(LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE));
    lv_obj_remove_flag(handle, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *pill = lv_obj_create(handle);
    lv_obj_remove_style_all(pill);
    lv_obj_set_size(pill, 50, 18);
    lv_obj_set_style_radius(pill, 9, 0);
    lv_obj_set_style_bg_color(pill, lv_color_hex(GENERIC_GREY), 0);
    lv_obj_set_style_bg_opa(pill, LV_OPA_70, 0);
    lv_obj_align(pill, LV_ALIGN_TOP_MID, 0, 6);
    lv_obj_remove_flag(pill, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *chevron = lv_label_create(pill);
    lv_label_set_text(chevron, LV_SYMBOL_UP);
    lv_obj_set_style_text_color(chevron, lv_color_hex(0xC0C0C0), 0);
    lv_obj_set_style_text_font(chevron, &lv_font_montserrat_10, 0);
    lv_obj_center(chevron);

    lv_obj_add_event_cb(handle, handle_click_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(handle, gesture_cb, LV_EVENT_GESTURE, nullptr);

    /* ── Top gesture zone: swipe down to open status overlay ── */
    statusHandle = lv_obj_create(parent);
    lv_obj_remove_style_all(statusHandle);
    lv_obj_set_size(statusHandle, 240, 60);
    lv_obj_set_style_bg_opa(statusHandle, LV_OPA_TRANSP, 0);
    lv_obj_align(statusHandle, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_add_flag(statusHandle, (lv_obj_flag_t)(LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE));
    lv_obj_remove_flag(statusHandle, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(statusHandle, status_handle_click_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(statusHandle, status_gesture_cb, LV_EVENT_GESTURE, nullptr);

    /* ── Full-screen overlay (hidden until opened) ── */
    overlay = lv_obj_create(parent);
    lv_obj_remove_style_all(overlay);
    lv_obj_set_size(overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(overlay, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_80, 0);
    lv_obj_add_flag(overlay, (lv_obj_flag_t)(LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_HIDDEN));
    lv_obj_remove_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(overlay, overlay_click_cb, LV_EVENT_CLICKED, nullptr);

    /* ── 3 circular menu buttons (presets live on home) ── */
    const char *icons[] = {LV_SYMBOL_HOME, LV_SYMBOL_SETTINGS, LV_SYMBOL_EYE_OPEN};
    const char *labels[] = {"Home", "Settings", "Status"};
    struct Pos { int x; int y; };
    Pos positions[] = {{-52, -30}, {52, -30}, {0, 44}};

    const int btnSz = 60;
    for (int i = 0; i < 3; i++) {
        menuBtns[i] = lv_btn_create(overlay);
        lv_obj_set_size(menuBtns[i], btnSz, btnSz);
        lv_obj_set_style_radius(menuBtns[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(menuBtns[i], lv_color_hex(GENERIC_GREY_DARK), 0);
        lv_obj_set_style_bg_opa(menuBtns[i], LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(menuBtns[i], 2, 0);
        lv_obj_set_style_border_color(menuBtns[i], lv_color_hex(THEME_COLOR_DARK), 0);
        lv_obj_set_style_shadow_width(menuBtns[i], 15, 0);
        lv_obj_set_style_shadow_color(menuBtns[i], lv_color_hex(0x000000), 0);
        lv_obj_set_style_shadow_opa(menuBtns[i], LV_OPA_50, 0);
        lv_obj_align(menuBtns[i], LV_ALIGN_CENTER, positions[i].x, positions[i].y);

        lv_obj_t *icon = lv_label_create(menuBtns[i]);
        lv_label_set_text(icon, icons[i]);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_20, 0);
        lv_obj_center(icon);

        menuLabels[i] = lv_label_create(overlay);
        lv_label_set_text(menuLabels[i], labels[i]);
        lv_obj_set_style_text_color(menuLabels[i], lv_color_hex(0xC0C0C0), 0);
        lv_obj_set_style_text_font(menuLabels[i], &lv_font_montserrat_10, 0);
        lv_obj_align_to(menuLabels[i], menuBtns[i], LV_ALIGN_OUT_BOTTOM_MID, 0, 4);

        lv_obj_add_event_cb(menuBtns[i], menu_btn_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    }

    setActive(0);
}

void CircleMenu::open()
{
    if (!overlay) return;
    isOpen = true;
    lv_obj_remove_flag(overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(overlay);
}

void CircleMenu::close()
{
    if (!overlay) return;
    isOpen = false;
    lv_obj_add_flag(overlay, LV_OBJ_FLAG_HIDDEN);
}

void CircleMenu::setActive(uint8_t index)
{
    if (index > 1) return;
    active_ = index;
    for (int i = 0; i < 3; i++) {
        if (!menuBtns[i]) continue;
        bool sel = (i == index);
        lv_obj_set_style_bg_color(menuBtns[i],
            lv_color_hex(sel ? THEME_COLOR_LIGHT : GENERIC_GREY_DARK), 0);
        lv_obj_set_style_border_color(menuBtns[i],
            lv_color_hex(sel ? THEME_COLOR_LIGHT : THEME_COLOR_DARK), 0);
        if (sel) {
            lv_obj_set_style_shadow_color(menuBtns[i], lv_color_hex(THEME_COLOR_MEDIUM), 0);
            lv_obj_set_style_shadow_opa(menuBtns[i], LV_OPA_60, 0);
        } else {
            lv_obj_set_style_shadow_color(menuBtns[i], lv_color_hex(0x000000), 0);
            lv_obj_set_style_shadow_opa(menuBtns[i], LV_OPA_50, 0);
        }
        if (menuLabels[i])
            lv_obj_set_style_text_color(menuLabels[i],
                lv_color_hex(sel ? THEME_COLOR_LIGHT : 0xC0C0C0), 0);
    }
}

void CircleMenu::showStatus()
{
    if (statusOverlay) return;

    statusOverlay = lv_obj_create(parent_);
    lv_obj_remove_style_all(statusOverlay);
    lv_obj_set_size(statusOverlay, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(statusOverlay, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(statusOverlay, LV_OPA_80, 0);
    lv_obj_add_flag(statusOverlay, (lv_obj_flag_t)(LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE));
    lv_obj_remove_flag(statusOverlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_move_foreground(statusOverlay);

    lv_obj_t *card = lv_obj_create(statusOverlay);
    lv_obj_set_size(card, 250, 250);
    lv_obj_set_style_radius(card, 125, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(GENERIC_GREY_DARK), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(THEME_COLOR_DARK), 0);
    lv_obj_set_style_pad_all(card, 40, 0);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(card, 8, 0);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(card);
    lv_label_set_text(title, "Status");
    lv_obj_set_style_text_color(title, lv_color_hex(THEME_COLOR_LIGHT), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);

    char *battStr = getBatteryVoltageString();
    bool charging = isBatteryCharging();
    static char battBuf[32];
    snprintf(battBuf, sizeof(battBuf), "%s %s%s",
             LV_SYMBOL_BATTERY_FULL, battStr, charging ? " (charging)" : "");
    lv_obj_t *battLine = lv_label_create(card);
    lv_label_set_text(battLine, battBuf);
    lv_obj_set_style_text_color(battLine, lv_color_white(), 0);
    lv_obj_set_style_text_font(battLine, &lv_font_montserrat_14, 0);

    static char bleBuf[32];
    bool connected = isConnectedToManifold();
    snprintf(bleBuf, sizeof(bleBuf), "%s %s",
             LV_SYMBOL_BLUETOOTH, connected ? "Connected" : "Disconnected");
    lv_obj_t *bleLine = lv_label_create(card);
    lv_label_set_text(bleLine, bleBuf);
    lv_obj_set_style_text_color(bleLine,
        lv_color_hex(connected ? 0x4ADE80 : 0xF87171), 0);
    lv_obj_set_style_text_font(bleLine, &lv_font_montserrat_14, 0);

    if (isGlobalAlertActive() && !isGlobalAlertDismissed()) {
        lv_obj_t *alertLine = lv_label_create(card);
        static char alertBuf[80];
        snprintf(alertBuf, sizeof(alertBuf), "%s %s",
                 LV_SYMBOL_WARNING, globalAlertState.currentMessage);
        lv_label_set_text(alertLine, alertBuf);
        lv_obj_set_style_text_color(alertLine, lv_color_hex(0xFF4444), 0);
        lv_obj_set_style_text_font(alertLine, &lv_font_montserrat_10, 0);
        lv_obj_set_width(alertLine, 160);
        lv_label_set_long_mode(alertLine, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_align(alertLine, LV_TEXT_ALIGN_CENTER, 0);
    }

    lv_obj_t *hint = lv_label_create(card);
    lv_label_set_text(hint, "Tap to close");
    lv_obj_set_style_text_color(hint, lv_color_hex(GENERIC_GREY_LIGHT), 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_10, 0);

    /* Both the overlay background AND the card dismiss the popup.
     * The card is clickable by default (lv_obj_create), so tapping it
     * consumes the event — we must handle it on both. */
    lv_obj_add_event_cb(statusOverlay, status_dismiss_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(card, status_dismiss_cb, LV_EVENT_CLICKED, nullptr);
}

void CircleMenu::hideStatus()
{
    if (!statusOverlay) return;
    lv_obj_del(statusOverlay);
    statusOverlay = nullptr;
}

void CircleMenu::cleanup()
{
    hideStatus();
    for (int i = 0; i < 3; i++) {
        menuBtns[i] = nullptr;
        menuLabels[i] = nullptr;
    }
    overlay = nullptr;
    handle = nullptr;
    statusHandle = nullptr;
    parent_ = nullptr;
}

#endif /* SCREEN_MODE_CIRCLE */
