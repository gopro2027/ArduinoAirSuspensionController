#include "tasks.h"

bool ps3ServiceStarted = false;

void task_bluetooth(void *parameters)
{
    delay(200); // just wait a moment i guess this is legacy

#if ENABLE_PS3_CONTROLLER_SUPPORT
    // wait for ps3 controller service to boot
    while (ps3ServiceStarted == false)
    {
        delay(1);
    }
    delay(50);
#endif

    Serial.println(F("Bluetooth Rest Service Beginning"));

    bt.begin(BT_NAME);
    for (;;)
    {
        bt_cmd();
        delay(10);
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
            delay(100);
        }
    }
    drawsplashscreen();
    for (;;)
    {
        drawPSIReadings();
        delay(100); // 10fps should be plenty
    }
}

#endif

#if ENABLE_PS3_CONTROLLER_SUPPORT
void task_ps3_controller(void *parameters)
{
    ps3_controller_setup();
    ps3ServiceStarted = true;
    for (;;)
    {
        ps3_controller_loop();
        delay(500);
    }
}
#endif

void task_compressor(void *parameters)
{
    for (;;)
    {
        compressorLogic();
        delay(100);
    }
}

// NOTICE: Parameters is supposed to be an array, but idc im just gonna make it the Wheel * because I can. No need to actually create an array to pass it in.
void task_wheel(void *parameters)
{
    for (;;)
    {
        ((Wheel *)parameters)->loop();
        delay(100);
    }
}

void task_adc_read(void *parameters)
{
    for (;;)
    {
        ADSLoop();
        delay(1);
    }
}

void setup_tasks()
{
    //  Bluetooth Task
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

#if USE_ADS
    // ADS Queue Task
    xTaskCreate(
        task_adc_read,
        "ADS Queue",
        512 * 3,
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
            512 * 3,
            getWheel(i),
            1000,
            NULL);
    }

#if ENABLE_PS3_CONTROLLER_SUPPORT
    // PS3 Controller Task
    xTaskCreate(
        task_ps3_controller,
        "PS3 Controller",
        512 * 5,
        NULL,
        1000,
        NULL);
#endif
}