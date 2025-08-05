#include "tasks.h"

bool ps3ServiceStarted = false;

void writeToSpiffsLog(char *text);

void task_bluetooth(void *parameters)
{
    delay(200); // just wait a moment i guess this is legacy

    delay(1000); // wait for the bluepad32 to start first

#if ENABLE_PS3_CONTROLLER_SUPPORT
    // wait for ps3 controller service to boot
    while (ps3ServiceStarted == false)
    {
        delay(1);
    }
    delay(50);
#endif

    Serial.println(F("Bluetooth Rest Service Beginning"));
    ble_setup();
    delay(10);
    for (;;)
    {
        ble_loop();
        delay(10);
    }
}
bool do_dance = false;
void easterEggFunc()
{
    if (do_dance)
    {
        do_dance = false;
#if ENABLE_PS3_CONTROLLER_SUPPORT
        doDance();
#endif
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
            easterEggFunc();
            Serial.println(F("SSD1306 allocation failed!"));
            delay(100);
        }
    }
    drawsplashscreen();
    for (;;)
    {
        drawPSIReadings();
        easterEggFunc();
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
        delay(100);
    }
}
#endif

void task_compressor(void *parameters)
{
    for (;;)
    {
        getCompressor()->loop();
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

void task_trainAI(void *parameters)
{
    trainAIModels();
    vTaskDelete(NULL);
}

void setup_tasks()
{

    //   Bluetooth Task

    xTaskCreate(
        task_bluetooth,
        "Bluetooth",
        512 * 6,
        NULL,
        1000,
        NULL);

#if SCREEN_ENABLED == true
    // Manifold OLED Task
    xTaskCreate(
        task_screen,
        "OLED",
        512 * 4,
        NULL,
        1000,
        NULL);
#endif

    // Compressor Control Task
    xTaskCreate(
        task_compressor,
        "Compressor Control",
        512 * 4,
        NULL,
        1000,
        NULL);

    // Wheel Tasks
    for (int i = 0; i < 4; i++)
    {
        xTaskCreate(
            task_wheel,
            "Wheel Task",
            512 * 5,
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

    //  Train AI Task
    xTaskCreate(
        task_trainAI,
        "trainAI",
        512 * 4,
        NULL,
        1000,
        NULL);
}