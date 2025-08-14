#if defined(WAVESHARE_BOARD)

// PWR_Key.c
#include "PWR_Key.h"
#include <stdbool.h>
#include <stdint.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_sleep.h"
#include <esp32_smartdisplay.h>

// -----------------------------------------------------------------------------
#ifndef PWR_KEY_ACTIVE_LOW
#define PWR_KEY_ACTIVE_LOW 1 // Button pulls the line LOW when pressed
#endif

#ifndef PWR_LATCH_ACTIVE_HIGH
#define PWR_LATCH_ACTIVE_HIGH 1 // Drive HIGH on PWR_Control_PIN to hold power
#endif
// -----------------------------------------------------------------------------

// Internal state
static uint8_t Booted_From_State = 0; // 0:no power, 1:key held at boot, 2:booted from usb plugged in
static uint8_t Device_State = 0;      // 0:none, 1:sleep, 2:shutdown
static uint16_t Long_Press = 0;

static inline bool key_pressed(void)
{
    int lvl = gpio_get_level((gpio_num_t)PWR_KEY_Input_PIN);
    return PWR_KEY_ACTIVE_LOW ? (lvl == 0) : (lvl != 0);
}

static inline void latch_on(void)
{
    gpio_set_level((gpio_num_t)PWR_Control_PIN, PWR_LATCH_ACTIVE_HIGH ? 1 : 0);
}

static inline void latch_off(void)
{
    gpio_set_level((gpio_num_t)PWR_Control_PIN, PWR_LATCH_ACTIVE_HIGH ? 0 : 1);
}

static inline void configure_GPIO(gpio_num_t pin, gpio_mode_t mode)
{
    gpio_reset_pin(pin);
    gpio_set_direction(pin, mode);
}
auto woke_up = true;
void PWR_Init(void)
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
    latch_off();
    vTaskDelay(pdMS_TO_TICKS(10));

    if (key_pressed())
    {
        Booted_From_State = 1; // booted by holding the key
        latch_on();            // keep board on
    }
    else
    {
        Booted_From_State = 2; // booted without key (e.g., USB)
        // latch_on();    // we can uncomment this if we want the device to stay on when the usb is unplugged. Personally I prefer the device turn off on it's own due to the nature of our system being in a car. The device may turn on when the car is turned on, then turn off when the car is turned off.
    }
    woke_up = true;
}

void PWR_Loop(void)
{

    if (key_pressed())
    {
        if (!woke_up)
        {
            if (Long_Press < 0xFFFF)
                Long_Press++;

            if (Long_Press < Device_Shutdown_Time)
            {
                Device_State = 1; // go into sleep
            }
            else
            {
                // tell it to shutdown once button is released
                Device_State = 2;
                smartdisplay_lcd_set_backlight(0); // this is so the user knows the device is about to turn off so they can let go of the button. Once they let go, device state being 2 makes the device shut down. The reason being, we want to make sure the user is not actively pressing the button upon shutdown time to ensure that is is not woken back up immediately after shutdown accidentally
            }
        }
    }
    else
    {
        woke_up = false;

        if (Device_State == 1)
            Fall_Asleep();
        else if (Device_State == 2)
            Shutdown();

        Device_State = 0;
        Long_Press = 0;
    }
}

void enableWakeup()
{
// Configure wake on key (active level depends on your wiring)
#if PWR_KEY_ACTIVE_LOW
    gpio_wakeup_enable((gpio_num_t)PWR_KEY_Input_PIN, GPIO_INTR_LOW_LEVEL);
#else
    gpio_wakeup_enable((gpio_num_t)PWR_KEY_Input_PIN, GPIO_INTR_HIGH_LEVEL);
#endif
    esp_sleep_enable_gpio_wakeup();
}

void onWakeup()
{
    log_i("Waking up");
    gpio_wakeup_disable((gpio_num_t)PWR_KEY_Input_PIN);
    woke_up = true;

    smartdisplay_lcd_set_backlight(0.8f);
}

void Fall_Asleep(void)
{
    log_i("Falling asleep");
    smartdisplay_lcd_set_backlight(0);
    vTaskDelay(pdMS_TO_TICKS(50)); // give time for backlight to turn off before sleeping

    enableWakeup();
    // Enter light sleep (returns on wake)
    esp_light_sleep_start();
    onWakeup();
}

void Shutdown(void)
{
    log_i("Shutting down");
    // Turn off UI/backlight, drop the power latch
    smartdisplay_lcd_set_backlight(0);
    latch_off();
    vTaskDelay(pdMS_TO_TICKS(50));

    // If USB is still powering the board, do this fake shutdown sequence by going into light sleep where pressing the button will wake it up

    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
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
    esp_deep_sleep_start();

    // alternate method of using light sleep instead of deep sleep for faking shutdown
    // enableWakeup();
    // esp_light_sleep_start(); // deep sleep wakeup is not supported on this hardware so instead we are going to use light sleep and then immediately reboot upon wake to simulate a fake shutdown then power back on
    // // button pressed, powered back on... do full reboot
    // esp_restart();
}

#endif