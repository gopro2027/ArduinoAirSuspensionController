#include <stdio.h>
#include <cstring>
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/i2c_master.h"

#include "bsp_i2c.h"

#define XPOWERS_CHIP_AXP2101
#include "XPowersLib.h"
static const char *TAG = "AXP2101";

XPowersPMU power;
i2c_master_dev_handle_t i2c_device;

static esp_err_t i2c_init(i2c_master_bus_handle_t bus_handle)
{
    i2c_device_config_t i2c_dev_conf = {};

    i2c_dev_conf.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    i2c_dev_conf.device_address = 0X34;
    i2c_dev_conf.scl_speed_hz = 400 * 1000;
    return i2c_master_bus_add_device(bus_handle, &i2c_dev_conf, &i2c_device);
}

static int pmu_register_read(uint8_t devAddr, uint8_t regAddr, uint8_t *data, uint8_t len)
{
    esp_err_t ret = ESP_FAIL;
    if (len == 0)
    {
        return 0;
    }
    if (data == NULL)
    {
        return -1;
    }

    if (bsp_i2c_lock(0))
    {
        ret = i2c_master_transmit_receive(i2c_device, (const uint8_t *)&regAddr, 1, data, len, pdMS_TO_TICKS(1000));
        bsp_i2c_unlock();
    }
    return (ret == ESP_OK) ? 0 : -1;
}

static int pmu_register_write_byte(uint8_t devAddr, uint8_t regAddr, uint8_t *data, uint8_t len)
{
    esp_err_t ret;
    if (data == NULL)
    {
        return ESP_FAIL;
    }

    uint8_t *write_buffer = (uint8_t *)malloc(sizeof(uint8_t) * (len + 1));
    if (!write_buffer)
    {
        return -1;
    }
    write_buffer[0] = regAddr;
    memcpy(write_buffer + 1, data, len);

    ret = i2c_master_transmit(i2c_device, write_buffer, len + 1, -1);
    free(write_buffer);
    return ret == ESP_OK ? 0 : -1;
}

