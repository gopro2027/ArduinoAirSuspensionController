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

#if SCREEN_MOODE == true
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#endif

BluetoothSerial bt;

Manifold *manifold;
Manifold *getManifold() {
  return manifold;
}
//InputType *manifoldSafetyWire;



Compressor *compressor;
Wheel *wheel[4];
Wheel *getWheel(int i) {
  return wheel[i];
}

void setRideHeightFrontPassenger(byte value) {
  currentProfile[WHEEL_FRONT_PASSENGER] = value;
  if (getRaiseOnPressureSet()) {
    getWheel(WHEEL_FRONT_PASSENGER)->initPressureGoal(value);
  }
}
void setRideHeightRearPassenger(byte value) {
  currentProfile[WHEEL_REAR_PASSENGER] = value;
  if (getRaiseOnPressureSet()) {
    getWheel(WHEEL_REAR_PASSENGER)->initPressureGoal(value);
  }
}
void setRideHeightFrontDriver(byte value) {
  currentProfile[WHEEL_FRONT_DRIVER] = value;
  if (getRaiseOnPressureSet()) {
    getWheel(WHEEL_FRONT_DRIVER)->initPressureGoal(value);
  }
}
void setRideHeightRearDriver(byte value) {
  currentProfile[WHEEL_REAR_DRIVER] = value;
  if (getRaiseOnPressureSet()) {
    getWheel(WHEEL_REAR_DRIVER)->initPressureGoal(value);
  }
}

#if USE_ADS == true
Adafruit_ADS1115 ADS1115A;
#if USE_2_ADS == true
Adafruit_ADS1115 ADS1115B;
#endif
void initializeADS() {
  if (!ADS1115A.begin(ADS_A_ADDRESS)) {
    Serial.println(F("Failed to initialize ADS A"));
    #if ADS_MOCK_BYPASS == false
    while (1);
    #endif
  }
  #if USE_2_ADS == true
  if (!ADS1115B.begin(ADS_B_ADDRESS)) {
    Serial.println(F("Failed to initialize ADS B"));
    #if ADS_MOCK_BYPASS == false
    while (1);
    #endif
  }
  #endif
}
#endif

void setupManifold() {
  #if USE_ADS == true
    initializeADS();
  #endif
  
  manifold = new Manifold(
              solenoidFrontPassengerInPin,
              solenoidFrontPassengerOutPin,
              solenoidRearPassengerInPin,
              solenoidRearPassengerOutPin,
              solenoidFrontDriverInPin,
              solenoidFrontDriverOutPin,
              solenoidRearDriverInPin,
              solenoidRearDriverOutPin);
}

bool isAnyWheelActive() {
  for (int i = 0; i < 4; i++) {
    if (getWheel(i)->isActive()) {
      return true;
    }
  }
  return false;
}
byte goToPerciseBitset = 0;
void setGoToPressureGoalPercise(byte wheelnum) {
  goToPerciseBitset = goToPerciseBitset | (1 << wheelnum);
}
void setNotGoToPressureGoalPercise(byte wheelnum) {
  goToPerciseBitset = goToPerciseBitset & ~(1 << wheelnum);
}
bool shouldDoPressureGoalOnWheel(byte wheelnum) {
  return (goToPerciseBitset >> wheelnum) & 1;
}
void readPressures();
bool skipPerciseSet = false; // this is like a global flag to tell it to not do percise pressure set only from the main pressure goal routine
void pressureGoalRoutine() {
  bool a = false;
  if (isAnyWheelActive()) {
     readPressures();
     a = true;
  }
  for (int i = 0; i < 4; i++) {
    getWheel(i)->pressureGoalRoutine();
  }
  if (a == false) {
    if (goToPerciseBitset != 0) {
      //Uncomment this to make it run twice for more precision
      for (byte i = 0; i < 4; i++) {
        if (shouldDoPressureGoalOnWheel(i)) {
          if (skipPerciseSet == false)
            getWheel(i)->percisionGoToPressure();
        }
      }
      //run a second time :P and also set it to not run again
      for (byte i = 0; i < 4; i++) {
        if (shouldDoPressureGoalOnWheel(i)) {
          if (skipPerciseSet == false)
            getWheel(i)->percisionGoToPressure();
          setNotGoToPressureGoalPercise(i);
        }
      }
    }
  }
}

void airUp() {
  getWheel(WHEEL_FRONT_PASSENGER)->initPressureGoal(currentProfile[WHEEL_FRONT_PASSENGER]);
  getWheel(WHEEL_REAR_PASSENGER)->initPressureGoal(currentProfile[WHEEL_REAR_PASSENGER]);
  getWheel(WHEEL_FRONT_DRIVER)->initPressureGoal(currentProfile[WHEEL_FRONT_DRIVER]);
  getWheel(WHEEL_REAR_DRIVER)->initPressureGoal(currentProfile[WHEEL_REAR_DRIVER]);
  
}

