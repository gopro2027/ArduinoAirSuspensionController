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

#include <SPIFFS.h>

void setup()
{
    Serial.begin(SERIAL_BAUD_RATE);
    Serial.println(F("Startup!"));

    SPIFFS.begin(true);
    
    beginSaveData();


    //clearPressureData();

    //trainAIModels();

    #ifdef parabolaLearn
    //  updateParabola(true, parabola_default_value, parabola_default_value, parabola_default_value);
    //  updateParabola(false, parabola_default_value, parabola_default_value, parabola_default_value);
#endif

    delay(200); // wait for voltage stabilize

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
    delay(100);
}
