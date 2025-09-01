#if defined(WAVESHARE_BOARD)
#include "BAT_Driver.h"
#include "PWR_Key.h"

void waveshare_init()
{
    BAT_Init();
    PWR_Init();
}

void waveshare_loop()
{
    static uint32_t pwr_next = 0;

    auto const now = millis();
    // Power-key state machine tick every 100 ms
    if (now >= pwr_next)
    {
        PWR_Loop();
        pwr_next = now + 100; // 100 ms period → Device_*_Time in 0.1s units
    }

    static uint32_t t = 0;
    if (millis() - t > 2000)
    {
        t = millis();
        ESP_LOGI("BAT", "Vbat=%.3f", BAT_Get_Volts());
    }
}
#endif