void airOut() {
  getWheel(WHEEL_FRONT_PASSENGER)->initPressureGoal(30);
  getWheel(WHEEL_REAR_PASSENGER)->initPressureGoal(30);
  getWheel(WHEEL_FRONT_DRIVER)->initPressureGoal(30);
  getWheel(WHEEL_REAR_DRIVER)->initPressureGoal(30);
  
}

void airUpRelativeToAverage(int value) {
  getWheel(WHEEL_FRONT_PASSENGER)->initPressureGoal(getWheel(WHEEL_FRONT_PASSENGER)->getPressure() + value);
  getWheel(WHEEL_REAR_PASSENGER)->initPressureGoal(getWheel(WHEEL_REAR_PASSENGER)->getPressure() + value);
  getWheel(WHEEL_FRONT_DRIVER)->initPressureGoal(getWheel(WHEEL_FRONT_DRIVER)->getPressure() + value);
  getWheel(WHEEL_REAR_DRIVER)->initPressureGoal(getWheel(WHEEL_REAR_DRIVER)->getPressure() + value);
}

void drawsplashscreen();
void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  beginEEPROM();
  bt.begin(BT_NAME);

  delay(200); // wait for voltage stabilize

  Serial.println(F("Startup!"));

  setupManifold();

  #if SCREEN_MOODE == true
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

  #if SCREEN_MOODE == true
  drawsplashscreen();
  #endif

  #if TEST_MODE == false
    if (getRiseOnStart() == true) {
      airUp();
    }
  #endif

  Serial.println(F("Startup Complete"));
}


float pressureValueTank = 0;
int getTankPressure() {
#if TANK_PRESSURE_MOCK == true
  return 200;
#else
  return pressureValueTank;
#endif
}

float readPinPressure(InputType *pin);
const int time_solenoid_movement_delta = 500;//ms
const int time_solenoid_open_time = 1;//ms
void readPressures() {
  pressureValueTank = compressor->readPressure();

  //check if any air up solenoids are open and if so, close them for reading
  bool safePressureReadAny = false;
  for (int i = 0; i < 4; i++) {
    if (getWheel(i)->prepareSafePressureRead()) {
      safePressureReadAny = true;
    }
  }

  //wait a bit of time for the solenoids to physically close
  if (safePressureReadAny) {
    for (int i = 0; i < 4; i++) {
      getWheel(i)->safePressureReadPauseClose();
    }
    delay(time_solenoid_movement_delta);
  }

  //read the pressures
  for (int i = 0; i < 4; i++) {
    getWheel(i)->readPressure();
  }

  //re-open solenoids if necessary
  for (int i = 0; i < 4; i++) {
    getWheel(i)->safePressureClose();
  }

  //give them a brief pause to stay open (not super necessary)
  if (safePressureReadAny) {
    delay(time_solenoid_open_time);
    //resume wheels after delay
    for (int i = 0; i < 4; i++) {
      getWheel(i)->safePressureReadResumeClose();
    }
  }
}

void compressorLogic() {
  if (isAnyWheelActive()) {
    compressor->pause();
  } else {
    compressor->resume();
  }
  compressor->loop();
}

const int sensorreadDelay = 100; //constant integer to set the sensor read delay in milliseconds
unsigned long lastPressureReadTime = 0;
bool pause_exe = false;
void bt_cmd();
void drawPSIReadings();
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
#if SCREEN_MOODE == true
    drawPSIReadings();
#endif

    pressureGoalRoutine();
  
  }
  
  delay(10);
}

#if SCREEN_MOODE == true
void drawPSIReadings() {
  display.clearDisplay();

  display.drawBitmap(0,0,logo_bmp_corvette, 128, 20, 1);
  
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  int textHeightPx = 10;
  int secondRowXPos = SCREEN_WIDTH/2;

  display.setCursor(0,5*textHeightPx+5);
  display.print(F("Tank: "));
  display.print(int(getTankPressure()));

//Front
  
  display.setCursor(0,2*textHeightPx+5);
  display.print(F("FD: "));
  display.print(int(getWheel(WHEEL_FRONT_DRIVER)->getPressure()));//front driver

  display.setCursor(secondRowXPos,2*textHeightPx+5);
  display.print(F("FP: "));
  display.print(int(getWheel(WHEEL_FRONT_PASSENGER)->getPressure()));//front passenger

//Rear
  display.setCursor(0,3.5*textHeightPx+5);
  display.print(F("RD: "));
  display.print(int(getWheel(WHEEL_REAR_DRIVER)->getPressure()));//rear driver

  display.setCursor(secondRowXPos,3.5*textHeightPx+5);
  display.print(F("RP: "));
  display.print(int(getWheel(WHEEL_REAR_PASSENGER)->getPressure()));//rear passenger

  display.display();
}

void drawsplashscreen() {
  for(int i = -display.height(); i <= 0; i+=2) {
    display.clearDisplay();

    display.drawBitmap(0,i,logo_bmp_airtekk, 128, 64, 1);
    display.display();
  }
  display.clearDisplay();

  display.drawBitmap(0,0,logo_bmp_airtekk, 128, 64, 1);
  display.display();
  delay(2000);//2 seconds
}
#endif

