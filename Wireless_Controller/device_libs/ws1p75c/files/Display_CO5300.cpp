#include "Display_CO5300.h"
#include "Touch_CST9217.h"

#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_commands.h"
#include "esp_log.h"

static const char *TAG = "CO5300";

// ─────────────────────────────────────────────────────────────────────────────
// CO5300 panel driver (structure from the SH8601 driver; both are QSPI AMOLED)
// ─────────────────────────────────────────────────────────────────────────────

#define LCD_OPCODE_WRITE_CMD   (0x02ULL)
#define LCD_OPCODE_READ_CMD    (0x03ULL)
#define LCD_OPCODE_WRITE_COLOR (0x32ULL)

/** Brightness is a DCS write, not a backlight pin — this panel has no BK_LIGHT GPIO. */
#define CO5300_CMD_WRITE_DISPLAY_BRIGHTNESS 0x51

typedef struct {
    int cmd;
    const void *data;
    size_t data_bytes;
    unsigned int delay_ms;
} co5300_lcd_init_cmd_t;

typedef struct {
    const co5300_lcd_init_cmd_t *init_cmds;
    uint16_t init_cmds_size;
    struct {
        unsigned int use_qspi_interface: 1;
    } flags;
} co5300_vendor_config_t;

typedef struct {
    esp_lcd_panel_t base;
    esp_lcd_panel_io_handle_t io;
    int reset_gpio_num;
    int x_gap;
    int y_gap;
    uint8_t fb_bits_per_pixel;
    uint8_t madctl_val;
    uint8_t colmod_val;
    const co5300_lcd_init_cmd_t *init_cmds;
    uint16_t init_cmds_size;
    struct {
        unsigned int use_qspi_interface: 1;
        unsigned int reset_level: 1;
    } flags;
} co5300_panel_t;

/** Wrap an 8-bit DCS command in the QSPI frame the panel expects (opcode << 24 | cmd << 8). */
static inline int co5300_qspi_cmd(int lcd_cmd, uint64_t opcode)
{
    lcd_cmd &= 0xff;
    lcd_cmd <<= 8;
    lcd_cmd |= opcode << 24;
    return lcd_cmd;
}

static esp_err_t tx_param(co5300_panel_t *co5300, esp_lcd_panel_io_handle_t io, int lcd_cmd, const void *param, size_t param_size)
{
    if (co5300->flags.use_qspi_interface) {
        lcd_cmd = co5300_qspi_cmd(lcd_cmd, LCD_OPCODE_WRITE_CMD);
    }
    return esp_lcd_panel_io_tx_param(io, lcd_cmd, param, param_size);
}

static esp_err_t tx_color(co5300_panel_t *co5300, esp_lcd_panel_io_handle_t io, int lcd_cmd, const void *param, size_t param_size)
{
    if (co5300->flags.use_qspi_interface) {
        lcd_cmd = co5300_qspi_cmd(lcd_cmd, LCD_OPCODE_WRITE_COLOR);
    }
    return esp_lcd_panel_io_tx_color(io, lcd_cmd, param, param_size);
}

static esp_err_t panel_co5300_del(esp_lcd_panel_t *panel)
{
    co5300_panel_t *co5300 = __containerof(panel, co5300_panel_t, base);
    if (co5300->reset_gpio_num >= 0) {
        gpio_reset_pin((gpio_num_t)co5300->reset_gpio_num);
    }
    free(co5300);
    return ESP_OK;
}

static esp_err_t panel_co5300_reset(esp_lcd_panel_t *panel)
{
    co5300_panel_t *co5300 = __containerof(panel, co5300_panel_t, base);
    esp_lcd_panel_io_handle_t io = co5300->io;

    if (co5300->reset_gpio_num >= 0) {
        gpio_set_level((gpio_num_t)co5300->reset_gpio_num, co5300->flags.reset_level);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level((gpio_num_t)co5300->reset_gpio_num, !co5300->flags.reset_level);
        vTaskDelay(pdMS_TO_TICKS(200));
    } else {
        ESP_RETURN_ON_ERROR(tx_param(co5300, io, LCD_CMD_SWRESET, NULL, 0), TAG, "send command failed");
        vTaskDelay(pdMS_TO_TICKS(80));
    }
    return ESP_OK;
}

static const co5300_lcd_init_cmd_t vendor_specific_init_default[] = {
    {0x35, (uint8_t []){0x00}, 1, 0},
    {0x53, (uint8_t []){0x20}, 1, 25},
};

