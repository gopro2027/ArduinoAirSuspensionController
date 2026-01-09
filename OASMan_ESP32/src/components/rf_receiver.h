#ifndef rf_receiver_h
#define rf_receiver_h

#include <user_defines.h>
#include "input_type.h"
#include "solenoid.h"
#include "sampleReading.tcc"
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include "manifoldSaveData.h"

extern Adafruit_ADS1115 ADS1115C;
extern bool ADS1115C_exists;

class RfReceiver
{
private:
    InputType *rf_inputA;
    InputType *rf_inputB;
    InputType *rf_inputC;
    InputType *rf_inputD;
    InputType *rf_programPin;
public:
    RfReceiver();
    void loop();
    void sendProgramCommand(int numSignals, int msOn, int msOff);
    void programDelete();
    void programLearnMomentary();
    void programLearnToggle();
    void programLearnRadioButton();
};
extern bool isVehicleOn();                                     // defined in airSuspensionUtil.h
extern void loadProfileAirUpQuick(int profileIndex);           // defined in airSuspensionUtil.h
#endif
