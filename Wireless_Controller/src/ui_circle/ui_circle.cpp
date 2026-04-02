#include "device_lib_exports.h"

#ifdef SCREEN_MODE_CIRCLE

#include "ui/ui.h"
#include "waveshare/board_driver_util.h"
#include "utils/touch_lib.h"
#include "ui/components/alert.h"
#include "ui_circle/components/circle_nav.h"
#include "ui_circle/components/circle_statusbar.h"

LV_IMG_DECLARE(oasman_splash);

SCREEN currentScreen = SCREEN_NONE;
static lv_obj_t *mainScreen = nullptr;
static lv_obj_t *tileview = nullptr;
static lv_obj_t *tiles[3] = {nullptr, nullptr, nullptr};

static void syncScreenFromActiveTile()
{
    if (!tileview)
        return;
    lv_obj_t *act = lv_tileview_get_tile_active(tileview);
    int tabIndex = 0;
    for (; tabIndex < 3; tabIndex++) {
        if (tiles[tabIndex] == act)
            break;
    }
    if (tabIndex >= 3)
        return;

    SCREEN newScreen = (SCREEN)(tabIndex + 1);
    if (currentScreen == newScreen)
        return;

    currentScreen = newScreen;
    currentScr = screens[tabIndex];
    circlePageDots.setActive((uint8_t)tabIndex);

    if (currentScr != nullptr && currentScr->alert != nullptr)
        currentScr->alert->syncFromGlobal();

    screenLoop();
}

static void tileview_value_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED)
        syncScreenFromActiveTile();
}

void ui_init(void)
{
    lv_obj_t *tempScr = lv_screen_active();

    lv_display_t *dispp = lv_display_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
                                              true, LV_FONT_DEFAULT);
    lv_display_set_theme(dispp, theme);

    mainScreen = lv_obj_create(nullptr);
    lv_obj_remove_flag(mainScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(mainScreen, lv_color_hex(GENERIC_GREY_VERY_DARK), 0);
    lv_obj_set_style_bg_opa(mainScreen, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(mainScreen, 0, 0);
    lv_obj_set_style_pad_all(mainScreen, 0, 0);

    tileview = lv_tileview_create(mainScreen);
    lv_obj_set_size(tileview, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(tileview, 0, 0);
    lv_obj_set_style_bg_opa(tileview, LV_OPA_TRANSP, 0);
    lv_obj_add_event_cb(tileview, tileview_value_cb, LV_EVENT_VALUE_CHANGED, nullptr);

    tiles[0] = lv_tileview_add_tile(tileview, 0, 0, LV_DIR_HOR);
    tiles[1] = lv_tileview_add_tile(tileview, 1, 0, LV_DIR_HOR);
    tiles[2] = lv_tileview_add_tile(tileview, 2, 0, LV_DIR_HOR);

    scrHome.init(tiles[0]);
    scrPresets.init(tiles[1]);
    scrSettings.init(tiles[2]);

    circlePageDots.create(mainScreen);
    circleStatusbarMini.create(mainScreen);
    if (circlePageDots.container)
        lv_obj_move_foreground(circlePageDots.container);
    if (circleStatusbarMini.container)
        lv_obj_move_foreground(circleStatusbarMini.container);

    lv_screen_load(mainScreen);

    currentScreen = SCREEN_HOME;
    currentScr = &scrHome;
    screens[0] = &scrHome;
    screens[1] = &scrPresets;
    screens[2] = &scrSettings;
    circlePageDots.setActive(0);

    if (tempScr != nullptr && tempScr != mainScreen)
        lv_obj_del(tempScr);
}

void ui_reinit(void)
{
    SCREEN prevScreen = currentScreen;

    set_brightness(0);
    delay(10);

    lv_obj_t *splashScr = applyRotationAndShowSplashScreen();

    currentScreen = SCREEN_NONE;
    currentScr = nullptr;

    scrHome.cleanup();
    scrPresets.cleanup();
    scrSettings.cleanup();

    circleStatusbarMini.cleanup();
    circlePageDots.cleanup();

    if (mainScreen) {
        lv_obj_del(mainScreen);
        mainScreen = nullptr;
        tileview = nullptr;
        tiles[0] = tiles[1] = tiles[2] = nullptr;
    }

    ui_init();

    changeScreen(prevScreen, false);

    lv_obj_del(splashScr);
}

void changeScreen(SCREEN screen, bool animate)
{
    if (currentScreen == screen)
        return;

    uint32_t col;
    switch (screen) {
    case SCREEN_HOME:
        col = 0;
        break;
    case SCREEN_PRESETS:
        col = 1;
        break;
    case SCREEN_SETTINGS:
        col = 2;
        break;
    default:
        return;
    }

    lv_anim_enable_t a = animate ? LV_ANIM_ON : LV_ANIM_OFF;
    lv_tileview_set_tile_by_index(tileview, col, 0, a);
    syncScreenFromActiveTile();
}

void safetyModeMsgBoxCheck()
{
    bool safety_mode = (*util_configValues._configFlagsBits() & (1 << ConfigFlagsBit::CONFIG_SAFETY_MODE)) != 0 ? 1 : 0;
    static bool hasShownSafetyMode = false;
    if (safety_mode && hasShownSafetyMode == false) {
        hasShownSafetyMode = true;
        static char buf[40];
        snprintf(buf, sizeof(buf), "Save current height to preset %i?", currentPreset);
        currentScr->showMsgBox("Safe Boot is ENABLED!", "Safe boot is enabled meaning some features are disabled (your compressor & rise on start). Please check your settings are correct and then disable safe boot. This includes 'Pressure Sensor Rating PSI'. Then double check system functionality before disabling safety mode.", "View Settings", "Disable Anyway",
                               []() -> void {
                                   changeScreen(SCREEN_SETTINGS);
                                   showDialog("Please check your settings!", lv_color_hex(THEME_COLOR_LIGHT));
                               },
                               []() -> void {
                                   Serial.println("Disabling safety mode");
                                   setManifoldConfigValuesFlag(ConfigFlagsBit::CONFIG_SAFETY_MODE, false);
                                   showDialog("Safety mode disabled!", lv_color_hex(0xFF0000));
                               },
                               true);
    }
}

void screenLoop()
{
    circleStatusbarMini.update();

    switch (currentScreen) {
    case SCREEN_HOME:
        scrHome.loop();
        break;
    case SCREEN_PRESETS:
        scrPresets.loop();
        break;
    case SCREEN_SETTINGS:
        scrSettings.loop();
        break;
    default:
        break;
    }

    resetTouchInputFrame();
}

#endif /* SCREEN_MODE_CIRCLE */
