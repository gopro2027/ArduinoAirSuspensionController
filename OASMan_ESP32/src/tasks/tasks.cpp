#include "tasks.h"

void task_bluetooth(void *parameters)
{
    bt.begin(BT_NAME);
    task_sleep(200); // just wait a second
    for (;;)
    {
        bt_cmd();
        task_sleep(10);
    }
}

#if SCREEN_ENABLED == true
void task_screen(void *parameters)
{
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
    {
        for (;;)
        {
            Serial.println(F("SSD1306 allocation failed!"));
            task_sleep(100);
        }
    }
    drawsplashscreen();
    for (;;)
    {
        drawPSIReadings();
        task_sleep(100); // 10fps should be plenty
    }
}

#endif

void task_readPressures(void *parameters)
{
    for (;;)
    {
        if (!isAnyWheelActive())
        {
            readPressures();
        }
        task_sleep(100);
    }
}

void task_compressor(void *parameters)
{
    for (;;)
    {
        compressorLogic();
        task_sleep(100);
    }
}

void setup_tasks()
{

    // Bluetooth Task
    xTaskCreate(
        task_bluetooth,
        "Bluetooth",
        512 * 4,
        NULL,
        1000,
        NULL);

#if SCREEN_ENABLED == true
    // Manifold OLED Task
    xTaskCreate(
        task_screen,
        "OLED",
        512 * 2,
        NULL,
        1000,
        NULL);
#endif

    // Read Pressures Task
    xTaskCreate(
        task_readPressures,
        "Read Pressures",
        512 * 2,
        NULL,
        1000,
        NULL);

    // Compressor Control Task
    xTaskCreate(
        task_compressor,
        "Compressor Control",
        512 * 3,
        NULL,
        1000,
        NULL);
}