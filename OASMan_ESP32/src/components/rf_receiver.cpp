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
void RfReceiver::programLearnToggle() {
    sendProgramCommand(2, DEFAULT_SIGNAL_ON_MS, DEFAULT_SIGNAL_OFF_MS);
}
void RfReceiver::programLearnRadioButton() {
    sendProgramCommand(3, DEFAULT_SIGNAL_ON_MS, DEFAULT_SIGNAL_OFF_MS);
}

#define RF_SIGNAL_DETECTION_THRESHOLD 3000

void RfReceiver::loop()
{
    #ifdef RF_ENABLED_WITH_CAR_ON_DEBUG
    // if in debug mode, don't care if vehicle is on
    #else
    // if vehicle is off disable RfReceiver
    if (!isVehicleOn())
    {
        return;
    }
    #endif

    static bool state_a = false;
    static bool state_b = false;
    static bool state_c = false;
    static bool state_d = false;

    // do rf code
    if (ADS1115C_exists) {
        int a = rf_inputA->analogRead();
        int b = rf_inputB->analogRead();
        int c = rf_inputC->analogRead();
        int d = rf_inputD->analogRead();

        bool new_state_a = a > RF_SIGNAL_DETECTION_THRESHOLD;
        bool new_state_b = b > RF_SIGNAL_DETECTION_THRESHOLD;
        bool new_state_c = c > RF_SIGNAL_DETECTION_THRESHOLD;
        bool new_state_d = d > RF_SIGNAL_DETECTION_THRESHOLD;

        if (new_state_a && !state_a)
        {
            // button A pressed
            Serial.println("Button A pressed");
            loadProfileAirUpQuick(getrfButtonAPreset());
        }
        if (new_state_b && !state_b)
        {
            // button B pressed
            Serial.println("Button B pressed");
            loadProfileAirUpQuick(getrfButtonBPreset());
        }
        if (new_state_c && !state_c)
        {
            // button C pressed
            Serial.println("Button C pressed");
            loadProfileAirUpQuick(getrfButtonCPreset());
        }
        if (new_state_d && !state_d)
        {
            // button D pressed
            Serial.println("Button D pressed");
            loadProfileAirUpQuick(getrfButtonDPreset());
        }

        state_a = new_state_a;
        state_b = new_state_b;
        state_c = new_state_c;
        state_d = new_state_d;

        // TODO: Implement preset calling using the saved data for which preset goes to which button
    }
}

