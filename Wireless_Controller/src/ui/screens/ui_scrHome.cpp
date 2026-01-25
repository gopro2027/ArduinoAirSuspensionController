#include "ui_scrHome.h"
#include "ui/ui.h" // sketchy backwards import may break in the future

ScrHome scrHome(true, true, NAV_HOME);

// Unified pill button dimensions - calculated dynamically for rotation support
static int PILL_WIDTH = 60;
static int PILL_HEIGHT = 100;
static int PILL_RADIUS = 30;   // Rounded ends
static int ARROW_SIZE = 10;    // Arrow size
static bool LANDSCAPE_MODE = false;  // Track if we're using horizontal layout

// Precomputed arrow points (relative to pill container origin)
static lv_point_precise_t ARROW_POINTS_UP[3];
static lv_point_precise_t ARROW_POINTS_DOWN[3];

// Precomputed divider points (relative to pill container origin)
static lv_point_precise_t DIVIDER_POINTS[2];

// Calculate pill dimensions based on available space
static void calculatePillDimensions() {
    const int screenHeight = getScreenHeight();
    const int pressureAreaHeight = scaledY(55);
    const int navbarHeight = NAVBAR_HEIGHT;
    const int contentHeight = screenHeight - pressureAreaHeight - navbarHeight;

    LANDSCAPE_MODE = isLandscape();

    if (LANDSCAPE_MODE) {
        // Landscape: contentHeight varies by display
        // Use HORIZONTAL pills (wider than tall) with left=up, right=down
        const int totalGap = scaledY(24);  // Scale the vertical gap
        PILL_HEIGHT = (contentHeight - totalGap) / 2;
        PILL_WIDTH = scaledX(100);  // Scale horizontal width
        PILL_RADIUS = PILL_HEIGHT / 2;  // Round ends on short sides
        ARROW_SIZE = scaledX(8);  // Scale arrow size
    } else {
        // Portrait: dimensions scale with display
        PILL_HEIGHT = scaledY(90);
        PILL_WIDTH = scaledX(54);
        PILL_RADIUS = PILL_WIDTH / 2;
        ARROW_SIZE = scaledX(10);
    }

    // Precompute arrow point coordinates now that pill dimensions are known.
    auto fill_arrow_points = [](lv_point_precise_t (&pts)[3], int cx, int cy, int direction) {
        pts[0].x = -ARROW_SIZE + cx;
        pts[0].y = -4 * direction + cy;
        pts[1].x = cx;
        pts[1].y = 5 * direction + cy;
        pts[2].x = ARROW_SIZE + cx;
        pts[2].y = -4 * direction + cy;
    };

    if (LANDSCAPE_MODE) {
        // Landscape: left=UP, right=DOWN
        fill_arrow_points(ARROW_POINTS_UP,   PILL_WIDTH / 4,       PILL_HEIGHT / 2, -1);
        fill_arrow_points(ARROW_POINTS_DOWN, PILL_WIDTH * 3 / 4,   PILL_HEIGHT / 2,  1);
    } else {
        // Portrait: top=UP, bottom=DOWN
        fill_arrow_points(ARROW_POINTS_UP,   PILL_WIDTH / 2,       PILL_HEIGHT / 4,      -1);
        fill_arrow_points(ARROW_POINTS_DOWN, PILL_WIDTH / 2,       PILL_HEIGHT * 3 / 4,   1);
    }

    // Precompute center divider line points now that pill dimensions are known.
    const int dividerMargin = scaledX(8);  // Scale the margin
    if (LANDSCAPE_MODE) {
        // Vertical divider in center
        DIVIDER_POINTS[0].x = PILL_WIDTH / 2;
        DIVIDER_POINTS[0].y = dividerMargin;
        DIVIDER_POINTS[1].x = PILL_WIDTH / 2;
        DIVIDER_POINTS[1].y = PILL_HEIGHT - dividerMargin;
    } else {
        // Horizontal divider in center
        const int horizontalMargin = scaledX(10);
        DIVIDER_POINTS[0].x = horizontalMargin;
        DIVIDER_POINTS[0].y = PILL_HEIGHT / 2;
        DIVIDER_POINTS[1].x = PILL_WIDTH - horizontalMargin;
        DIVIDER_POINTS[1].y = PILL_HEIGHT / 2;
    }
}

// Animation callback to set transform scale
static void anim_scale_cb(void *var, int32_t value) {
    lv_obj_t *obj = (lv_obj_t *)var;
    lv_obj_set_style_transform_scale(obj, value, 0);
}

