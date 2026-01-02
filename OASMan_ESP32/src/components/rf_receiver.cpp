#include "rf_receiver.h"

RfReceiver::RfReceiver()
{
    if (ADS1115C_exists) {
        rf_inputA = rfInputA;
        rf_inputB = rfInputB;
        rf_inputC = rfInputC;
        rf_inputD = rfInputD;
        rf_programPin = rfProgramPin;

        rf_programPin->digitalWrite(LOW); // ensure program pin is low on init
    }
}

#define DEFAULT_SIGNAL_ON_MS 250
#define DEFAULT_SIGNAL_OFF_MS 250

void RfReceiver::sendProgramCommand(int numSignals, int msOn, int msOff) {
    if (ADS1115C_exists) {
        for (int i = 0; i < numSignals; i++) {
            rf_programPin->digitalWrite(HIGH);
            delay(msOn);
            rf_programPin->digitalWrite(LOW);
            delay(msOff);
        }
    }
}

void RfReceiver::programDelete() {
    sendProgramCommand(8, DEFAULT_SIGNAL_ON_MS, DEFAULT_SIGNAL_OFF_MS);
}
void RfReceiver::programLearnMomentary() {
    sendProgramCommand(1, DEFAULT_SIGNAL_ON_MS, DEFAULT_SIGNAL_OFF_MS);
}


void RfReceiver::loop()
{
    // if vehicle is off disable RfReceiver
    // TODO: TEMPORARILY REMOVED FOR TESTING. RE-ADD FOR PRODUCTION
    // if (!isVehicleOn())
    // {
    //     return;
    // }

    // do rf code
    if (ADS1115C_exists) {
        int a = rf_inputA->analogRead();
        int b = rf_inputB->analogRead();
        int c = rf_inputC->analogRead();
        int d = rf_inputD->analogRead();

        Serial.print("RF Readings: A:");
        Serial.print(a);
        Serial.print(" B:");
        Serial.print(b);
        Serial.print(" C:");
        Serial.print(c);
        Serial.print(" D:");
        Serial.println(d);
    }
}