static esp_err_t panel_co5300_init(esp_lcd_panel_t *panel)
{
    co5300_panel_t *co5300 = __containerof(panel, co5300_panel_t, base);
    esp_lcd_panel_io_handle_t io = co5300->io;
    const co5300_lcd_init_cmd_t *init_cmds = NULL;
    uint16_t init_cmds_size = 0;
    bool is_cmd_overwritten = false;

    ESP_RETURN_ON_ERROR(tx_param(co5300, io, LCD_CMD_MADCTL, (uint8_t[]) {
        co5300->madctl_val,
    }, 1), TAG, "send command failed");
    ESP_RETURN_ON_ERROR(tx_param(co5300, io, LCD_CMD_COLMOD, (uint8_t[]) {
        co5300->colmod_val,
    }, 1), TAG, "send command failed");

    if (co5300->init_cmds) {
        init_cmds = co5300->init_cmds;
        init_cmds_size = co5300->init_cmds_size;
    } else {
        init_cmds = vendor_specific_init_default;
        init_cmds_size = sizeof(vendor_specific_init_default) / sizeof(co5300_lcd_init_cmd_t);
    }

    for (int i = 0; i < init_cmds_size; i++) {
        switch (init_cmds[i].cmd) {
        case LCD_CMD_MADCTL:
            is_cmd_overwritten = true;
            co5300->madctl_val = ((uint8_t *)init_cmds[i].data)[0];
            break;
        case LCD_CMD_COLMOD:
            is_cmd_overwritten = true;
            co5300->colmod_val = ((uint8_t *)init_cmds[i].data)[0];
            break;
        default:
            is_cmd_overwritten = false;
            break;
        }
        if (is_cmd_overwritten) {
            ESP_LOGW(TAG, "The %02Xh command has been used and will be overwritten by external initialization sequence", init_cmds[i].cmd);
        }
        ESP_RETURN_ON_ERROR(tx_param(co5300, io, init_cmds[i].cmd, init_cmds[i].data, init_cmds[i].data_bytes), TAG, "send command failed");
        vTaskDelay(pdMS_TO_TICKS(init_cmds[i].delay_ms));
    }

    return ESP_OK;
}

static esp_err_t panel_co5300_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start, int x_end, int y_end, const void *color_data)
{
    co5300_panel_t *co5300 = __containerof(panel, co5300_panel_t, base);
    esp_lcd_panel_io_handle_t io = co5300->io;

    x_start += co5300->x_gap;
    x_end   += co5300->x_gap;
    y_start += co5300->y_gap;
    y_end   += co5300->y_gap;

    ESP_RETURN_ON_ERROR(tx_param(co5300, io, LCD_CMD_CASET, (uint8_t[]) {
        (uint8_t)((x_start >> 8) & 0xFF), (uint8_t)(x_start & 0xFF),
        (uint8_t)(((x_end - 1) >> 8) & 0xFF), (uint8_t)((x_end - 1) & 0xFF),
    }, 4), TAG, "send command failed");
    ESP_RETURN_ON_ERROR(tx_param(co5300, io, LCD_CMD_RASET, (uint8_t[]) {
        (uint8_t)((y_start >> 8) & 0xFF), (uint8_t)(y_start & 0xFF),
        (uint8_t)(((y_end - 1) >> 8) & 0xFF), (uint8_t)((y_end - 1) & 0xFF),
    }, 4), TAG, "send command failed");

    size_t len = (x_end - x_start) * (y_end - y_start) * co5300->fb_bits_per_pixel / 8;
    tx_color(co5300, io, LCD_CMD_RAMWR, color_data, len);

    return ESP_OK;
}

static esp_err_t panel_co5300_invert_color(esp_lcd_panel_t *panel, bool invert_color_data)
{
    co5300_panel_t *co5300 = __containerof(panel, co5300_panel_t, base);
    esp_lcd_panel_io_handle_t io = co5300->io;
    int command = invert_color_data ? LCD_CMD_INVON : LCD_CMD_INVOFF;
    ESP_RETURN_ON_ERROR(tx_param(co5300, io, command, NULL, 0), TAG, "send command failed");
    return ESP_OK;
}

static esp_err_t panel_co5300_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y)
{
    co5300_panel_t *co5300 = __containerof(panel, co5300_panel_t, base);
    esp_lcd_panel_io_handle_t io = co5300->io;
    if (mirror_x) {
        co5300->madctl_val |= BIT(6);
    } else {
        co5300->madctl_val &= ~BIT(6);
    }
    if (mirror_y) {
        co5300->madctl_val |= BIT(7);
    } else {
        co5300->madctl_val &= ~BIT(7);
    }
    ESP_RETURN_ON_ERROR(tx_param(co5300, io, LCD_CMD_MADCTL, (uint8_t[]) {
        co5300->madctl_val
    }, 1), TAG, "send command failed");
    return ESP_OK;
}

