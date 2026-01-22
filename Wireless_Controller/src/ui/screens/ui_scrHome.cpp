#include "ui_scrHome.h"
#include "ui/ui.h" // sketchy backwards import may break in the future
#include "../theme_colors.h"

LV_IMG_DECLARE(navbar_home);

ScrHome scrHome(navbar_home, true, true, NAV_HOME);

// Unified pill button dimensions - calculated dynamically for rotation support
static int PILL_WIDTH = 60;
static int PILL_HEIGHT = 100;
static int PILL_RADIUS = 30;   // Rounded ends
static int ARROW_SIZE = 10;    // Arrow size
static bool LANDSCAPE_MODE = false;  // Track if we're using horizontal layout

// Calculate pill dimensions based on available space
static void calculatePillDimensions() {
    const int screenHeight = getScreenHeight();
    const int screenWidth = getScreenWidth();
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
}

// Pill structure to hold references to highlight overlays
struct PillRefs {
    lv_obj_t *pill;
    lv_obj_t *highlightTop;
    lv_obj_t *highlightBottom;
};
static PillRefs pillRefs[6];  // 6 pills total
static int pillRefCount = 0;

// Draw arrow at specific position in pill
static void draw_arrow_at(lv_obj_t *parent, int cx, int cy, int direction)
{
    lv_point_precise_t *line_points = new lv_point_precise_t[3];
    line_points[0].x = -ARROW_SIZE + cx;
    line_points[0].y = -4 * direction + cy;
    line_points[1].x = cx;
    line_points[1].y = 5 * direction + cy;
    line_points[2].x = ARROW_SIZE + cx;
    line_points[2].y = -4 * direction + cy;

    // Create arrow line with current theme color (no static style to avoid stale colors)
    lv_obj_t *line = lv_line_create(parent);
    lv_line_set_points(line, line_points, 3);
    lv_obj_set_style_line_width(line, 3, 0);
    lv_obj_set_style_line_color(line, lv_color_hex(THEME_COLOR_LIGHT), 0);
    lv_obj_set_style_line_rounded(line, true, 0);
}

// Find pill refs by pill object
static PillRefs* findPillRefs(lv_obj_t *pill) {
    for (int i = 0; i < pillRefCount; i++) {
        if (pillRefs[i].pill == pill) return &pillRefs[i];
    }
    return NULL;
}

