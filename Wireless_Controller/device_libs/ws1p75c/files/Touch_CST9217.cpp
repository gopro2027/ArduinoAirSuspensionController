#include "Touch_CST9217.h"
#include "Display_CO5300.h"
#include "i2c_guard.h"

#include <Wire.h>
#include <touch/TouchDrvCST92xx.h>

// The CST9217 shares GPIO15/14 with the AXP2101 and the QMI8658, and pmic_cache polls the PMIC
// from a task on the other core, so every transaction here takes the shared bus mutex.

static TouchDrvCST92xx touch_driver;
static bool touch_ok = false;

void Touch_Init(void)
{
    // I2C_Init() has already done Wire.begin on these pins; begin() below is idempotent.
    pinMode(CST9217_INT_PIN, INPUT);

    touch_driver.setPins(CST9217_RST_PIN, CST9217_INT_PIN);

    if (!i2c_lock(100)) {
        printf("Touch init skipped: I2C busy\r\n");
        return;
    }
    // Pins omitted on purpose: I2C_Init() already started this bus, and passing them again makes
    // SensorLib re-run Wire.setPins()/begin() and log a bus-already-initialized error.
    touch_ok = touch_driver.begin(Wire, CST9217_ADDR);
    if (touch_ok) {
        // Mirroring is done against these maxima, so they must be the last valid pixel.
        touch_driver.setMaxCoordinates(LCD_WIDTH - 1, LCD_HEIGHT - 1);
        touch_driver.setSwapXY(CST9217_SWAP_XY);
        touch_driver.setMirrorXY(CST9217_MIRROR_X, CST9217_MIRROR_Y);
    }
    i2c_unlock();

    if (touch_ok) {
        printf("Touch controller: %s\r\n", touch_driver.getModelName());
    } else {
        printf("No touch controller detected at 0x%02X!\r\n", CST9217_ADDR);
    }
}

uint8_t getTouch(uint16_t *x, uint16_t *y)
{
    if (!touch_ok)
        return 0;
    // Report "not pressed" rather than stall the LVGL task if the PMIC poller holds the bus.
    if (!i2c_lock(5))
        return 0;

    TouchPoints points = touch_driver.getTouchPoints();
    i2c_unlock();

    if (!points.hasPoints())
        return 0;

    const TouchPoint &p = points.getPoint(0);
    *x = p.x;
    *y = p.y;
    return 1;
}
