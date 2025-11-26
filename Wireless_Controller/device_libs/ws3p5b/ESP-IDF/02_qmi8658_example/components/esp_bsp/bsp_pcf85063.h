#ifndef __BSP_PCF85063_H__
#define __BSP_PCF85063_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <time.h>

#include "driver/i2c_master.h"


#define PCF85063_DEVICE_ADDR 0x51


typedef enum
{
    PCF85063_CONTROL_1 = 0,
    PCF85063_CONTROL_2,
    PCF85063_OFFSET,
    PCF85063_RAM_BYTE,
    PCF85063_SECONDS,
    PCF85063_MINUTES,
    PCF85063_HOURS,
    PCF85063_DAYS,
    PCF85063_WEEKDAYS,
    PCF85063_MONTHS,
    PCF85063_YEARS,
    PCF85063_SECOND_ALARM,
    PCF85063_MINUTE_ALARM,
    PCF85063_HOUR_ALARM,
    PCF85063_DAY_ALARM,
    PCF85063_WEEKDAY_ALARM,
    PCF85063_TIMER_VALUE,
    PCF85063_TIMER_MODE,
}pcf85063_reg_t;



#ifdef __cplusplus
extern "C" {
#endif

void bsp_pcf85063_init(i2c_master_bus_handle_t bus_handle);
bool bsp_pcf85063_get_time(struct tm *now_tm);
void bsp_pcf85063_set_time(struct tm *now_tm);
void bsp_pcf85063_test(void);


#ifdef __cplusplus
}
#endif

#endif