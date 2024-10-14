#include "input_type.h"
#include <Wire.h>

// when using simple high and low addressing, 0x48 is low and 0x49 is high

int AnalogADCToESP32Value(int adcVal) {
    return (int)(adcVal * (4096.0f/65536.0f));
}

int AnalogESP32ToADCValue(int espVal) {
    return (int)(espVal * (65536.0f/4096.0f));
}

InputType::InputType() {
    this->input_type = NORMAL;
    this->adc = nullptr;
    this->pin = -1;
}

InputType::InputType(int pin, int pinModeInputOutput) {
    this->input_type = NORMAL;
    this->pin = pin;
    this->adc = nullptr;
    pinMode(pin, pinModeInputOutput);
}

InputType::InputType(int pin, Adafruit_ADS1115 *adc) {
    this->input_type = ADC;
    this->pin = pin;
    this->adc = adc;
}

int InputType::digitalRead() {
    if (this->input_type == NORMAL) {
        return ::digitalRead(this->pin);
    } else {
        // not implemented
        return -1;
    }
}

int InputType::analogRead() {
    if (this->input_type == NORMAL) {
        return ::analogRead(this->pin);
    } else {
        return AnalogADCToESP32Value(this->adc->readADC_SingleEnded(this->pin)); // this should be correct for analog
    }
}

void InputType::digitalWrite(int value) {
    if (this->input_type == NORMAL) {
        ::digitalWrite(this->pin, value);
    } else {
        // not implemented
    }
}

void InputType::analogWrite(int value) {
    if (this->input_type == NORMAL) {
        ::analogWrite(this->pin, value);
    } else {
        // not implemented
    }
}