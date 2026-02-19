#include "alert.h"

// Dynamic toast dimensions based on screen size
#define TOAST_WIDTH (getScreenWidth() - scaledX(20))
#define TOAST_HEIGHT scaledY(50)
#define TOAST_MARGIN scaledY(10)
#define TOAST_PADDING_X scaledX(12)
#define TOAST_PADDING_Y scaledY(8)
#define TOAST_BORDER_RADIUS scaledX(12)
#define DISMISS_BTN_SIZE scaledX(24)
#define ACCENT_BAR_WIDTH scaledX(4)

// Global alert state - shared across all screens
GlobalAlertState globalAlertState = {"", {0}, false, false, NULL};

void globalAlertSetActive(const char *message, lv_color_t color, void *originScreen)
{
    strncpy(globalAlertState.currentMessage, message, sizeof(globalAlertState.currentMessage) - 1);
    globalAlertState.currentMessage[sizeof(globalAlertState.currentMessage) - 1] = '\0';
    globalAlertState.currentColor = color;
    globalAlertState.hasActiveAlert = true;
    globalAlertState.isDismissed = false;
    globalAlertState.originScreen = originScreen;
}

void globalAlertDismiss()
{
    globalAlertState.isDismissed = true;
}

void globalAlertClear()
{
    globalAlertState.currentMessage[0] = '\0';
    globalAlertState.hasActiveAlert = false;
    globalAlertState.isDismissed = false;
    globalAlertState.originScreen = NULL;
}

bool isGlobalAlertActive()
{
    return globalAlertState.hasActiveAlert && !globalAlertState.isDismissed;
}

bool isGlobalAlertDismissed()
{
    return globalAlertState.hasActiveAlert && globalAlertState.isDismissed;
}

// Callback for dismiss button click
static void dismiss_btn_cb(lv_event_t *e)
{
    Alert *alert = (Alert *)lv_event_get_user_data(e);
    if (alert != NULL)
    {
        alert->dismiss();
    }
}