esp_err_t bsp_axp2101_init(i2c_master_bus_handle_t bus_handle)
{
    i2c_init(bus_handle);
    //* Implemented using read and write callback methods, applicable to other platforms
    ESP_LOGI(TAG, "Implemented using read and write callback methods");
    if (power.begin(AXP2101_SLAVE_ADDRESS, pmu_register_read, pmu_register_write_byte))
    {
        ESP_LOGI(TAG, "Init PMU SUCCESS!");
    }
    else
    {
        ESP_LOGE(TAG, "Init PMU FAILED!");
        return ESP_FAIL;
    }

    printf("getID:0x%x\n", power.getChipID());

    // Set the minimum common working voltage of the PMU VBUS input,
    // below this value will turn off the PMU
    power.setVbusVoltageLimit(XPOWERS_AXP2101_VBUS_VOL_LIM_4V36);

    // Set the maximum current of the PMU VBUS input,
    // higher than this value will turn off the PMU
    power.setVbusCurrentLimit(XPOWERS_AXP2101_VBUS_CUR_LIM_1500MA);

    // Get the VSYS shutdown voltage
    uint16_t vol = power.getSysPowerDownVoltage();
    printf("->  getSysPowerDownVoltage:%u\n", vol);

    // Set VSY off voltage as 2600mV , Adjustment range 2600mV ~ 3300mV
    power.setSysPowerDownVoltage(2600);

    vol = power.getSysPowerDownVoltage();
    printf("->  getSysPowerDownVoltage:%u\n", vol);

    // DC1 IMAX=2A
    // 1500~3400mV,100mV/step,20steps
    power.setDC1Voltage(3300);
    printf("DC1  : %s   Voltage:%u mV \n", power.isEnableDC1() ? "+" : "-", power.getDC1Voltage());

    // DC2 IMAX=2A
    // 500~1200mV  10mV/step,71steps
    // 1220~1540mV 20mV/step,17steps
    power.setDC2Voltage(1000);
    printf("DC2  : %s   Voltage:%u mV \n", power.isEnableDC2() ? "+" : "-", power.getDC2Voltage());

    // DC3 IMAX = 2A
    // 500~1200mV,10mV/step,71steps
    // 1220~1540mV,20mV/step,17steps
    // 1600~3400mV,100mV/step,19steps
    power.setDC3Voltage(3300);
    printf("DC3  : %s   Voltage:%u mV \n", power.isEnableDC3() ? "+" : "-", power.getDC3Voltage());

    // DCDC4 IMAX=1.5A
    // 500~1200mV,10mV/step,71steps
    // 1220~1840mV,20mV/step,32steps
    power.setDC4Voltage(1000);
    printf("DC4  : %s   Voltage:%u mV \n", power.isEnableDC4() ? "+" : "-", power.getDC4Voltage());

    // DC5 IMAX=2A
    // 1200mV
    // 1400~3700mV,100mV/step,24steps
    power.setDC5Voltage(3300);
    printf("DC5  : %s   Voltage:%u mV \n", power.isEnableDC5() ? "+" : "-", power.getDC5Voltage());

    // ALDO1 IMAX=300mA
    // 500~3500mV, 100mV/step,31steps
    power.setALDO1Voltage(3300);

    // ALDO2 IMAX=300mA
    // 500~3500mV, 100mV/step,31steps
    power.setALDO2Voltage(3300);

    // ALDO3 IMAX=300mA
    // 500~3500mV, 100mV/step,31steps
    power.setALDO3Voltage(3300);

    // ALDO4 IMAX=300mA
    // 500~3500mV, 100mV/step,31steps
    power.setALDO4Voltage(3300);

    // BLDO1 IMAX=300mA
    // 500~3500mV, 100mV/step,31steps
    power.setBLDO1Voltage(1500);

    // BLDO2 IMAX=300mA
    // 500~3500mV, 100mV/step,31steps
    power.setBLDO2Voltage(2800);

    // CPUSLDO IMAX=30mA
    // 500~1400mV,50mV/step,19steps
    power.setCPUSLDOVoltage(1000);

    // DLDO1 IMAX=300mA
    // 500~3400mV, 100mV/step,29steps
    power.setDLDO1Voltage(3300);

    // DLDO2 IMAX=300mA
    // 500~1400mV, 50mV/step,2steps
    power.setDLDO2Voltage(3300);

    // power.enableDC1();
    power.enableDC2();
    power.enableDC3();
    power.enableDC4();
    power.enableDC5();
    power.enableALDO1();
    power.enableALDO2();
    power.enableALDO3();
    power.enableALDO4();
    power.enableBLDO1();
    power.enableBLDO2();
    power.enableCPUSLDO();
    power.enableDLDO1();
    power.enableDLDO2();

    printf("DCDC=======================================================================\n");
    printf("DC1  : %s   Voltage:%u mV \n", power.isEnableDC1() ? "+" : "-", power.getDC1Voltage());
    printf("DC2  : %s   Voltage:%u mV \n", power.isEnableDC2() ? "+" : "-", power.getDC2Voltage());
    printf("DC3  : %s   Voltage:%u mV \n", power.isEnableDC3() ? "+" : "-", power.getDC3Voltage());
    printf("DC4  : %s   Voltage:%u mV \n", power.isEnableDC4() ? "+" : "-", power.getDC4Voltage());
    printf("DC5  : %s   Voltage:%u mV \n", power.isEnableDC5() ? "+" : "-", power.getDC5Voltage());
    printf("ALDO=======================================================================\n");
    printf("ALDO1: %s   Voltage:%u mV\n", power.isEnableALDO1() ? "+" : "-", power.getALDO1Voltage());
    printf("ALDO2: %s   Voltage:%u mV\n", power.isEnableALDO2() ? "+" : "-", power.getALDO2Voltage());
    printf("ALDO3: %s   Voltage:%u mV\n", power.isEnableALDO3() ? "+" : "-", power.getALDO3Voltage());
    printf("ALDO4: %s   Voltage:%u mV\n", power.isEnableALDO4() ? "+" : "-", power.getALDO4Voltage());
    printf("BLDO=======================================================================\n");
    printf("BLDO1: %s   Voltage:%u mV\n", power.isEnableBLDO1() ? "+" : "-", power.getBLDO1Voltage());
    printf("BLDO2: %s   Voltage:%u mV\n", power.isEnableBLDO2() ? "+" : "-", power.getBLDO2Voltage());
    printf("CPUSLDO====================================================================\n");
    printf("CPUSLDO: %s Voltage:%u mV\n", power.isEnableCPUSLDO() ? "+" : "-", power.getCPUSLDOVoltage());
    printf("DLDO=======================================================================\n");
    printf("DLDO1: %s   Voltage:%u mV\n", power.isEnableDLDO1() ? "+" : "-", power.getDLDO1Voltage());
    printf("DLDO2: %s   Voltage:%u mV\n", power.isEnableDLDO2() ? "+" : "-", power.getDLDO2Voltage());
    printf("===========================================================================\n");

    // Set the time of pressing the button to turn off
    power.setPowerKeyPressOffTime(XPOWERS_POWEROFF_4S);
    uint8_t opt = power.getPowerKeyPressOffTime();
    printf("PowerKeyPressOffTime:");
    switch (opt)
    {
    case XPOWERS_POWEROFF_4S:
        printf("4 Second\n");
        break;
    case XPOWERS_POWEROFF_6S:
        printf("6 Second\n");
        break;
    case XPOWERS_POWEROFF_8S:
        printf("8 Second\n");
        break;
    case XPOWERS_POWEROFF_10S:
        printf("10 Second\n");
        break;
    default:
        break;
    }
    // Set the button power-on press time
    power.setPowerKeyPressOnTime(XPOWERS_POWERON_128MS);
    opt = power.getPowerKeyPressOnTime();
    printf("PowerKeyPressOnTime:");
    switch (opt)
    {
    case XPOWERS_POWERON_128MS:
        printf("128 Ms\n");
        break;
    case XPOWERS_POWERON_512MS:
        printf("512 Ms\n");
        break;
    case XPOWERS_POWERON_1S:
        printf("1 Second\n");
        break;
    case XPOWERS_POWERON_2S:
        printf("2 Second\n");
        break;
    default:
        break;
    }

    printf("===========================================================================\n");

    bool en;

    // DCDC 120%(130%) high voltage turn off PMIC function
    en = power.getDCHighVoltagePowerDownEn();
    printf("getDCHighVoltagePowerDownEn:");
    printf(en ? "ENABLE\n" : "DISABLE\n");
    // DCDC1 85% low voltage turn off PMIC function
    en = power.getDC1LowVoltagePowerDownEn();
    printf("getDC1LowVoltagePowerDownEn:");
    printf(en ? "ENABLE\n" : "DISABLE\n");
    // DCDC2 85% low voltage turn off PMIC function
    en = power.getDC2LowVoltagePowerDownEn();
    printf("getDC2LowVoltagePowerDownEn:");
    printf(en ? "ENABLE\n" : "DISABLE\n");
    // DCDC3 85% low voltage turn off PMIC function
    en = power.getDC3LowVoltagePowerDownEn();
    printf("getDC3LowVoltagePowerDownEn:");
    printf(en ? "ENABLE\n" : "DISABLE\n");
    // DCDC4 85% low voltage turn off PMIC function
    en = power.getDC4LowVoltagePowerDownEn();
    printf("getDC4LowVoltagePowerDownEn:");
    printf(en ? "ENABLE\n" : "DISABLE\n");
    // DCDC5 85% low voltage turn off PMIC function
    en = power.getDC5LowVoltagePowerDownEn();
    printf("getDC5LowVoltagePowerDownEn:");
    printf(en ? "ENABLE\n" : "DISABLE\n");

    // power.setDCHighVoltagePowerDown(true);
    // power.setDC1LowVoltagePowerDown(true);
    // power.setDC2LowVoltagePowerDown(true);
    // power.setDC3LowVoltagePowerDown(true);
    // power.setDC4LowVoltagePowerDown(true);
    // power.setDC5LowVoltagePowerDown(true);

    // It is necessary to disable the detection function of the TS pin on the board
    // without the battery temperature detection function, otherwise it will cause abnormal charging
    power.disableTSPinMeasure();

    // power.enableTemperatureMeasure();

    // Enable internal ADC detection
    power.enableBattDetection();
    power.enableVbusVoltageMeasure();
    power.enableBattVoltageMeasure();
    power.enableSystemVoltageMeasure();

    /*
        The default setting is CHGLED is automatically controlled by the PMU.
      - XPOWERS_CHG_LED_OFF,
      - XPOWERS_CHG_LED_BLINK_1HZ,
      - XPOWERS_CHG_LED_BLINK_4HZ,
      - XPOWERS_CHG_LED_ON,
      - XPOWERS_CHG_LED_CTRL_CHG,
      * */
    power.setChargingLedMode(XPOWERS_CHG_LED_OFF);

    // Force add pull-up
    // pinMode(pmu_irq_pin, INPUT_PULLUP);
    // attachInterrupt(pmu_irq_pin, setFlag, FALLING);

    // Disable all interrupts
    power.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
    // Clear all interrupt flags
    power.clearIrqStatus();
    // Enable the required interrupt function
    power.enableIRQ(
        XPOWERS_AXP2101_BAT_INSERT_IRQ | XPOWERS_AXP2101_BAT_REMOVE_IRQ |    // BATTERY
        XPOWERS_AXP2101_VBUS_INSERT_IRQ | XPOWERS_AXP2101_VBUS_REMOVE_IRQ |  // VBUS
        XPOWERS_AXP2101_PKEY_SHORT_IRQ | XPOWERS_AXP2101_PKEY_LONG_IRQ |     // POWER KEY
        XPOWERS_AXP2101_BAT_CHG_DONE_IRQ | XPOWERS_AXP2101_BAT_CHG_START_IRQ // CHARGE
                                                                             //  XPOWERS_AXP2101_PKEY_NEGATIVE_IRQ | XPOWERS_AXP2101_PKEY_POSITIVE_IRQ   |   //POWER KEY
    );

    // Set the precharge charging current
    power.setPrechargeCurr(XPOWERS_AXP2101_PRECHARGE_50MA);
    // Set constant current charge current limit
    power.setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_200MA);
    // Set stop charging termination current
    power.setChargerTerminationCurr(XPOWERS_AXP2101_CHG_ITERM_25MA);

    // Set charge cut-off voltage
    power.setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V1);

    // Set the watchdog trigger event type
    power.setWatchdogConfig(XPOWERS_AXP2101_WDT_IRQ_TO_PIN);
    // Set watchdog timeout
    power.setWatchdogTimeout(XPOWERS_AXP2101_WDT_TIMEOUT_4S);
    // Enable watchdog to trigger interrupt event
    power.enableWatchdog();

    // power.disableWatchdog();

    // Enable Button Battery charge
    power.enableButtonBatteryCharge();

    // Set Button Battery charge voltage
    power.setButtonBatteryChargeVoltage(3300);
    return ESP_OK;
}

