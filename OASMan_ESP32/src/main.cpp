// OASMan ESP32

#include <user_defines.h>
#include "input_type.h"
#include "components/wheel.h"
#include "components/compressor.h"
#include "bitmaps.h"
#include "saveData.h"
#include "airSuspensionUtil.h"
#include "tasks/tasks.h"

void setup()
{
    Serial.begin(SERIAL_BAUD_RATE);
    beginSaveData();

    delay(200); // wait for voltage stabilize

    setupADCReadMutex();

    Serial.println(F("Startup!"));

    setupManifold();

#if SCREEN_ENABLED == true

#endif

    delay(20);

    wheel[WHEEL_FRONT_PASSENGER] = new Wheel(manifold->get(FRONT_PASSENGER_IN), manifold->get(FRONT_PASSENGER_OUT), pressureInputFrontPassenger, WHEEL_FRONT_PASSENGER);
    wheel[WHEEL_REAR_PASSENGER] = new Wheel(manifold->get(REAR_PASSENGER_IN), manifold->get(REAR_PASSENGER_OUT), pressureInputRearPassenger, WHEEL_REAR_PASSENGER);
    wheel[WHEEL_FRONT_DRIVER] = new Wheel(manifold->get(FRONT_DRIVER_IN), manifold->get(FRONT_DRIVER_OUT), pressureInputFrontDriver, WHEEL_FRONT_DRIVER);
    wheel[WHEEL_REAR_DRIVER] = new Wheel(manifold->get(REAR_DRIVER_IN), manifold->get(REAR_DRIVER_OUT), pressureInputRearDriver, WHEEL_REAR_DRIVER);

    compressor = new Compressor(compressorRelayPin, pressureInputTank);

    accessoryWireSetup();

    readProfile(getBaseProfile());

    setup_tasks();

#if TEST_MODE == false
    // only want to rise on start if it was a full boot and not a quick reboot
    if (getReboot() == false)
    {
        if (getRiseOnStart() == true)
        {
            airUp();
        }
    }
#endif

    setReboot(false);

    Serial.println(F("Startup Complete"));
}

void loop()
{
    accessoryWireLoop();
    if (getReboot() == true)
    {
        ESP.restart();
    }
    delay(100);
}