Alert::Alert(Scr *scr)
{
    this->parentScr = scr;
    this->dismissed = false;
    this->lastMessage[0] = '\0';
    this->lastColor = lv_color_hex(THEME_COLOR_LIGHT);

    // ============================================
    // MAIN TOAST CONTAINER (top of screen)
    // ============================================
    this->container = lv_obj_create(scr->scr);
    lv_obj_remove_style_all(this->container);

    // Position at top with margin
    lv_obj_set_width(this->container, TOAST_WIDTH);
    lv_obj_set_height(this->container, TOAST_HEIGHT);
    lv_obj_set_align(this->container, LV_ALIGN_TOP_MID);
    lv_obj_set_y(this->container, STATUSBAR_HEIGHT + TOAST_MARGIN);

    // Modern dark background with subtle transparency
    lv_obj_set_style_bg_opa(this->container, LV_OPA_90, LV_PART_MAIN);
    lv_obj_set_style_bg_color(this->container, lv_color_hex(0x1E1E2E), LV_PART_MAIN);

    // Rounded corners for modern look
    lv_obj_set_style_radius(this->container, TOAST_BORDER_RADIUS, LV_PART_MAIN);

    // Subtle shadow effect via border
    lv_obj_set_style_border_width(this->container, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(this->container, lv_color_hex(0x313244), LV_PART_MAIN);
    lv_obj_set_style_border_opa(this->container, LV_OPA_80, LV_PART_MAIN);

    // Padding
    lv_obj_set_style_pad_left(this->container, TOAST_PADDING_X + ACCENT_BAR_WIDTH + scaledX(4), LV_PART_MAIN);
    lv_obj_set_style_pad_right(this->container, TOAST_PADDING_X + DISMISS_BTN_SIZE, LV_PART_MAIN);
    lv_obj_set_style_pad_top(this->container, TOAST_PADDING_Y, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(this->container, TOAST_PADDING_Y, LV_PART_MAIN);

    lv_obj_remove_flag(this->container, LV_OBJ_FLAG_SCROLLABLE);

    // ============================================
    // ACCENT COLOR BAR (left side indicator)
    // ============================================
    lv_obj_t *accentBar = lv_obj_create(this->container);
    lv_obj_remove_style_all(accentBar);
    lv_obj_set_size(accentBar, ACCENT_BAR_WIDTH, TOAST_HEIGHT - (TOAST_PADDING_Y * 2) - scaledY(8));
    lv_obj_set_align(accentBar, LV_ALIGN_LEFT_MID);
    lv_obj_set_x(accentBar, -TOAST_PADDING_X);
    lv_obj_set_style_bg_opa(accentBar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(accentBar, lv_color_hex(THEME_COLOR_LIGHT), LV_PART_MAIN);
    lv_obj_set_style_radius(accentBar, scaledX(2), LV_PART_MAIN);
    lv_obj_remove_flag(accentBar, LV_OBJ_FLAG_SCROLLABLE);
    // Store reference to update color later
    lv_obj_set_user_data(this->container, accentBar);

    // ============================================
    // MESSAGE TEXT
    // ============================================
    this->text = lv_label_create(this->container);
    lv_obj_set_style_text_color(this->text, lv_color_hex(0xCDD6F4), 0);
    lv_obj_set_width(this->text, TOAST_WIDTH - TOAST_PADDING_X * 2 - DISMISS_BTN_SIZE - ACCENT_BAR_WIDTH - scaledX(16));
    lv_obj_set_height(this->text, LV_SIZE_CONTENT);
    lv_obj_set_align(this->text, LV_ALIGN_LEFT_MID);
    lv_obj_set_style_text_align(this->text, LV_TEXT_ALIGN_LEFT, 0);
    lv_label_set_long_mode(this->text, LV_LABEL_LONG_WRAP);
    lv_label_set_text(this->text, "");

    // ============================================
    // DISMISS BUTTON (X)
    // ============================================
    this->dismissBtn = lv_button_create(this->container);
    lv_obj_remove_style_all(this->dismissBtn);
    lv_obj_set_size(this->dismissBtn, DISMISS_BTN_SIZE, DISMISS_BTN_SIZE);
    lv_obj_set_align(this->dismissBtn, LV_ALIGN_RIGHT_MID);
    lv_obj_set_x(this->dismissBtn, TOAST_PADDING_X - scaledX(4));

    // Button styling - circular with hover effect
    lv_obj_set_style_bg_opa(this->dismissBtn, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(this->dismissBtn, LV_OPA_30, LV_PART_MAIN | (lv_style_selector_t)LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(this->dismissBtn, lv_color_hex(0xFFFFFF), LV_PART_MAIN | (lv_style_selector_t)LV_STATE_PRESSED);
    lv_obj_set_style_radius(this->dismissBtn, DISMISS_BTN_SIZE / 2, LV_PART_MAIN);

    // X label
    lv_obj_t *btnLabel = lv_label_create(this->dismissBtn);
    lv_label_set_text(btnLabel, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_color(btnLabel, lv_color_hex(0x6C7086), LV_PART_MAIN);
    lv_obj_set_align(btnLabel, LV_ALIGN_CENTER);

    lv_obj_add_event_cb(this->dismissBtn, dismiss_btn_cb, LV_EVENT_CLICKED, this);

    // Initially hide toast
    lv_obj_add_flag(this->container, LV_OBJ_FLAG_HIDDEN);

    this->expiry = 0;
}

void Alert::dismiss()
{
    this->dismissed = true;
    // Store in global state so all screens know about the dismissal
    globalAlertDismiss();

    lv_obj_add_flag(this->container, LV_OBJ_FLAG_HIDDEN);
}

void Alert::syncFromGlobal()
{
    // Sync this alert's state from global state when switching screens
    if (globalAlertState.hasActiveAlert)
    {
        // Copy the message and color from global state
        this->lastColor = globalAlertState.currentColor;
        strncpy(this->lastMessage, globalAlertState.currentMessage, sizeof(this->lastMessage) - 1);
        this->lastMessage[sizeof(this->lastMessage) - 1] = '\0';

        // Always hide the toast on screen switch
        this->dismissed = true;
        lv_obj_add_flag(this->container, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        // No active alert
        this->dismissed = false;
        lv_obj_add_flag(this->container, LV_OBJ_FLAG_HIDDEN);
    }
}

void Alert::loop()
{
    if (millis() > expiry && !dismissed)
    {
        if (!lv_obj_has_flag(this->container, LV_OBJ_FLAG_HIDDEN))
        {
            // Auto-dismiss: update global state so other screens know
            this->dismissed = true;
            globalAlertDismiss();

            lv_obj_add_flag(this->container, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void Alert::show(lv_color_t accentColor, char *text, unsigned long expiry)
{
    // Check if this is the same message that's already in global state
    if (globalAlertState.hasActiveAlert && strcmp(globalAlertState.currentMessage, text) == 0)
    {
        // Same message - don't re-show, just maintain current state
        // Update local data
        this->lastColor = accentColor;
        strncpy(this->lastMessage, text, sizeof(this->lastMessage) - 1);
        this->lastMessage[sizeof(this->lastMessage) - 1] = '\0';
        return;
    }

    // New/different message - reset everything and show the toast
    globalAlertSetActive(text, accentColor, this->parentScr);
    showForced(accentColor, text, expiry);
}

void Alert::showForced(lv_color_t accentColor, char *text, unsigned long expiry)
{
    this->expiry = expiry;
    this->dismissed = false;
    // Update global state for this forced show
    globalAlertSetActive(text, accentColor, this->parentScr);
    this->lastColor = accentColor;
    strncpy(this->lastMessage, text, sizeof(this->lastMessage) - 1);
    this->lastMessage[sizeof(this->lastMessage) - 1] = '\0';

    lv_obj_remove_flag(this->container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(this->container);

    // Update accent bar color
    lv_obj_t *accentBar = (lv_obj_t *)lv_obj_get_user_data(this->container);
    if (accentBar != NULL)
    {
        lv_obj_set_style_bg_color(accentBar, accentColor, LV_PART_MAIN);
    }

    // Update text
    lv_label_set_text_fmt(this->text, "%s", text);
}
