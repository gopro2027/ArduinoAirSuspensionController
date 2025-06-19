#include "tasks.h"

bool ps3ServiceStarted = false;

void writeToSpiffsLog(char *text);

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

#if USE_BLE == false
    bt.begin(BT_NAME);
    for (;;)
    {
        bt_cmd();
        delay(10);
    }
#else
    ble_setup();
    delay(10);
    for (;;)
    {
        ble_loop();
        delay(10);
    }
#endif
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
        // ideally we need to make this loop synced or get rid of this delay so they all start at the same time too but eh idk if it will be that beneficial tbh
        ((Wheel *)parameters)->loop();
        delay(100);
    }
}

bool do_dance = false;
void task_ota(void *parameters)
{
    delay(150);
    while (startOTAServiceRequest == false)
    {
        // This is completely unrelated to the ota function but it is here because it is blocking and needs to be ran from it's own task and this task is basically unused so I am putting it in here rather than allocating memory for a new task.
        if (do_dance)
        {
            do_dance = false;
            doDance();
        }
        delay(50);
    }
    ota_setup();
    delay(150);
    for (;;)
    {
        ota_loop();
        delay(150);
    }
}

#ifdef parabolaLearn
void task_parabolaLearn(void *parameters)
{
    learnParabolaSetup();
    for (;;)
    {
        if (learnParabolaLoop())
        {
            Serial.println("Finished parabola learn!");
            setinternalReboot(true);
            ESP.restart();
            return;
        }
        delay(100);
    }
}

void start_parabolaLearnTask()
{
    //  Parabola Task
    xTaskCreate(
        task_parabolaLearn,
        "parabolalearn",
        512 * 4,
        NULL,
        1000,
        NULL);
}
#endif

void task_trainAI(void *parameters)
{
    trainAIModels();
    vTaskDelete(NULL);
}

void setup_tasks()
{
    //  Bluetooth Task
    xTaskCreate(
        task_bluetooth,
        "Bluetooth",
        512 * 5,
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

    // OTA Task
    xTaskCreate(
        task_ota,
        "OTA",
        512 * 5,
        NULL,
        1000,
        NULL);

    //  Train AI Task
    xTaskCreate(
        task_trainAI,
        "trainAI",
        512 * 4,
        NULL,
        1000,
        NULL);
}