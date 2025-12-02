

// PWR_Key.c
#include "PWR_Key.h"

#include <stdbool.h>
#include <stdint.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_sleep.h"


// Internal state
static uint8_t Booted_From_State = 0; // 0:no power, 1:key held at boot, 2:booted from usb plugged in
static uint8_t Device_State = 0;      // 0:none, 1:sleep, 2:shutdown
static uint16_t Long_Press = 0;




auto woke_up = true;
void PWR_Init(void)
{

    power_key_setup();
    power_latch_off();
    vTaskDelay(pdMS_TO_TICKS(10));

    if (power_key_pressed())
    {
        Booted_From_State = 1; // booted by holding the key
        power_latch_on();            // keep board on
    }
    else
    {
        Booted_From_State = 2; // booted without key (e.g., USB)
        // power_latch_on();    // we can uncomment this if we want the device to stay on when the usb is unplugged. Personally I prefer the device turn off on it's own due to the nature of our system being in a car. The device may turn on when the car is turned on, then turn off when the car is turned off.
    }
    woke_up = true;
}

void PWR_Loop(void)
{

    if (power_key_pressed())
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
                set_brightness(0); // this is so the user knows the device is about to turn off so they can let go of the button. Once they let go, device state being 2 makes the device shut down. The reason being, we want to make sure the user is not actively pressing the button upon shutdown time to ensure that is is not woken back up immediately after shutdown accidentally
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



void onWakeup()
{
    log_i("Waking up");
    power_disable_wakeup_lightsleep();
    woke_up = true;

    if (!power_key_pressed())
    {
        // woken up by 10 minute timer instead of key press, go ahead and shut down
        Shutdown();
    }
    else
    {
        set_brightness(getBrightnessFloat());
        showDialog("Waking up, reconnecting...", lv_color_hex(0xFFFF00), 30000);
    }
}

void Fall_Asleep(void)
{
    log_i("Falling asleep");
    disconnect(false); // disconnect from any BLE connections
    set_brightness(0);
    vTaskDelay(pdMS_TO_TICKS(50)); // give time for backlight to turn off before sleeping

    esp_sleep_enable_timer_wakeup(10 * 60 * 1000000); // 10 minutes in sleep will shutdown the device fully
    power_enable_wakeup_lightsleep();
    // Enter light sleep (returns on wake)
    esp_light_sleep_start();
    onWakeup();
}

void Shutdown(void)
{
    log_i("Shutting down");
    // Turn off UI/backlight, drop the power latch
    set_brightness(0);
    power_latch_off();
    vTaskDelay(pdMS_TO_TICKS(50));

    // If USB is still powering the board, do this fake shutdown sequence by going into deep sleep where pressing the button will wake it up (deep sleep naturally causes reboot upon waking)
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    
    // special function needed for deep sleep wakeup
    power_enable_wakeup_deepsleep();

    esp_deep_sleep_start();
    // no reboot call required, waking up from deep sleep automatically reboots the device



    // alternate method of using light sleep instead of deep sleep for faking shutdown
    // power_enable_wakeup();
    // esp_light_sleep_start(); // deep sleep wakeup is not supported on this hardware so instead we are going to use light sleep and then immediately reboot upon wake to simulate a fake shutdown then power back on
    // // button pressed, powered back on... do full reboot
    // esp_restart();
}

