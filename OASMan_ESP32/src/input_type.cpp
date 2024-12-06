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

float adc5vToExpected5v(float in)
{
    // although 5v is supposed to be 32768, in my testing 5v averaged out to 24954. Tested by unplugging one of the pressure sensors and sticking a wire between data and 5v and uncommenting the code below that references this function name
    float realAtZeroPSI = 339.00;
    if (getCalibration()->hasCalibrated)
    {
        // Serial.print("ADC: ");
        // Serial.println(getCalibration()->adcCalibration);
        realAtZeroPSI = getCalibration()->adcCalibration;
    }
    return in * (pressureZeroAnalogValue / realAtZeroPSI);
}

float voltageDivider5vToExpected5v(float in)
{
    float realAtZeroPSI = 307.20;
    if (getCalibration()->hasCalibrated)
    {
        // Serial.print("VD: ");
        // Serial.println(getCalibration()->voltageDividerCalibration);
        realAtZeroPSI = getCalibration()->voltageDividerCalibration;
    }
    // realAtZeroPSI = 240;
    // Serial.print("vd5te5: ");
    // Serial.print(realAtZeroPSI);
    // Serial.print("\t");
    // Serial.print(in);
    // Serial.print("\t");
    // Serial.print(pressureZeroAnalogValue / realAtZeroPSI);
    // Serial.print("\t");
    // Serial.print(in * (pressureZeroAnalogValue / realAtZeroPSI));
    // Serial.print("\t");
    return in * (pressureZeroAnalogValue / realAtZeroPSI);
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
        return voltageDivider5vToExpected5v(::analogRead(this->pin));
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