// Animate pill press - highlight only top or bottom half
static void animatePillPress(lv_obj_t *pill, bool pressed, bool isUpHalf)
{
    PillRefs *refs = findPillRefs(pill);
    if (!refs) return;

    // Scale animation on whole pill
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, pill);
    lv_anim_set_time(&a, pressed ? 80 : 150);

    int startScale = pressed ? 256 : 240;
    int endScale = pressed ? 240 : 256;

    lv_anim_set_values(&a, startScale, endScale);
    lv_anim_set_path_cb(&a, pressed ? lv_anim_path_ease_out : lv_anim_path_ease_in_out);
    lv_anim_set_exec_cb(&a, [](void* obj, int32_t v) {
        lv_obj_set_style_transform_scale((lv_obj_t*)obj, v, 0);
    });
    lv_anim_start(&a);

    // Show/hide the appropriate highlight overlay
    if (pressed) {
        if (isUpHalf) {
            lv_obj_remove_flag(refs->highlightTop, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(refs->highlightBottom, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(refs->highlightTop, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(refs->highlightBottom, LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        // Hide both highlights on release
        lv_obj_add_flag(refs->highlightTop, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(refs->highlightBottom, LV_OBJ_FLAG_HIDDEN);
    }
}

// Create a unified pill button with up/down arrows and highlight overlays
// In portrait: vertical pill, top=up, bottom=down
// In landscape: horizontal pill, left=up, right=down (for larger touch targets)
static lv_obj_t* createUnifiedPill(lv_obj_t *parent)
{
    // Single pill-shaped button
    lv_obj_t *pill = lv_obj_create(parent);
    lv_obj_remove_style_all(pill);
    lv_obj_set_size(pill, PILL_WIDTH, PILL_HEIGHT);
    lv_obj_set_style_bg_color(pill, lv_color_hex(GENERIC_GREY_VERY_DARK), 0);
    lv_obj_set_style_bg_opa(pill, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(pill, PILL_RADIUS, 0);
    lv_obj_set_style_transform_pivot_x(pill, PILL_WIDTH / 2, 0);
    lv_obj_set_style_transform_pivot_y(pill, PILL_HEIGHT / 2, 0);
    lv_obj_remove_flag(pill, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *highlightTop;  // "Up" highlight (top in portrait, left in landscape)
    lv_obj_t *highlightBottom;  // "Down" highlight (bottom in portrait, right in landscape)

    if (LANDSCAPE_MODE) {
        // Landscape: horizontal split (left/right)
        // Left half = UP
        highlightTop = lv_obj_create(pill);
        lv_obj_remove_style_all(highlightTop);
        lv_obj_set_size(highlightTop, PILL_WIDTH / 2, PILL_HEIGHT);
        lv_obj_set_pos(highlightTop, 0, 0);
        lv_obj_set_style_bg_color(highlightTop, lv_color_hex(THEME_COLOR_MEDIUM), 0);
        lv_obj_set_style_bg_opa(highlightTop, LV_OPA_50, 0);  // Semi-transparent for better effect
        lv_obj_set_style_radius(highlightTop, PILL_RADIUS, LV_PART_MAIN);  // Rounded to match pill
        lv_obj_set_style_border_width(highlightTop, 0, 0);
        lv_obj_remove_flag(highlightTop, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(highlightTop, LV_OBJ_FLAG_HIDDEN);

        // Right half = DOWN
        highlightBottom = lv_obj_create(pill);
        lv_obj_remove_style_all(highlightBottom);
        lv_obj_set_size(highlightBottom, PILL_WIDTH / 2, PILL_HEIGHT);
        lv_obj_set_pos(highlightBottom, PILL_WIDTH / 2, 0);
        lv_obj_set_style_bg_color(highlightBottom, lv_color_hex(THEME_COLOR_MEDIUM), 0);
        lv_obj_set_style_bg_opa(highlightBottom, LV_OPA_50, 0);  // Semi-transparent for better effect
        lv_obj_set_style_radius(highlightBottom, PILL_RADIUS, LV_PART_MAIN);  // Rounded to match pill
        lv_obj_set_style_border_width(highlightBottom, 0, 0);
        lv_obj_remove_flag(highlightBottom, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(highlightBottom, LV_OBJ_FLAG_HIDDEN);
    } else {
        // Portrait: vertical split (top/bottom)
        highlightTop = lv_obj_create(pill);
        lv_obj_remove_style_all(highlightTop);
        lv_obj_set_size(highlightTop, PILL_WIDTH, PILL_HEIGHT / 2);
        lv_obj_set_pos(highlightTop, 0, 0);
        lv_obj_set_style_bg_color(highlightTop, lv_color_hex(THEME_COLOR_MEDIUM), 0);
        lv_obj_set_style_bg_opa(highlightTop, LV_OPA_50, 0);  // Semi-transparent for better effect
        lv_obj_set_style_radius(highlightTop, PILL_RADIUS, LV_PART_MAIN);  // Rounded to match pill
        lv_obj_set_style_border_width(highlightTop, 0, 0);
        lv_obj_remove_flag(highlightTop, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(highlightTop, LV_OBJ_FLAG_HIDDEN);

        highlightBottom = lv_obj_create(pill);
        lv_obj_remove_style_all(highlightBottom);
        lv_obj_set_size(highlightBottom, PILL_WIDTH, PILL_HEIGHT / 2);
        lv_obj_set_pos(highlightBottom, 0, PILL_HEIGHT / 2);
        lv_obj_set_style_bg_color(highlightBottom, lv_color_hex(THEME_COLOR_MEDIUM), 0);
        lv_obj_set_style_bg_opa(highlightBottom, LV_OPA_50, 0);  // Semi-transparent for better effect
        lv_obj_set_style_radius(highlightBottom, PILL_RADIUS, LV_PART_MAIN);  // Rounded to match pill
        lv_obj_set_style_border_width(highlightBottom, 0, 0);
        lv_obj_remove_flag(highlightBottom, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(highlightBottom, LV_OBJ_FLAG_HIDDEN);
    }

    // Store references
    if (pillRefCount < 6) {
        pillRefs[pillRefCount].pill = pill;
        pillRefs[pillRefCount].highlightTop = highlightTop;
        pillRefs[pillRefCount].highlightBottom = highlightBottom;
        pillRefCount++;
    }

    // Draw center divider line
    lv_point_precise_t *divider_points = new lv_point_precise_t[2];
    const int dividerMargin = scaledX(8);  // Scale the margin
    if (LANDSCAPE_MODE) {
        // Vertical divider in center
        divider_points[0].x = PILL_WIDTH / 2;
        divider_points[0].y = dividerMargin;
        divider_points[1].x = PILL_WIDTH / 2;
        divider_points[1].y = PILL_HEIGHT - dividerMargin;
    } else {
        // Horizontal divider in center
        const int horizontalMargin = scaledX(10);
        divider_points[0].x = horizontalMargin;
        divider_points[0].y = PILL_HEIGHT / 2;
        divider_points[1].x = PILL_WIDTH - horizontalMargin;
        divider_points[1].y = PILL_HEIGHT / 2;
    }

    // Create divider line with current theme color (no static style to avoid stale colors)
    lv_obj_t *divider = lv_line_create(pill);
    lv_line_set_points(divider, divider_points, 2);
    lv_obj_set_style_line_width(divider, 1, 0);
    lv_obj_set_style_line_color(divider, lv_color_hex(THEME_COLOR_DARK), 0);

    // Draw arrows
    if (LANDSCAPE_MODE) {
        // Left side = UP arrow, Right side = DOWN arrow
        draw_arrow_at(pill, PILL_WIDTH / 4, PILL_HEIGHT / 2, -1);
        draw_arrow_at(pill, PILL_WIDTH * 3 / 4, PILL_HEIGHT / 2, 1);
    } else {
        // Top = UP arrow, Bottom = DOWN arrow
        draw_arrow_at(pill, PILL_WIDTH / 2, PILL_HEIGHT / 4, -1);
        draw_arrow_at(pill, PILL_WIDTH / 2, PILL_HEIGHT * 3 / 4, 1);
    }

    return pill;
}

void ScrHome::init(void)
{
    Scr::init();

    this->pressedPill = NULL;
    this->pressedIsUp = false;
    pillRefCount = 0;  // Reset pill references

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

    this->pillFrontDriver = createUnifiedPill(row0);
    this->pillFrontAxle = createUnifiedPill(row0);
    this->pillFrontPassenger = createUnifiedPill(row0);

    // Row 1 (Rear) - 3 unified pills
    lv_obj_t *row1 = lv_obj_create(content);
    lv_obj_remove_style_all(row1);
    lv_obj_set_size(row1, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row1, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row1, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(row1, LV_OBJ_FLAG_SCROLLABLE);

    this->pillRearDriver = createUnifiedPill(row1);
    this->pillRearAxle = createUnifiedPill(row1);
    this->pillRearPassenger = createUnifiedPill(row1);

    // Bring overlays to foreground
    if (this->navbar_container) lv_obj_move_foreground(this->navbar_container);
    lv_obj_move_foreground(this->ui_lblPressureFrontPassenger);
    lv_obj_move_foreground(this->ui_lblPressureRearPassenger);
    lv_obj_move_foreground(this->ui_lblPressureFrontDriver);
    lv_obj_move_foreground(this->ui_lblPressureRearDriver);
    lv_obj_move_foreground(this->ui_lblPressureTank);
}

// down = true when just pressed, false when just released
void ScrHome::runTouchInput(SimplePoint pos, bool down)
{
    Scr::runTouchInput(pos, down);

    if (down == false)
    {
        closeValves();
        // Release animation for pressed pill
        if (this->pressedPill) {
            animatePillPress(this->pressedPill, false, this->pressedIsUp);
            this->pressedPill = NULL;
        }
    }
    else
    {
        // driver side (using dynamic touch areas for rotation support)
        bool _FRONT_DRIVER_IN = cr_contains(get_ctr_row0col0up(), pos);
        bool _FRONT_DRIVER_OUT = cr_contains(get_ctr_row0col0down(), pos);

        bool _REAR_DRIVER_IN = cr_contains(get_ctr_row1col0up(), pos);
        bool _REAR_DRIVER_OUT = cr_contains(get_ctr_row1col0down(), pos);

        // passenger side
        bool _FRONT_PASSENGER_IN = cr_contains(get_ctr_row0col2up(), pos);
        bool _FRONT_PASSENGER_OUT = cr_contains(get_ctr_row0col2down(), pos);

        bool _REAR_PASSENGER_IN = cr_contains(get_ctr_row1col2up(), pos);
        bool _REAR_PASSENGER_OUT = cr_contains(get_ctr_row1col2down(), pos);

        // axles
        bool _FRONT_AXLE_IN = cr_contains(get_ctr_row0col1up(), pos);
        bool _FRONT_AXLE_OUT = cr_contains(get_ctr_row0col1down(), pos);

        bool _REAR_AXLE_IN = cr_contains(get_ctr_row1col1up(), pos);
        bool _REAR_AXLE_OUT = cr_contains(get_ctr_row1col1down(), pos);

        // driver side
        if (_FRONT_DRIVER_IN)
        {
            setValveBit(FRONT_DRIVER_IN);
            animatePillPress(this->pillFrontDriver, true, true);
            this->pressedPill = this->pillFrontDriver;
            this->pressedIsUp = true;
        }

        if (_FRONT_DRIVER_OUT)
        {
            setValveBit(FRONT_DRIVER_OUT);
            animatePillPress(this->pillFrontDriver, true, false);
            this->pressedPill = this->pillFrontDriver;
            this->pressedIsUp = false;
        }

        if (_REAR_DRIVER_IN)
        {
            setValveBit(REAR_DRIVER_IN);
            animatePillPress(this->pillRearDriver, true, true);
            this->pressedPill = this->pillRearDriver;
            this->pressedIsUp = true;
        }

        if (_REAR_DRIVER_OUT)
        {
            setValveBit(REAR_DRIVER_OUT);
            animatePillPress(this->pillRearDriver, true, false);
            this->pressedPill = this->pillRearDriver;
            this->pressedIsUp = false;
        }

        // passenger side
        if (_FRONT_PASSENGER_IN)
        {
            setValveBit(FRONT_PASSENGER_IN);
            animatePillPress(this->pillFrontPassenger, true, true);
            this->pressedPill = this->pillFrontPassenger;
            this->pressedIsUp = true;
        }

        if (_FRONT_PASSENGER_OUT)
        {
            setValveBit(FRONT_PASSENGER_OUT);
            animatePillPress(this->pillFrontPassenger, true, false);
            this->pressedPill = this->pillFrontPassenger;
            this->pressedIsUp = false;
        }

        if (_REAR_PASSENGER_IN)
        {
            setValveBit(REAR_PASSENGER_IN);
            animatePillPress(this->pillRearPassenger, true, true);
            this->pressedPill = this->pillRearPassenger;
            this->pressedIsUp = true;
        }

        if (_REAR_PASSENGER_OUT)
        {
            setValveBit(REAR_PASSENGER_OUT);
            animatePillPress(this->pillRearPassenger, true, false);
            this->pressedPill = this->pillRearPassenger;
            this->pressedIsUp = false;
        }

        // axles
        if (_FRONT_AXLE_IN)
        {
            setValveBit(FRONT_DRIVER_IN);
            setValveBit(FRONT_PASSENGER_IN);
            animatePillPress(this->pillFrontAxle, true, true);
            this->pressedPill = this->pillFrontAxle;
            this->pressedIsUp = true;
        }

        if (_FRONT_AXLE_OUT)
        {
            setValveBit(FRONT_DRIVER_OUT);
            setValveBit(FRONT_PASSENGER_OUT);
            animatePillPress(this->pillFrontAxle, true, false);
            this->pressedPill = this->pillFrontAxle;
            this->pressedIsUp = false;
        }

        if (_REAR_AXLE_IN)
        {
            setValveBit(REAR_DRIVER_IN);
            setValveBit(REAR_PASSENGER_IN);
            animatePillPress(this->pillRearAxle, true, true);
            this->pressedPill = this->pillRearAxle;
            this->pressedIsUp = true;
        }

        if (_REAR_AXLE_OUT)
        {
            setValveBit(REAR_DRIVER_OUT);
            setValveBit(REAR_PASSENGER_OUT);
            animatePillPress(this->pillRearAxle, true, false);
            this->pressedPill = this->pillRearAxle;
            this->pressedIsUp = false;
        }
    }
}

void ScrHome::loop()
{
    Scr::loop();
}
