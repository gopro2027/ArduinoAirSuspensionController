#include "PWR_Key.h"

// -----------------------------------------------------------------------------
#ifndef PWR_KEY_ACTIVE_LOW
#define PWR_KEY_ACTIVE_LOW 1 // Button pulls the line LOW when pressed
#endif

#ifndef PWR_LATCH_ACTIVE_HIGH
#define PWR_LATCH_ACTIVE_HIGH 1 // Drive HIGH on PWR_Control_PIN to hold power
#endif
// -----------------------------------------------------------------------------

static inline void configure_GPIO(gpio_num_t pin, gpio_mode_t mode)
{
    gpio_reset_pin(pin);
    gpio_set_direction(pin, mode);
}
void power_key_setup(void)
{
        // Key pin (input with appropriate pull)
    configure_GPIO((gpio_num_t)PWR_KEY_Input_PIN, GPIO_MODE_INPUT);
#if PWR_KEY_ACTIVE_LOW
    gpio_pullup_en((gpio_num_t)PWR_KEY_Input_PIN);
    gpio_pulldown_dis((gpio_num_t)PWR_KEY_Input_PIN);
#else
    gpio_pulldown_en((gpio_num_t)PWR_KEY_Input_PIN);
    gpio_pullup_dis((gpio_num_t)PWR_KEY_Input_PIN);
#endif

    // Latch pin (output). Start OFF, then take over if key is held.
    configure_GPIO((gpio_num_t)PWR_Control_PIN, GPIO_MODE_OUTPUT);
}
bool power_key_pressed(void)
{
    int lvl = gpio_get_level((gpio_num_t)PWR_KEY_Input_PIN);
    return PWR_KEY_ACTIVE_LOW ? (lvl == 0) : (lvl != 0);
}

void power_latch_on(void)
{
    gpio_set_level((gpio_num_t)PWR_Control_PIN, PWR_LATCH_ACTIVE_HIGH ? 1 : 0);
}

void power_latch_off(void)
{
    gpio_set_level((gpio_num_t)PWR_Control_PIN, PWR_LATCH_ACTIVE_HIGH ? 0 : 1);
}

// this function is for when the device is in light sleep mode... this simply configures the power button to wake up from light sleep
void power_enable_wakeup_lightsleep()
{
// Configure wake on key (active level depends on your wiring)
#if PWR_KEY_ACTIVE_LOW
    gpio_wakeup_enable((gpio_num_t)PWR_KEY_Input_PIN, GPIO_INTR_LOW_LEVEL);
#else
    gpio_wakeup_enable((gpio_num_t)PWR_KEY_Input_PIN, GPIO_INTR_HIGH_LEVEL);
#endif
    esp_sleep_enable_gpio_wakeup();
}

void power_disable_wakeup_lightsleep()
{
    gpio_wakeup_disable((gpio_num_t)PWR_KEY_Input_PIN);
}

// this function is for when the device is in deep sleep mode but plugged in (aka can't fully shut off since it's getting power from usb)... this configures the power button to wake up from deep sleep, which then by nature does a full reboot (waking up from deep sleep always reboots)
void power_enable_wakeup_deepsleep() {
    uint64_t mask = 1ULL << PWR_KEY_Input_PIN;
#if PWR_KEY_ACTIVE_LOW
    // Idle = HIGH via pull-up; wake when pressed (LOW)
    gpio_pullup_en((gpio_num_t)PWR_KEY_Input_PIN);
    gpio_pulldown_dis((gpio_num_t)PWR_KEY_Input_PIN);
    esp_sleep_enable_ext1_wakeup(mask, ESP_EXT1_WAKEUP_ANY_LOW);
#else
    // Idle = LOW via pull-down; wake when pressed (HIGH)
    gpio_pulldown_en((gpio_num_t)PWR_KEY_Input_PIN);
    gpio_pullup_dis((gpio_num_t)PWR_KEY_Input_PIN);
    esp_sleep_enable_ext1_wakeup(mask, ESP_EXT1_WAKEUP_ANY_HIGH);
#endif 
}