static esp_err_t panel_co5300_swap_xy(esp_lcd_panel_t *panel, bool swap_axes)
{
    ESP_LOGE(TAG, "swap_xy is not supported by this panel");
    return ESP_ERR_NOT_SUPPORTED;
}

static esp_err_t panel_co5300_set_gap(esp_lcd_panel_t *panel, int x_gap, int y_gap)
{
    co5300_panel_t *co5300 = __containerof(panel, co5300_panel_t, base);
    co5300->x_gap = x_gap;
    co5300->y_gap = y_gap;
    return ESP_OK;
}

static esp_err_t panel_co5300_disp_on_off(esp_lcd_panel_t *panel, bool on_off)
{
    co5300_panel_t *co5300 = __containerof(panel, co5300_panel_t, base);
    esp_lcd_panel_io_handle_t io = co5300->io;
    int command = on_off ? LCD_CMD_DISPON : LCD_CMD_DISPOFF;
    ESP_RETURN_ON_ERROR(tx_param(co5300, io, command, NULL, 0), TAG, "send command failed");
    return ESP_OK;
}

static esp_err_t esp_lcd_new_panel_co5300(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel)
{
    ESP_RETURN_ON_FALSE(io && panel_dev_config && ret_panel, ESP_ERR_INVALID_ARG, TAG, "invalid argument");

    co5300_panel_t *co5300 = (co5300_panel_t *)calloc(1, sizeof(co5300_panel_t));
    ESP_RETURN_ON_FALSE(co5300, ESP_ERR_NO_MEM, TAG, "no mem for co5300 panel");

    if (panel_dev_config->reset_gpio_num >= 0) {
        gpio_config_t io_conf = {};
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pin_bit_mask = 1ULL << panel_dev_config->reset_gpio_num;
        gpio_config(&io_conf);
    }

    switch (panel_dev_config->rgb_ele_order) {
    case LCD_RGB_ELEMENT_ORDER_RGB:
        co5300->madctl_val = 0;
        break;
    case LCD_RGB_ELEMENT_ORDER_BGR:
        co5300->madctl_val |= LCD_CMD_BGR_BIT;
        break;
    default:
        free(co5300);
        return ESP_ERR_NOT_SUPPORTED;
    }

    uint8_t fb_bits_per_pixel = 0;
    switch (panel_dev_config->bits_per_pixel) {
    case 16:
        co5300->colmod_val = 0x55;
        fb_bits_per_pixel = 16;
        break;
    case 18:
        co5300->colmod_val = 0x66;
        fb_bits_per_pixel = 18;
        break;
    case 24:
        co5300->colmod_val = 0x77;
        fb_bits_per_pixel = 24;
        break;
    default:
        free(co5300);
        return ESP_ERR_NOT_SUPPORTED;
    }

    co5300->io = io;
    co5300->reset_gpio_num = panel_dev_config->reset_gpio_num;
    co5300->fb_bits_per_pixel = fb_bits_per_pixel;

    co5300_vendor_config_t *vendor_config = (co5300_vendor_config_t *)panel_dev_config->vendor_config;
    if (vendor_config) {
        co5300->init_cmds = vendor_config->init_cmds;
        co5300->init_cmds_size = vendor_config->init_cmds_size;
        co5300->flags.use_qspi_interface = vendor_config->flags.use_qspi_interface;
    }
    co5300->flags.reset_level = panel_dev_config->flags.reset_active_high;

    co5300->base.del = panel_co5300_del;
    co5300->base.reset = panel_co5300_reset;
    co5300->base.init = panel_co5300_init;
    co5300->base.draw_bitmap = panel_co5300_draw_bitmap;
    co5300->base.invert_color = panel_co5300_invert_color;
    co5300->base.set_gap = panel_co5300_set_gap;
    co5300->base.mirror = panel_co5300_mirror;
    co5300->base.swap_xy = panel_co5300_swap_xy;
    co5300->base.disp_on_off = panel_co5300_disp_on_off;
    *ret_panel = &(co5300->base);

    return ESP_OK;
}

// ─────────────────────────────────────────────────────────────────────────────
// Vendor-specific init commands for the 1.75" round AMOLED
// (from the waveshare/esp32_s3_touch_amoled_1_75c BSP lcd_init_cmds)
// ─────────────────────────────────────────────────────────────────────────────

