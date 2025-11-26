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

void task_waveshare(void *parameters)
{

    Serial.println(F("Waveshare Service Beginning"));

    waveshare_init();
    delay(10);
    for (;;)
    {
        waveshare_loop();
        delay(10);
    }
}

#define BLE_TASK_STACK_SIZE 512 * 6

// Statically allocate the buffer for the task's stack.
// It must be of type StackType_t and large enough to hold MY_TASK_STACK_SIZE words.
// static StackType_t *myTaskStack;
//[MY_TASK_STACK_SIZE];

// Statically allocate the variable for the task's TCB.
// It must be of type StaticTask_t.
// static StaticTask_t myTaskBuffer;
void setup_tasks()
{
    //  Bluetooth Task
    xTaskCreate(
        task_bluetooth,
        "Bluetooth",
        BLE_TASK_STACK_SIZE,
        NULL,
        1000,
        NULL);

    //  Waveshare board/button Task
    xTaskCreate(
        task_waveshare,
        "Waveshare",
        512 * 6,
        NULL,
        1000,
        NULL);


    // myTaskStack = (StackType_t *)heap_caps_malloc((BLE_TASK_STACK_SIZE), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT); // default didn't work, internal does seem to work

    // xTaskCreateStatic(task_bluetooth, "XYZ_TASK", BLE_TASK_STACK_SIZE, NULL, 1000, myTaskStack, &myTaskBuffer);
}