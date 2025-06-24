#include "tasks.h"

bool ps3ServiceStarted = false;

void task_bluetooth(void *parameters)
{
    delay(200); // just wait a moment i guess this is legacy

    Serial.println(F("Bluetooth Rest Service Beginning"));

    ble_setup();
    delay(10);
    for (;;)
    {
        ble_loop();
        delay(10);
    }
}

void setup_tasks()
{
    //  Bluetooth Task
    xTaskCreate(
        task_bluetooth,
        "Bluetooth",
        512 * 6,
        NULL,
        1000,
        NULL);
}