static const co5300_lcd_init_cmd_t lcd_init_cmds[] = {
    {0xFE, (uint8_t[]){0x20}, 1, 0},
    {0x19, (uint8_t[]){0x10}, 1, 0},
    {0x1C, (uint8_t[]){0xA0}, 1, 0},
    {0xFE, (uint8_t[]){0x00}, 1, 0},
    {0xC4, (uint8_t[]){0x80}, 1, 0},
    // ; was: {0x3A, 0x55} -- the driver already sends COLMOD from bits_per_pixel, which only logged a warning
    {0x35, (uint8_t[]){0x00}, 1, 0},
    {0x53, (uint8_t[]){0x20}, 1, 0},
    // Start dark so the splash fades in like every other board; set_brightness() lights it ; was: 0xFF in the BSP
    {0x51, (uint8_t[]){0x00}, 1, 0},
    {0x63, (uint8_t[]){0xFF}, 1, 0},
    {0x2A, (uint8_t[]){0x00, 0x06, 0x01, 0xD7}, 4, 0},
    {0x2B, (uint8_t[]){0x00, 0x00, 0x01, 0xD1}, 4, 600},
    {0x11, (uint8_t[]){0x00}, 0, 600},
    {0x29, (uint8_t[]){0x00}, 0, 0},
};

// ─────────────────────────────────────────────────────────────────────────────
// LCD BSP: QSPI bus init, panel creation, and public API
// ─────────────────────────────────────────────────────────────────────────────

#define LCD_HOST SPI2_HOST

esp_lcd_panel_handle_t panel_handle = NULL;
static esp_lcd_panel_io_handle_t io_handle = NULL;

void LCD_Init(void)
{
    const spi_bus_config_t buscfg = {
        .data0_io_num = EXAMPLE_PIN_NUM_LCD_DATA0,
        .data1_io_num = EXAMPLE_PIN_NUM_LCD_DATA1,
        .sclk_io_num  = EXAMPLE_PIN_NUM_LCD_PCLK,
        .data2_io_num = EXAMPLE_PIN_NUM_LCD_DATA2,
        .data3_io_num = EXAMPLE_PIN_NUM_LCD_DATA3,
        .max_transfer_sz = CO5300_MAX_TRANSFER_SZ,
    };
    ESP_ERROR_CHECK_WITHOUT_ABORT(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    const esp_lcd_panel_io_spi_config_t io_config = {
        .cs_gpio_num = EXAMPLE_PIN_NUM_LCD_CS,
        .dc_gpio_num = -1,
        .spi_mode = 0,
        // Conservative start. The CO5300 is rated well above this; raise only after the panel is confirmed stable.
        .pclk_hz = 40 * 1000 * 1000,
        .trans_queue_depth = 2,
        .on_color_trans_done = NULL,
        .user_ctx = NULL,
        .lcd_cmd_bits = 32,
        .lcd_param_bits = 8,
        .flags = {
            .quad_mode = true,
        },
    };
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

    co5300_vendor_config_t vendor_config = {
        .init_cmds = lcd_init_cmds,
        .init_cmds_size = sizeof(lcd_init_cmds) / sizeof(lcd_init_cmds[0]),
        .flags = {
            .use_qspi_interface = 1,
        },
    };

    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_PIN_NUM_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = LCD_BIT_PER_PIXEL,
        .vendor_config = &vendor_config,
    };
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_lcd_new_panel_co5300(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_lcd_panel_set_gap(panel_handle, CO5300_X_GAP, CO5300_Y_GAP));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_lcd_panel_disp_on_off(panel_handle, true));

    Touch_Init();
}

void LCD_addWindow(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend, uint16_t *color)
{
    // esp_lcd_panel_draw_bitmap uses exclusive end coordinates
    esp_lcd_panel_draw_bitmap(panel_handle, Xstart, Ystart, Xend + 1, Yend + 1, color);
}

// ─────────────────────────────────────────────────────────────────────────────
// Brightness: AMOLED, no backlight pin — DCS 0x51 over the same QSPI command path
// ─────────────────────────────────────────────────────────────────────────────

void Backlight_Init(void)
{
    // Nothing to set up: board_drivers_init() calls this before LCD_Init(), so there is no
    // panel IO to talk to yet. The init table leaves the panel dark until Set_Backlight().
}

void Set_Backlight(uint8_t Light)
{
    if (Light > Backlight_MAX)
        return;
    if (io_handle == NULL)
        return;

    uint8_t duty = (uint8_t)((255u * Light) / Backlight_MAX);
    esp_lcd_panel_io_tx_param(io_handle,
                              co5300_qspi_cmd(CO5300_CMD_WRITE_DISPLAY_BRIGHTNESS, LCD_OPCODE_WRITE_CMD),
                              &duty, 1);
}

void LCD_SetRotation(uint8_t rotation)
{
    (void)rotation;
    // Round display - rotation is not meaningful
}
