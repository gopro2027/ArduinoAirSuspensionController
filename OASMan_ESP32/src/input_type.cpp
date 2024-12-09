#include "input_type.h"
#include <Wire.h>

// when using simple high and low addressing, 0x48 is low and 0x49 is high

int AnalogADCToESP32Value(int adcVal)
{
    return (int)(adcVal * (4096.0f / 32768.0f));
}

int AnalogESP32ToADCValue(int espVal)
{
    return (int)(espVal * (32768.0f / 4096.0f));
}

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

float adc5vToExpected5v(float in)
{
    return in;
    // // although 5v is supposed to be 32768, in my testing 5v averaged out to 24954. Tested by unplugging one of the pressure sensors and sticking a wire between data and 5v and uncommenting the code below that references this function name
    // float realAtZeroPSI = 339.00;
    // if (getCalibration()->hasCalibrated)
    // {
    //     // Serial.print("ADC: ");
    //     // Serial.println(getCalibration()->adcCalibration);
    //     realAtZeroPSI = getCalibration()->adcCalibration;
    // }
    // return in * (pressureZeroAnalogValue / realAtZeroPSI);
}

float voltageDivider5vToExpected5v(float in)
{
    return in;
    // float realAtZeroPSI = 307.20;
    // if (getCalibration()->hasCalibrated)
    // {
    //     // Serial.print("VD: ");
    //     // Serial.println(getCalibration()->voltageDividerCalibration);
    //     realAtZeroPSI = getCalibration()->voltageDividerCalibration;
    // }
    // // realAtZeroPSI = 240;
    // // Serial.print("vd5te5: ");
    // // Serial.print(realAtZeroPSI);
    // // Serial.print("\t");
    // // Serial.print(in);
    // // Serial.print("\t");
    // // Serial.print(pressureZeroAnalogValue / realAtZeroPSI);
    // // Serial.print("\t");
    // // Serial.print(in * (pressureZeroAnalogValue / realAtZeroPSI));
    // // Serial.print("\t");
    // return in * (pressureZeroAnalogValue / realAtZeroPSI);
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
        // Serial.println(::analogRead(this->pin), DEC);
        // So this is read over the voltage divider! 5v = 4000
        // Should likely put in some
        if (skipVoltageAdjustment)
        {
            return ::analogRead(this->pin);
        }
        return ESP32InternalADCAdjustValue(voltageDivider5vToExpected5v(::analogRead(this->pin)));
    }
    else
    {
        if (this->adc == nullptr)
        {
            Serial.println(F("Fatal error null ADC"));
            return -1;
        }
#if ADS_MOCK_BYPASS == false

        // ads request special code, ads must be called from the other thread
        Ads_Request request;
        queueADSRead(&request, this->adc, this->pin);
        while (!request.completed)
        {
            delay(1);
        }

        if (skipVoltageAdjustment)
        {
            return AnalogADCToESP32Value(request.resultValue);
        }
        return AnalogADCToESP32Value(adc5vToExpected5v(request.resultValue));
#else
        return 3686; // value of max psi on esp32
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

#define ADS_QUEUE_SIZE 10
Ads_Request *adsQueue[ADS_QUEUE_SIZE];

int getADSQueNextOpenSlot()
{
    for (int i = 0; i < ADS_QUEUE_SIZE; i++)
    {
        if (adsQueue[i] == 0)
        {
            return i;
        }
    }
    return -1;
}
bool adsQueLock = false;
void queueADSRead(Ads_Request *request, Adafruit_ADS1115 *adc, int pin)
{
    while (adsQueLock)
    {
        delay(1);
    }
    adsQueLock = true;

    int i = getADSQueNextOpenSlot();
    while (i == -1)
    {
        delay(1);
        i = getADSQueNextOpenSlot();
    }

    request->adc = adc;
    request->pin = pin;
    request->completed = false;
    request->resultValue = -1;

    adsQueue[i] = request;

    adsQueLock = false;
}

void ADSLoop()
{
    for (int i = 0; i < ADS_QUEUE_SIZE; i++)
    {
        if (adsQueue[i] != 0)
        {
            Ads_Request *adsRequest = adsQueue[i];
            adsRequest->resultValue = adsRequest->adc->readADC_SingleEnded(adsRequest->pin);
            adsRequest->completed = true;
            adsQueue[i] = 0;
        }
    }
}