esp_err_t esp_axp2101_port_init1(i2c_master_bus_handle_t bus_handle)
{
    i2c_init(bus_handle);
    //* Implemented using read and write callback methods, applicable to other platforms
    ESP_LOGI(TAG, "Implemented using read and write callback methods");
    if (power.begin(AXP2101_SLAVE_ADDRESS, pmu_register_read, pmu_register_write_byte))
    {
        ESP_LOGI(TAG, "Init PMU SUCCESS!");
    }
    else
    {
        ESP_LOGE(TAG, "Init PMU FAILED!");
        return ESP_FAIL;
    }

    // Turn off not use power channel
    power.disableDC2();
    power.disableDC3();
    power.disableDC4();
    power.disableDC5();

    // power.disableALDO1();
    power.disableALDO2();
    power.disableALDO3();
    power.disableALDO4();

    power.disableBLDO1();
    power.disableBLDO2();

    // power.disableDLDO1();
    // power.disableDLDO2();

    // power.disableCPUSLDO();
    // power.disableDLDO1();
    // power.disableDLDO2();

    // //ESP32s3 Core VDD
    // power.setDC3Voltage(3300);
    // power.enableDC3();

    // //Extern 3.3V VDD
    // power.setDC1Voltage(3300);
    // power.enableDC1();

    // // CAM DVDD  1500~1800
    // power.setALDO1Voltage(1800);
    // // power.setALDO1Voltage(1500);
    // power.enableALDO1();

    // // CAM DVDD 2500~2800
    // power.setALDO2Voltage(2800);
    // power.enableALDO2();

    // // CAM AVDD 2800~3000
    // power.setALDO4Voltage(3000);
    // power.enableALDO4();
    power.enableCPUSLDO();

    power.enableDLDO1();
    power.enableDLDO2();

    power.setALDO1Voltage(3300);
    power.enableALDO1();

    power.setBLDO1Voltage(1500);
    power.enableBLDO1();

    power.setBLDO2Voltage(2800);
    power.enableBLDO2();

    ESP_LOGI(TAG, "DCDC=======================================================================\n");
    ESP_LOGI(TAG, "DC1  : %s   Voltage:%u mV \n", power.isEnableDC1() ? "+" : "-", power.getDC1Voltage());
    ESP_LOGI(TAG, "DC2  : %s   Voltage:%u mV \n", power.isEnableDC2() ? "+" : "-", power.getDC2Voltage());
    ESP_LOGI(TAG, "DC3  : %s   Voltage:%u mV \n", power.isEnableDC3() ? "+" : "-", power.getDC3Voltage());
    ESP_LOGI(TAG, "DC4  : %s   Voltage:%u mV \n", power.isEnableDC4() ? "+" : "-", power.getDC4Voltage());
    ESP_LOGI(TAG, "DC5  : %s   Voltage:%u mV \n", power.isEnableDC5() ? "+" : "-", power.getDC5Voltage());
    ESP_LOGI(TAG, "ALDO=======================================================================\n");
    ESP_LOGI(TAG, "ALDO1: %s   Voltage:%u mV\n", power.isEnableALDO1() ? "+" : "-", power.getALDO1Voltage());
    ESP_LOGI(TAG, "ALDO2: %s   Voltage:%u mV\n", power.isEnableALDO2() ? "+" : "-", power.getALDO2Voltage());
    ESP_LOGI(TAG, "ALDO3: %s   Voltage:%u mV\n", power.isEnableALDO3() ? "+" : "-", power.getALDO3Voltage());
    ESP_LOGI(TAG, "ALDO4: %s   Voltage:%u mV\n", power.isEnableALDO4() ? "+" : "-", power.getALDO4Voltage());
    ESP_LOGI(TAG, "BLDO=======================================================================\n");
    ESP_LOGI(TAG, "BLDO1: %s   Voltage:%u mV\n", power.isEnableBLDO1() ? "+" : "-", power.getBLDO1Voltage());
    ESP_LOGI(TAG, "BLDO2: %s   Voltage:%u mV\n", power.isEnableBLDO2() ? "+" : "-", power.getBLDO2Voltage());
    ESP_LOGI(TAG, "CPUSLDO====================================================================\n");
    ESP_LOGI(TAG, "CPUSLDO: %s Voltage:%u mV\n", power.isEnableCPUSLDO() ? "+" : "-", power.getCPUSLDOVoltage());
    ESP_LOGI(TAG, "DLDO=======================================================================\n");
    ESP_LOGI(TAG, "DLDO1: %s   Voltage:%u mV\n", power.isEnableDLDO1() ? "+" : "-", power.getDLDO1Voltage());
    ESP_LOGI(TAG, "DLDO2: %s   Voltage:%u mV\n", power.isEnableDLDO2() ? "+" : "-", power.getDLDO2Voltage());
    ESP_LOGI(TAG, "===========================================================================\n");

    power.clearIrqStatus();

    power.enableVbusVoltageMeasure();
    power.enableBattVoltageMeasure();
    power.enableSystemVoltageMeasure();
    power.enableTemperatureMeasure();

    // It is necessary to disable the detection function of the TS pin on the board
    // without the battery temperature detection function, otherwise it will cause abnormal charging
    power.disableTSPinMeasure();

    // Disable all interrupts
    power.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
    // Clear all interrupt flags
    power.clearIrqStatus();
    // Enable the required interrupt function
    power.enableIRQ(
        XPOWERS_AXP2101_BAT_INSERT_IRQ | XPOWERS_AXP2101_BAT_REMOVE_IRQ |    // BATTERY
        XPOWERS_AXP2101_VBUS_INSERT_IRQ | XPOWERS_AXP2101_VBUS_REMOVE_IRQ |  // VBUS
        XPOWERS_AXP2101_PKEY_SHORT_IRQ | XPOWERS_AXP2101_PKEY_LONG_IRQ |     // POWER KEY
        XPOWERS_AXP2101_BAT_CHG_DONE_IRQ | XPOWERS_AXP2101_BAT_CHG_START_IRQ // CHARGE
        // XPOWERS_AXP2101_PKEY_NEGATIVE_IRQ | XPOWERS_AXP2101_PKEY_POSITIVE_IRQ   |   //POWER KEY
    );

    /*
      The default setting is CHGLED is automatically controlled by the power.
    - XPOWERS_CHG_LED_OFF,
    - XPOWERS_CHG_LED_BLINK_1HZ,
    - XPOWERS_CHG_LED_BLINK_4HZ,
    - XPOWERS_CHG_LED_ON,
    - XPOWERS_CHG_LED_CTRL_CHG,
    * */
    power.setChargingLedMode(XPOWERS_CHG_LED_CTRL_CHG);

    // Set the precharge charging current
    power.setPrechargeCurr(XPOWERS_AXP2101_PRECHARGE_50MA);
    // Set constant current charge current limit
    power.setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_200MA);
    // Set stop charging termination current
    power.setChargerTerminationCurr(XPOWERS_AXP2101_CHG_ITERM_25MA);

    // Set charge cut-off voltage
    power.setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V1);

    // Set the watchdog trigger event type
    // power.setWatchdogConfig(XPOWERS_AXP2101_WDT_IRQ_TO_PIN);
    // Set watchdog timeout
    power.setWatchdogTimeout(XPOWERS_AXP2101_WDT_TIMEOUT_4S);
    // Enable watchdog to trigger interrupt event
    power.enableWatchdog();
    return ESP_OK;
}

