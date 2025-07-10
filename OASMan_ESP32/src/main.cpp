// OASMan ESP32

#include <Preferences.h> // have to include it here or it isn't found in the shared libs
#include <user_defines.h>
#include "input_type.h"
#include "components/wheel.h"
#include "components/compressor.h"
#include "bitmaps.h"
#include "manifoldSaveData.h"
#include "airSuspensionUtil.h"
#include "tasks/tasks.h"
#include <directdownload.h>

#include <SPIFFS.h>

void setupSpiffsLog();
void writeToSpiffsLog(char *text);


#pragma region bp32test

#include <Bluepad32.h>

GamepadPtr myGamepad = nullptr;

void onConnectedGamepad(GamepadPtr gp) {
    Serial.println("Gamepad connected");
    myGamepad = gp;
}

void onDisconnectedGamepad(GamepadPtr gp) {
    Serial.println("Gamepad disconnected");
    myGamepad = nullptr;
}

void bp32setup() {

    // Initialize Bluepad32
    BP32.setup(&onConnectedGamepad, &onDisconnectedGamepad);
    BP32.forgetBluetoothKeys(); // Optional: forget previous connections

}

void bp32loop() {
    // Update Bluepad32
    BP32.update();

    if (myGamepad && myGamepad->isConnected()) {
        // Read right joystick Y-axis (value between -512 to 512)
        int axisX = myGamepad->axisX();

        // Map from joystick range (-512 to 512) to servo range (0 to 180)
        int angle = map(axisX, 512, -512, 0, 180);

        // Constrain to valid servo range
        angle = constrain(angle, 0, 180);

        // Debug
        Serial.print("Joystick X: ");
        Serial.print(axisX);
        Serial.print(" -> Servo angle: ");
        Serial.println(angle);
    }

    delay(20); // Small delay for stability
}

#pragma endregion

void setup()
{
    Serial.begin(SERIAL_BAUD_RATE);
    Serial.println(F("Startup!"));

    SPIFFS.begin(true);

    beginSaveData();

    // Check if in update mode and ignore everything else and just start the web server.
    if (getupdateMode())
    {
        setupdateMode(false);
        Serial.println("Gonna try to download update");
        downloadUpdate(getwifiSSID(), getwifiPassword());
        return;
    }

    setupSpiffsLog();

    // clearPressureData();

    // trainAIModels();

    delay(200); // wait for voltage stabilize

    bp32setup();

    setupADCReadMutex();
    setupWheelLockSem();

    setupManifold();

#if SCREEN_ENABLED == true

#endif

    delay(20);

    pressureInputs[0] = pressureSensorInput0;
    pressureInputs[1] = pressureSensorInput1;
    pressureInputs[2] = pressureSensorInput2;
    pressureInputs[3] = pressureSensorInput3;
    pressureInputs[4] = pressureSensorInput4;

    wheel[WHEEL_FRONT_PASSENGER] = new Wheel(manifold->get(FRONT_PASSENGER_IN), manifold->get(FRONT_PASSENGER_OUT), pressureInputs[getpressureInputFrontPassenger()], levelInputFrontPassenger, WHEEL_FRONT_PASSENGER);
    wheel[WHEEL_REAR_PASSENGER] = new Wheel(manifold->get(REAR_PASSENGER_IN), manifold->get(REAR_PASSENGER_OUT), pressureInputs[getpressureInputRearPassenger()], levelInputRearPassenger, WHEEL_REAR_PASSENGER);
    wheel[WHEEL_FRONT_DRIVER] = new Wheel(manifold->get(FRONT_DRIVER_IN), manifold->get(FRONT_DRIVER_OUT), pressureInputs[getpressureInputFrontDriver()], levelInputFrontDriver, WHEEL_FRONT_DRIVER);
    wheel[WHEEL_REAR_DRIVER] = new Wheel(manifold->get(REAR_DRIVER_IN), manifold->get(REAR_DRIVER_OUT), pressureInputs[getpressureInputRearDriver()], levelInputRearDriver, WHEEL_REAR_DRIVER);

    compressor = new Compressor(compressorRelayPin, pressureInputs[getpressureInputTank()]);

    if (getlearnPressureSensors())
    {
        setlearnPressureSensors(false);
        PressureSensorCalibration::learnPressureSensorsRoutine();
    }

    accessoryWireSetup();

    // TODO: make base profile work (look in other spots in app for this)
    // readProfile(getbaseProfile());// TODO: add functionality for this in the controller
    readProfile(2);

    setup_tasks();

#if TEST_MODE == false
    // only want to rise on start if it was a full boot and not a quick reboot
    if (getinternalReboot() == false && getsafetyMode() == false)
    {
        if (getriseOnStart() == true)
        {
            airUp();
        }
    }
#endif

    setinternalReboot(false);

    Serial.println(F("Startup Complete"));

    // for (int i = 0; i < 200; i++) {
    //     for (int j = 0; j < 2; j++) {
    //         appendPressureDataToFile((SOLENOID_AI_INDEX)j, 0,1,2,3);
    //         delay(2);
    //     }
    //     for (int j = 2; j < 4; j++) {
    //         appendPressureDataToFile((SOLENOID_AI_INDEX)j, 2,1,3,3);
    //         delay(2);
    //     }
    // }
}

void loop()
{
    accessoryWireLoop();
    if (getinternalReboot() == true)
    {
        ESP.restart();
    }

    bp32loop();

    delay(100);
}


