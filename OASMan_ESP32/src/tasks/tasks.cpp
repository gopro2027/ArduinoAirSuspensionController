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

#if ENABLE_PS3_CONTROLLER_SUPPORT
void task_ps3_controller(void *parameters)
{
    ps3_controller_setup();
    for (;;)
    {
        ps3_controller_loop();
        task_sleep(500);
    }
}
#endif

void task_compressor(void *parameters)
{
    for (;;)
    {
        compressorLogic();
        task_sleep(100);
    }
}

// NOTICE: Parameters is supposed to be an array, but idc im just gonna make it the Wheel * because I can. No need to actually create an array to pass it in.
void task_wheel(void *parameters)
{
    for (;;)
    {
        ((Wheel *)parameters)->loop();
        task_sleep(100);
    }
}

void setup_tasks()
{

#if ENABLE_PS3_CONTROLLER_SUPPORT
    bool ps3Mode = getPS3ControllerMode();
    setPS3ControllerMode(false); // tell it to turn off for the next boot.
#if DEBUG_ALWAYS_BOOT_PS3_CONTROLLER_MODE
    ps3Mode = true;
#endif
#else
    bool ps3Mode = false;
#endif

    if (ps3Mode == false)
    {
        // Bluetooth Task
        xTaskCreate(
            task_bluetooth,
            "Bluetooth",
            512 * 4,
            NULL,
            1000,
            NULL);
    }

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

    // Compressor Control Task
    xTaskCreate(
        task_compressor,
        "Compressor Control",
        512 * 3,
        NULL,
        1000,
        NULL);

    // Wheel Tasks
    for (int i = 0; i < 4; i++)
    {
        xTaskCreate(
            task_wheel,
            "Wheel Task",
            512 * 2,
            getWheel(i),
            1000,
            NULL);
    }

#if ENABLE_PS3_CONTROLLER_SUPPORT
    if (ps3Mode)
    {
        // PS3 Controller Task
        xTaskCreate(
            task_ps3_controller,
            "PS3 Controller",
            512 * 5,
            NULL,
            1000,
            NULL);
    }
#endif
}