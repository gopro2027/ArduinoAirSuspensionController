// OASMan ESP32

#include "user_defines.h"
#include "input_type.h"
#include "solenoid.h"
#include "manifold.h"
#include "wheel.h"
#include "compressor.h"
#include "bitmaps.h"
#include "bt.h"
#include "saveData.h"
#include "airSuspensionUtil.h"
#include "screen.h"


void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  beginEEPROM();
  bt.begin(BT_NAME);

  delay(200); // wait for voltage stabilize

  Serial.println(F("Startup!"));

  setupManifold();

  #if SCREEN_ENABLED == true
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  #endif
  
  delay(20);

  wheel[WHEEL_FRONT_PASSENGER] = new Wheel(manifold->get(FRONT_PASSENGER_IN), manifold->get(FRONT_PASSENGER_OUT), pressureInputFrontPassenger, WHEEL_FRONT_PASSENGER);
  wheel[WHEEL_REAR_PASSENGER] = new Wheel(manifold->get(REAR_PASSENGER_IN), manifold->get(REAR_PASSENGER_OUT), pressureInputRearPassenger, WHEEL_REAR_PASSENGER);
  wheel[WHEEL_FRONT_DRIVER] = new Wheel(manifold->get(FRONT_DRIVER_IN), manifold->get(FRONT_DRIVER_OUT), pressureInputFrontDriver, WHEEL_FRONT_DRIVER);
  wheel[WHEEL_REAR_DRIVER] = new Wheel(manifold->get(REAR_DRIVER_IN), manifold->get(REAR_DRIVER_OUT), pressureInputRearDriver, WHEEL_REAR_DRIVER);

  compressor = new Compressor(compressorRelayPin, pressureInputTank);

  readProfile(getBaseProfile());

  readPressures();

  #if SCREEN_ENABLED == true
  drawsplashscreen();
  #endif

  #if TEST_MODE == false
    if (getRiseOnStart() == true) {
      airUp();
    }
  #endif

  Serial.println(F("Startup Complete"));
}



const int sensorreadDelay = 100; //constant integer to set the sensor read delay in milliseconds
unsigned long lastPressureReadTime = 0;
bool pause_exe = false;
void loop() {
  compressorLogic();
  bt_cmd();
  if (pause_exe == false) {
    if (millis() - lastPressureReadTime > sensorreadDelay) {
      if (!isAnyWheelActive()) {
        readPressures();
      }
      lastPressureReadTime = millis();
    }
#if SCREEN_ENABLED == true
    drawPSIReadings();
#endif

    pressureGoalRoutine();
  
  }
  
  delay(10);
}