void pmu_isr_handler(void)
{
    // Get PMU Interrupt Status Register
    power.getIrqStatus();

    if (power.isDropWarningLevel2Irq())
    {
        ESP_LOGI(TAG, "isDropWarningLevel2");
    }
    if (power.isDropWarningLevel1Irq())
    {
        ESP_LOGI(TAG, "isDropWarningLevel1");
    }
    if (power.isGaugeWdtTimeoutIrq())
    {
        ESP_LOGI(TAG, "isWdtTimeout");
    }
    if (power.isBatChargerOverTemperatureIrq())
    {
        ESP_LOGI(TAG, "isBatChargeOverTemperature");
    }
    if (power.isBatWorkOverTemperatureIrq())
    {
        ESP_LOGI(TAG, "isBatWorkOverTemperature");
    }
    if (power.isBatWorkUnderTemperatureIrq())
    {
        ESP_LOGI(TAG, "isBatWorkUnderTemperature");
    }
    if (power.isVbusInsertIrq())
    {
        ESP_LOGI(TAG, "isVbusInsert");
    }
    if (power.isVbusRemoveIrq())
    {
        ESP_LOGI(TAG, "isVbusRemove");
    }
    if (power.isBatInsertIrq())
    {
        ESP_LOGI(TAG, "isBatInsert");
    }
    if (power.isBatRemoveIrq())
    {
        ESP_LOGI(TAG, "isBatRemove");
    }
    if (power.isPekeyShortPressIrq())
    {
        ESP_LOGI(TAG, "isPekeyShortPress");
    }
    if (power.isPekeyLongPressIrq())
    {
        ESP_LOGI(TAG, "isPekeyLongPress");
    }
    if (power.isPekeyNegativeIrq())
    {
        ESP_LOGI(TAG, "isPekeyNegative");
    }
    if (power.isPekeyPositiveIrq())
    {
        ESP_LOGI(TAG, "isPekeyPositive");
    }
    if (power.isWdtExpireIrq())
    {
        ESP_LOGI(TAG, "isWdtExpire");
    }
    if (power.isLdoOverCurrentIrq())
    {
        ESP_LOGI(TAG, "isLdoOverCurrentIrq");
    }
    if (power.isBatfetOverCurrentIrq())
    {
        ESP_LOGI(TAG, "isBatfetOverCurrentIrq");
    }
    if (power.isBatChargeDoneIrq())
    {
        ESP_LOGI(TAG, "isBatChargeDone");
    }
    if (power.isBatChargeStartIrq())
    {
        ESP_LOGI(TAG, "isBatChargeStart");
    }
    if (power.isBatDieOverTemperatureIrq())
    {
        ESP_LOGI(TAG, "isBatDieOverTemperature");
    }
    if (power.isChargeOverTimeoutIrq())
    {
        ESP_LOGI(TAG, "isChargeOverTimeout");
    }
    if (power.isBatOverVoltageIrq())
    {
        ESP_LOGI(TAG, "isBatOverVoltage");
    }
    // Clear PMU Interrupt Status Register
    power.clearIrqStatus();
}