// Pill button press callback (for PRESSED event)
static void pill_button_pressed_cb(lv_event_t *e) {
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_PRESSED) {
        int valveBit = (int)lv_event_get_user_data(e);
        lv_obj_t *btn = lv_event_get_target_obj(e);
        
        setValveBit(valveBit);
        
        // Get the pill container (parent of the button) to animate the whole pill
        lv_obj_t *container = lv_obj_get_parent(btn);
        
        // Animate shrink: scale from 256 (100%) to 230 (90%)
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, container);
        lv_anim_set_values(&a, 256, 230);
        lv_anim_set_time(&a, 100);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&a, anim_scale_cb);
        lv_anim_start(&a);
    }
}

// Pill button release callback (for RELEASED event)
static void pill_button_released_cb(lv_event_t *e) {
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_RELEASED) {
        int valveBit = (int)lv_event_get_user_data(e);
        lv_obj_t *btn = lv_event_get_target_obj(e);
        
        unsetValveBit(valveBit);
        
        // Get the pill container (parent of the button) to animate the whole pill
        lv_obj_t *container = lv_obj_get_parent(btn);
        
        // Animate pop back: scale from 230 (90%) to 256 (100%)
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, container);
        lv_anim_set_values(&a, 230, 256);
        lv_anim_set_time(&a, 150);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&a, anim_scale_cb);
        lv_anim_start(&a);
    }
}

// Draw arrow using precomputed points
static void draw_arrow_at(lv_obj_t *parent, lv_point_precise_t *line_points)
{
    // Create arrow line with current theme color (no static style to avoid stale colors)
    lv_obj_t *line = lv_line_create(parent);
    lv_line_set_points(line, line_points, 3);
    lv_obj_set_style_line_width(line, 3, 0);
    lv_obj_set_style_line_color(line, lv_color_hex(THEME_COLOR_LIGHT), 0);
    lv_obj_set_style_line_rounded(line, false, 0);
}


// Structure to hold both up and down buttons for a pill
struct PillButtons {
    lv_obj_t *container;  // Container to hold both buttons and visual elements
    lv_obj_t *btnUp;       // Up button (top in portrait, left in landscape)
    lv_obj_t *btnDown;     // Down button (bottom in portrait, right in landscape)
};

