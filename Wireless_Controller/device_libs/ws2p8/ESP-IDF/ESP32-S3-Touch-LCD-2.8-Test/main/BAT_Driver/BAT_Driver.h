#pragma once

#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"


/*---------------------------------------------------------------
        ADC General Macros
---------------------------------------------------------------*/
//ADC1 Channels
#define EXAMPLE_ADC1_CHAN       ADC_CHANNEL_7              // GPIO8
#define EXAMPLE_ADC_ATTEN       ADC_ATTEN_DB_12             // ADC_ATTEN_DB_12

#define Measurement_offset 0.994500  

extern float BAT_analogVolts;

void BAT_Init(void);
float BAT_Get_Volts(void);