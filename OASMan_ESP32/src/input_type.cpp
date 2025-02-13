#include "input_type.h"
#include "airSuspensionUtil.h"
#include <Wire.h>

// when using simple high and low addressing, 0x48 is low and 0x49 is high

static SemaphoreHandle_t adcReadMutex;

void setupADCReadMutex()
{
    adcReadMutex = xSemaphoreCreateMutex();
}

int voltageToESP32AnalogValue5v(float voltage)
{
    return voltage / 5.0f * 4096.0f;
}

int AnalogADCToESP32Value(Adafruit_ADS1115 *adc, int adcVal)
{
    return voltageToESP32AnalogValue5v(adc->computeVolts(adcVal));
    // return (int)(adcVal * (4096.0f / 32768.0f)); // the analog value of the adc read is not quite perfect and requires we use the built in computeVolts instead
}

int AnalogESP32ToADCValue(int espVal)
{
    return (int)(espVal * (32768.0f / 4096.0f));
}

// Function deprecated. analogReadMilliVolts returns a more accurate result than passing analogRead into this function will. keeping this function here for information on the esp's internal adc
int ESP32InternalADCAdjustValue(int readAnalogValue)
{
    // ESP32 adc is innacurate, scroll down here to the graph to read about it https://lastminuteengineers.com/esp32-basics-adc/
    // This causes our readings to be off by quite a bit especially on the further ends of the graph
    // 0.13V and less always reads 0
    // 3.2V and higher always read 4095
    // we never expect outside of 0.13 to 3.2 with the pressure sensors (accounting for voltage division, 0.33v to 2.97v, in 5v 0.5 to 4.5, so we can expect to never have an issue by expanding it out because we should never calculate a negative value )
    // stretch the graph out and plot old values on new one?
    // also graph is non linear but it's close enough so we will ignore that part at the higher end where it messes up and assume it is linear the whole way
    // basically we will now adjust a reading of 0 to attribute to 0v instead of 0.13v. This would mean that analog value of real 0.13v = (0.13v/3.3v) * 4095 = 161 instead of 0
    // similarly on the upper end, analog value of real 3.2v = (3.2v/3.3v) * 4095 = 3970.9 instead of 4096
    // so we just need to make a formula that stretches

    // x = voltage, y = analog value
    // using these to generate and visualize the formula from 2 points: https://www.desmos.com/calculator/slkjzmm3ly
    // line representing the internal ads graph is:
    // m = 1333.8762215
    // y - 0 = m (x - 0.13)
    // y = m (x - 0.13)
    // y = 1333.8762215 (x - 0.13)

    // line representing the real value we want to adjust to (perfect world reading):
    // y = 1240.90909091x

    // solve for x (voltage) where y is analog value
    // y = 1333.8762215 (x - 0.13)
    // y / 1333.8762215 = x - 0.13
    // (y / 1333.8762215) + 0.13 = x

    // plug it into the real equation
    // adjustedAnalogValue = 1240.90909091 ((readAnalogValue / 1333.8762215) + 0.13)

    return 1240.90909091f * ((readAnalogValue / 1333.8762215) + 0.13);
}

InputType::InputType()
{
    this->input_type = NORMAL;
    this->adc = nullptr;
    this->pin = -1;
}

InputType::InputType(int pin, int pinModeInputOutput)
{
    this->input_type = NORMAL;
    this->pin = pin;
    this->adc = nullptr;
    pinMode(pin, pinModeInputOutput);
}

InputType::InputType(int pin, Adafruit_ADS1115 *adc)
{
    this->input_type = ADC;
    this->pin = pin;
    this->adc = adc;
}

int InputType::digitalRead()
{
    if (this->input_type == NORMAL)
    {
        return ::digitalRead(this->pin);
    }
    else
    {
        // not implemented
        return -1;
    }
}

int InputType::analogRead(bool skipVoltageAdjustment)
{
    if (this->input_type == NORMAL)
    {
        // reading max of 3.3v (5v through the 1.5 voltage divider)
        if (skipVoltageAdjustment)
        {
        }

        // unlike analogRead, analogReadMilliVolts gives a proper reading
        return ::analogReadMilliVolts(this->pin) * 1.24090909091f; // map millivoltage to line between (0,0),(3.3,4095) to simulate analogRead
    }
    else
    {
        if (this->adc == nullptr)
        {
            Serial.println(F("Fatal error null ADC"));
            return -1;
        }
#if ADS_MOCK_BYPASS == false

        int value = -1;
        while (xSemaphoreTake(adcReadMutex, 1) != pdTRUE)
        {
            delay(1);
        }
        value = AnalogADCToESP32Value(this->adc, this->adc->readADC_SingleEnded(this->pin));
        xSemaphoreGive(adcReadMutex);

        return value;
#else
        return random(3686); // value of max psi on esp32
#endif
    }
}

void InputType::digitalWrite(int value)
{
    if (this->input_type == NORMAL)
    {
        ::digitalWrite(this->pin, value);
    }
    else
    {
        // not implemented
    }
}

void InputType::analogWrite(int value)
{
    if (this->input_type == NORMAL)
    {
        ::analogWrite(this->pin, value);
    }
    else
    {
        // not implemented
    }
}