// Create a unified pill button with up/down arrows using standard LVGL buttons
// In portrait: vertical pill, top=up, bottom=down
// In landscape: horizontal pill, left=up, right=down (for larger touch targets)
static PillButtons createUnifiedPill(lv_obj_t *parent)
{
    PillButtons pill;
    
    // Container to hold both buttons and visual elements
    pill.container = lv_obj_create(parent);
    lv_obj_remove_style_all(pill.container);
    lv_obj_set_size(pill.container, PILL_WIDTH, PILL_HEIGHT);
    lv_obj_set_style_bg_color(pill.container, lv_color_hex(GENERIC_GREY_VERY_DARK), 0);
    lv_obj_set_style_bg_opa(pill.container, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(pill.container, PILL_RADIUS, 0);
    lv_obj_set_style_transform_scale(pill.container, 256, 0);  // Initialize to 100% scale
    lv_obj_set_style_transform_pivot_x(pill.container, PILL_WIDTH / 2, 0);  // Center pivot X
    lv_obj_set_style_transform_pivot_y(pill.container, PILL_HEIGHT / 2, 0);  // Center pivot Y
    lv_obj_remove_flag(pill.container, LV_OBJ_FLAG_SCROLLABLE);

    if (LANDSCAPE_MODE) {
        // Landscape: horizontal split (left/right)
        // Left half = UP button
        pill.btnUp = lv_btn_create(pill.container);
        lv_obj_remove_style_all(pill.btnUp);
        lv_obj_set_size(pill.btnUp, PILL_WIDTH / 2, PILL_HEIGHT);
        lv_obj_set_pos(pill.btnUp, 0, 0);
        lv_obj_set_style_bg_opa(pill.btnUp, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(pill.btnUp, LV_OPA_50, LV_PART_MAIN | (lv_style_selector_t)LV_STATE_PRESSED);
        lv_obj_set_style_bg_color(pill.btnUp, lv_color_hex(THEME_COLOR_MEDIUM), LV_PART_MAIN | (lv_style_selector_t)LV_STATE_PRESSED);
        lv_obj_set_style_radius(pill.btnUp, PILL_RADIUS, LV_PART_MAIN);
        lv_obj_remove_flag(pill.btnUp, LV_OBJ_FLAG_SCROLLABLE);

        // Right half = DOWN button
        pill.btnDown = lv_btn_create(pill.container);
        lv_obj_remove_style_all(pill.btnDown);
        lv_obj_set_size(pill.btnDown, PILL_WIDTH / 2, PILL_HEIGHT);
        lv_obj_set_pos(pill.btnDown, PILL_WIDTH / 2, 0);
        lv_obj_set_style_bg_opa(pill.btnDown, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(pill.btnDown, LV_OPA_50, LV_PART_MAIN | (lv_style_selector_t)LV_STATE_PRESSED);
        lv_obj_set_style_bg_color(pill.btnDown, lv_color_hex(THEME_COLOR_MEDIUM), LV_PART_MAIN | (lv_style_selector_t)LV_STATE_PRESSED);
        lv_obj_set_style_radius(pill.btnDown, PILL_RADIUS, LV_PART_MAIN);
        lv_obj_remove_flag(pill.btnDown, LV_OBJ_FLAG_SCROLLABLE);
    } else {
        // Portrait: vertical split (top/bottom)
        // Top half = UP button
        pill.btnUp = lv_btn_create(pill.container);
        lv_obj_remove_style_all(pill.btnUp);
        lv_obj_set_size(pill.btnUp, PILL_WIDTH, PILL_HEIGHT / 2);
        lv_obj_set_pos(pill.btnUp, 0, 0);
        lv_obj_set_style_bg_opa(pill.btnUp, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(pill.btnUp, LV_OPA_50, LV_PART_MAIN | (lv_style_selector_t)LV_STATE_PRESSED);
        lv_obj_set_style_bg_color(pill.btnUp, lv_color_hex(THEME_COLOR_MEDIUM), LV_PART_MAIN | (lv_style_selector_t)LV_STATE_PRESSED);
        lv_obj_set_style_radius(pill.btnUp, PILL_RADIUS, LV_PART_MAIN);
        lv_obj_remove_flag(pill.btnUp, LV_OBJ_FLAG_SCROLLABLE);

        // Bottom half = DOWN button
        pill.btnDown = lv_btn_create(pill.container);
        lv_obj_remove_style_all(pill.btnDown);
        lv_obj_set_size(pill.btnDown, PILL_WIDTH, PILL_HEIGHT / 2);
        lv_obj_set_pos(pill.btnDown, 0, PILL_HEIGHT / 2);
        lv_obj_set_style_bg_opa(pill.btnDown, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(pill.btnDown, LV_OPA_50, LV_PART_MAIN | (lv_style_selector_t)LV_STATE_PRESSED);
        lv_obj_set_style_bg_color(pill.btnDown, lv_color_hex(THEME_COLOR_MEDIUM), LV_PART_MAIN | (lv_style_selector_t)LV_STATE_PRESSED);
        lv_obj_set_style_radius(pill.btnDown, PILL_RADIUS, LV_PART_MAIN);
        lv_obj_remove_flag(pill.btnDown, LV_OBJ_FLAG_SCROLLABLE);
    }

    // Create divider line with current theme color
    lv_obj_t *divider = lv_line_create(pill.container);
    lv_line_set_points(divider, DIVIDER_POINTS, 2);
    lv_obj_set_style_line_width(divider, 1, 0);
    lv_obj_set_style_line_color(divider, lv_color_hex(THEME_COLOR_DARK), 0);

    // Draw arrows
    draw_arrow_at(pill.container, ARROW_POINTS_UP);
    draw_arrow_at(pill.container, ARROW_POINTS_DOWN);

    return pill;
}

// Setup pill button callbacks with valve bit data
static void setupPillButtonCallbacks(PillButtons &pill,
                                     int valveBitsUp[], int valveBitsUpCount,
                                     int valveBitsDown[], int valveBitsDownCount) {
        
    // Add event callbacks - LVGL handles animations automatically
    for (int i = 0; i < valveBitsUpCount; i++) {
        lv_obj_add_event_cb(pill.btnUp, pill_button_pressed_cb, LV_EVENT_PRESSED, (void*)valveBitsUp[i]);
        lv_obj_add_event_cb(pill.btnUp, pill_button_released_cb, LV_EVENT_RELEASED, (void*)valveBitsUp[i]);
    }
    for (int i = 0; i < valveBitsDownCount; i++) {
        lv_obj_add_event_cb(pill.btnDown, pill_button_pressed_cb, LV_EVENT_PRESSED, (void*)valveBitsDown[i]);
        lv_obj_add_event_cb(pill.btnDown, pill_button_released_cb, LV_EVENT_RELEASED, (void*)valveBitsDown[i]);
    }
}

void ScrHome::init(void)
{
    Scr::init();

    // Calculate pill dimensions based on current screen orientation
    calculatePillDimensions();

    // Calculate available content area (between pressure labels and navbar)
    const int pressureAreaHeight = scaledY(55);  // Space for pressure labels at top
    const int navbarHeight = NAVBAR_HEIGHT;
    const int screenWidth = getScreenWidth();
    const int screenHeight = getScreenHeight();
    const int contentHeight = screenHeight - pressureAreaHeight - navbarHeight;

    // Main content container - centers the grid
    lv_obj_t *content = lv_obj_create(this->scr);
    lv_obj_remove_style_all(content);
    lv_obj_set_size(content, screenWidth, contentHeight);
    lv_obj_set_pos(content, 0, pressureAreaHeight);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(content, 4, 0);
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    // Row 0 (Front) - 3 unified pills
    lv_obj_t *row0 = lv_obj_create(content);
    lv_obj_remove_style_all(row0);
    lv_obj_set_size(row0, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row0, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row0, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(row0, LV_OBJ_FLAG_SCROLLABLE);

    PillButtons pillFrontDriver = createUnifiedPill(row0);
    this->pillFrontDriver = pillFrontDriver.container;
    int frontDriverUp[] = {FRONT_DRIVER_IN};
    int frontDriverDown[] = {FRONT_DRIVER_OUT};
    setupPillButtonCallbacks(pillFrontDriver, frontDriverUp, 1, frontDriverDown, 1);
    
    PillButtons pillFrontAxle = createUnifiedPill(row0);
    this->pillFrontAxle = pillFrontAxle.container;
    int frontAxleUp[] = {FRONT_DRIVER_IN, FRONT_PASSENGER_IN};
    int frontAxleDown[] = {FRONT_DRIVER_OUT, FRONT_PASSENGER_OUT};
    setupPillButtonCallbacks(pillFrontAxle, frontAxleUp, 2, frontAxleDown, 2);
    
    PillButtons pillFrontPassenger = createUnifiedPill(row0);
    this->pillFrontPassenger = pillFrontPassenger.container;
    int frontPassengerUp[] = {FRONT_PASSENGER_IN};
    int frontPassengerDown[] = {FRONT_PASSENGER_OUT};
    setupPillButtonCallbacks(pillFrontPassenger, frontPassengerUp, 1, frontPassengerDown, 1);

    // Row 1 (Rear) - 3 unified pills
    lv_obj_t *row1 = lv_obj_create(content);
    lv_obj_remove_style_all(row1);
    lv_obj_set_size(row1, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row1, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row1, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(row1, LV_OBJ_FLAG_SCROLLABLE);

    PillButtons pillRearDriver = createUnifiedPill(row1);
    this->pillRearDriver = pillRearDriver.container;
    int rearDriverUp[] = {REAR_DRIVER_IN};
    int rearDriverDown[] = {REAR_DRIVER_OUT};
    setupPillButtonCallbacks(pillRearDriver, rearDriverUp, 1, rearDriverDown, 1);
    
    PillButtons pillRearAxle = createUnifiedPill(row1);
    this->pillRearAxle = pillRearAxle.container;
    int rearAxleUp[] = {REAR_DRIVER_IN, REAR_PASSENGER_IN};
    int rearAxleDown[] = {REAR_DRIVER_OUT, REAR_PASSENGER_OUT};
    setupPillButtonCallbacks(pillRearAxle, rearAxleUp, 2, rearAxleDown, 2);
    
    PillButtons pillRearPassenger = createUnifiedPill(row1);
    this->pillRearPassenger = pillRearPassenger.container;
    int rearPassengerUp[] = {REAR_PASSENGER_IN};
    int rearPassengerDown[] = {REAR_PASSENGER_OUT};
    setupPillButtonCallbacks(pillRearPassenger, rearPassengerUp, 1, rearPassengerDown, 1);

    // Bring overlays to foreground
    if (this->navbar_container) lv_obj_move_foreground(this->navbar_container);
    lv_obj_move_foreground(this->ui_lblPressureFrontPassenger);
    lv_obj_move_foreground(this->ui_lblPressureRearPassenger);
    lv_obj_move_foreground(this->ui_lblPressureFrontDriver);
    lv_obj_move_foreground(this->ui_lblPressureRearDriver);
    lv_obj_move_foreground(this->ui_lblPressureTank);
}


void ScrHome::loop()
{
    Scr::loop();
}

void ScrHome::cleanup()
{
    Scr::cleanup();  // Base cleanup (Alert)
}
