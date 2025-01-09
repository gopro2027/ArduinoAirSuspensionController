#ifndef input_type_h
#define input_type_h

#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include "user_defines.h"
#include "saveData.h"

enum type
{
    NORMAL,
    ADC
};

class InputType
{
private:
    type input_type = NORMAL;
    Adafruit_ADS1115 *adc;
    int pin;

public:
    InputType();
    InputType(int pin, int pinModeInputOutput);
    InputType(int pin, Adafruit_ADS1115 *adc);
    int digitalRead();
    int analogRead(bool skipVoltageAdjustment = false);
    void digitalWrite(int value);
    void analogWrite(int value);
};

void setupADCReadMutex();

#endif
