#include "BAT_Driver.h"

const static char *ADC_TAG = "ADC";

float BAT_analogVolts = 0;

/*---------------------------------------------------------------
        ADC Calibration  
---------------------------------------------------------------*/
static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(ADC_TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(ADC_TAG, "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle;
    if (ret == ESP_OK) {
        ESP_LOGI(ADC_TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(ADC_TAG, "eFuse not burnt, skip software calibration");
    } else {
        ESP_LOGE(ADC_TAG, "Invalid arg or no memory");
    }

    return calibrated;
}

// static void example_adc_calibration_deinit(adc_cali_handle_t handle)
// {
// #if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
//     ESP_LOGI(ADC_TAG, "deregister %s calibration scheme", "Curve Fitting");
//     ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(handle));

// #elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
//     ESP_LOGI(ADC_TAG, "deregister %s calibration scheme", "Line Fitting");
//     ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(handle));
// #endif
// }

adc_oneshot_unit_handle_t adc1_handle;
bool do_calibration1_chan3;
adc_cali_handle_t adc1_cali_chan3_handle = NULL;

int adc_raw[2][10];                           
int voltage[2][10];                          
void ADC_Init(void)
{
    //-------------ADC1 Init---------------//
    adc_oneshot_unit_init_cfg_t init_config1 = {                          
        .unit_id = ADC_UNIT_1,                                               
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));      

    //-------------ADC1 Config---------------//
    adc_oneshot_chan_cfg_t config = {
        .atten = EXAMPLE_ADC_ATTEN,                                               
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, EXAMPLE_ADC1_CHAN3, &config)); 

    //-------------ADC1 Calibration Init---------------//
    do_calibration1_chan3 = example_adc_calibration_init(ADC_UNIT_1, EXAMPLE_ADC1_CHAN3, EXAMPLE_ADC_ATTEN, &adc1_cali_chan3_handle);     

    // //Tear Down
    // ESP_ERROR_CHECK(adc_oneshot_del_unit(adc1_handle));
    // if (do_calibration1_chan3) {                                                                                                                
    //     example_adc_calibration_deinit(adc1_cali_chan3_handle);                                                                                 
    // }
}

void BAT_Init(void)
{
    ADC_Init();
}
float BAT_Get_Volts(void)
{
    adc_oneshot_read(adc1_handle, EXAMPLE_ADC1_CHAN3, &adc_raw[0][0]);                                                     
    // printf( "ADC%d Channel[%d] Raw Data: %d\r\n", ADC_UNIT_1 + 1, EXAMPLE_ADC1_CHAN3, adc_raw[0][0]);                                                
    if (do_calibration1_chan3) {                                                                                           
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan3_handle, adc_raw[0][0], &voltage[0][0]));                    
        // printf("ADC%d Channel[%d] Cali Voltage: %d mV\r\n", ADC_UNIT_1 + 1, EXAMPLE_ADC1_CHAN3, voltage[0][0]);                
        BAT_analogVolts = (float)(voltage[0][0] * 3.0 / 1000.0) / Measurement_offset;
        // printf("BAT voltage : %.2f V\r\n", BAT_analogVolts);
    }
    return BAT_analogVolts;
}