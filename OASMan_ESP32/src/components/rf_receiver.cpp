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
#define RF_DEBOUNCE_COUNT 5

void RfReceiver::loop()
{
    #ifdef RF_ENABLED_WITH_CAR_ON_DEBUG
    // if in debug mode, don't care if vehicle is on
    #else
    // if vehicle is off disable RfReceiver
    if (!isVehicleOn())
    {//TODO: wtf was I thinking when I wrote this? Pretty sure we want the opposite here. No clue what I was thinking. We should change this to !isParked() once it is implemented
        return;
    }
    #endif

    // do rf code
    if (ADS1115C_exists) {
        int a = rf_inputA->analogRead();
        int b = rf_inputB->analogRead();
        int c = rf_inputC->analogRead();
        int d = rf_inputD->analogRead();

        bool raw_state_a = a > RF_SIGNAL_DETECTION_THRESHOLD;
        bool raw_state_b = b > RF_SIGNAL_DETECTION_THRESHOLD;
        bool raw_state_c = c > RF_SIGNAL_DETECTION_THRESHOLD;
        bool raw_state_d = d > RF_SIGNAL_DETECTION_THRESHOLD;

        // Counters for consecutive true readings
        static int counter_a = 0;
        static int counter_b = 0;
        static int counter_c = 0;
        static int counter_d = 0;

        // This code is a bit verbose because I just had AI write it because I'm being lazy. If we intent to improve upon it in the future we should move out the code per each a-d into a function, or maybe make it so we can loop through. For now though not a huge issue to leave as is. I also verified the logic the ai wrote by just reading through it, appears correct ~ Tyler

        // Handle channel A
        if (raw_state_a) {
            if (counter_a < RF_DEBOUNCE_COUNT) {
                counter_a++;
                if (counter_a == RF_DEBOUNCE_COUNT) {
                    // button A pressed
                    Serial.println("Button A pressed");
                    loadProfileAirUpQuick(getrfButtonAPreset());
                }
            }
        } else {
            counter_a = 0;
        }

        // Handle channel B
        if (raw_state_b) {
            if (counter_b < RF_DEBOUNCE_COUNT) {
                counter_b++;
                if (counter_b == RF_DEBOUNCE_COUNT) {
                    // button B pressed
                    Serial.println("Button B pressed");
                    loadProfileAirUpQuick(getrfButtonBPreset());
                }
            }
        } else {
            counter_b = 0;
        }

        // Handle channel C
        if (raw_state_c) {
            if (counter_c < RF_DEBOUNCE_COUNT) {
                counter_c++;
                if (counter_c == RF_DEBOUNCE_COUNT) {
                    // button C pressed
                    Serial.println("Button C pressed");
                    loadProfileAirUpQuick(getrfButtonCPreset());
                }
            }
        } else {
            counter_c = 0;
        }

        // Handle channel D
        if (raw_state_d) {
            if (counter_d < RF_DEBOUNCE_COUNT) {
                counter_d++;
                if (counter_d == RF_DEBOUNCE_COUNT) {
                    // button D pressed
                    Serial.println("Button D pressed");
                    loadProfileAirUpQuick(getrfButtonDPreset());
                }
            }
        } else {
            counter_d = 0;
        }
    }
}

