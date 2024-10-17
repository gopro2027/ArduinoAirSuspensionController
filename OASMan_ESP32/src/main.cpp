// OASMan ESP32

#include <Arduino.h>
#include <EEPROM.h>

#include "BluetoothSerial.h"

#include "user_defines.h"
#include "input_type.h"
#include "solenoid.h"
#include "manifold.h"
#include "wheel.h"
#include "compressor.h"
#include "bitmaps.h"

#if SCREEN_MOODE == true
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#endif

#define WHEEL_FRONT_PASSENGER 0
#define WHEEL_REAR_PASSENGER 1
#define WHEEL_FRONT_DRIVER 2
#define WHEEL_REAR_DRIVER 3

BluetoothSerial bt;
Manifold *manifold;
//InputType *manifoldSafetyWire;

struct Profile {
  byte pressure[4];
};

struct EEPROM_DATA {
  byte riseOnStart;
  byte baseProfile;
  byte raiseOnPressure;
  Profile profile[MAX_PROFILE_COUNT];
} EEPROM_DATA;
#define EEPROM_SIZE sizeof(EEPROM_DATA)

void saveEEPROM() {
  EEPROM.put(0, EEPROM_DATA);
  EEPROM.commit();
}
void beginEEPROM() {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(0, EEPROM_DATA);
}

byte currentProfile[4];

bool sendProfileBT = false;

void readProfile(byte profileIndex) {
  currentProfile[WHEEL_FRONT_PASSENGER] = EEPROM_DATA.profile[profileIndex].pressure[WHEEL_FRONT_PASSENGER];
  currentProfile[WHEEL_REAR_PASSENGER] = EEPROM_DATA.profile[profileIndex].pressure[WHEEL_REAR_PASSENGER];
  currentProfile[WHEEL_FRONT_DRIVER] = EEPROM_DATA.profile[profileIndex].pressure[WHEEL_FRONT_DRIVER];
  currentProfile[WHEEL_REAR_DRIVER] = EEPROM_DATA.profile[profileIndex].pressure[WHEEL_REAR_DRIVER];
  sendProfileBT = true;
}

void writeProfile(byte profileIndex) {

  if (currentProfile[WHEEL_FRONT_PASSENGER] != EEPROM_DATA.profile[profileIndex].pressure[WHEEL_FRONT_PASSENGER] ||
      currentProfile[WHEEL_REAR_PASSENGER] != EEPROM_DATA.profile[profileIndex].pressure[WHEEL_REAR_PASSENGER] ||
      currentProfile[WHEEL_FRONT_DRIVER] != EEPROM_DATA.profile[profileIndex].pressure[WHEEL_FRONT_DRIVER] ||
      currentProfile[WHEEL_REAR_DRIVER] != EEPROM_DATA.profile[profileIndex].pressure[WHEEL_REAR_DRIVER]) {

    EEPROM_DATA.profile[profileIndex].pressure[WHEEL_FRONT_PASSENGER] = currentProfile[WHEEL_FRONT_PASSENGER];
    EEPROM_DATA.profile[profileIndex].pressure[WHEEL_REAR_PASSENGER] = currentProfile[WHEEL_REAR_PASSENGER];
    EEPROM_DATA.profile[profileIndex].pressure[WHEEL_FRONT_DRIVER] = currentProfile[WHEEL_FRONT_DRIVER];
    EEPROM_DATA.profile[profileIndex].pressure[WHEEL_REAR_DRIVER] = currentProfile[WHEEL_REAR_DRIVER];
    saveEEPROM();
  }
}

bool getRiseOnStart() {
    return EEPROM_DATA.riseOnStart;
}
void setRiseOnStart(bool value) {
  if (getRiseOnStart() != value) {
    EEPROM_DATA.riseOnStart = value;
    saveEEPROM();
  }
}

byte getBaseProfile() {
    return EEPROM_DATA.baseProfile;
}
void setBaseProfile(byte value) {
  if (getBaseProfile() != value) {
    EEPROM_DATA.baseProfile = value;
    saveEEPROM();
  }
}

bool getRaiseOnPressureSet() {
    return EEPROM_DATA.raiseOnPressure;
}
void setRaiseOnPressureSet(bool value) {
  if (getRaiseOnPressureSet() != value) {
    EEPROM_DATA.raiseOnPressure = value;
    saveEEPROM();
  }
}

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
Adafruit_ADS1115 ADS1115A; // low ads
#if USE_2_ADS == true
Adafruit_ADS1115 ADS1115B; // high ads
#endif
void initializeADS() {
  if (!ADS1115A.begin(ADS_A_ADDRESS)) {
    Serial.println(F("Failed to initialize ADS A"));
    while (1);
  }
  #if USE_2_ADS == true
  if (!ADS1115B.begin(ADS_B_ADDRESS)) {
    Serial.println(F("Failed to initialize ADS B"));
    while (1);
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
bool skipPerciseSet = false;
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
}


float pressureValueTank = 0;
int getTankPressure() {
  return pressureValueTank;
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

void sendHeartbeat() {
  bt.print(F(PASSWORD));
  bt.print(F("PRES"));
  bt.print(int(getWheel(WHEEL_FRONT_PASSENGER)->getPressure()));
  bt.print(F("|"));
  bt.print(int(getWheel(WHEEL_REAR_PASSENGER)->getPressure()));
  bt.print(F("|"));
  bt.print(int(getWheel(WHEEL_FRONT_DRIVER)->getPressure()));
  bt.print(F("|"));
  bt.print(int(getWheel(WHEEL_REAR_DRIVER)->getPressure()));
  bt.print(F("|"));
  bt.print(int(getTankPressure()));
  bt.print(F("\n"));
}

void sendCurrentProfileData() {
  bt.print(F(PASSWORD));
  bt.print(F("PROF"));
  bt.print(int(currentProfile[WHEEL_FRONT_PASSENGER]));
  bt.print(F("|"));
  bt.print(int(currentProfile[WHEEL_REAR_PASSENGER]));
  bt.print(F("|"));
  bt.print(int(currentProfile[WHEEL_FRONT_DRIVER]));
  bt.print(F("|"));
  bt.print(int(currentProfile[WHEEL_REAR_DRIVER]));
  bt.print(F("\n"));
}

//https://www.seeedstudio.com/blog/2020/01/02/how-to-control-arduino-with-bluetooth-module-and-shields-to-get-started/

char *outString = "";
//String inString = "";
char inBuffer[30];
unsigned long lastHeartbeat = 0;
bool runInput();
void bt_cmd() {
  if (millis() - lastHeartbeat > 500) {

    if (strlen(outString) > 0) {
      bt.print(F(PASSWORD));
      bt.print(F("NOTIF"));
      bt.print(outString);
      bt.print(F("\n"));
      Serial.println(outString);
      outString = "";
    }
    else if (sendProfileBT == true) {
      sendProfileBT = false;
      sendCurrentProfileData();
    }
    else {
      sendHeartbeat();
    }
    lastHeartbeat = millis();
  } else {

  //Get input as string
  if (pause_exe == false) {
  if (bt.available()) {
    while (Serial.available() > 0) {
      Serial.read();
    }
    delay(200);
    pause_exe = true;
    return;
  }
  }
  
  while (bt.available()) {
    char c = bt.read();
    
    if (c == '\n') {
      bool valid = runInput();//execute command
      if (valid == true) {
        outString = "SUCC";
      } else {
        outString = "ERRUNK";
      }
      memset(inBuffer, 0, sizeof(inBuffer));
      pause_exe = false;//unpause
      continue;//just to skip writing out the original \n, could also be break but whatever
    }
    inBuffer[strlen(inBuffer)] = c;
    Serial.print(c);
  }
  bt.read();

  }
}

int trailingInt(const char str[]) {
  return atoi( &inBuffer[strlen_P(str)] );
}


const char _AIRUP[] PROGMEM = PASSWORD"AIRUP\0";
const char _AIROUT[] PROGMEM = PASSWORD"AIROUT\0";
const char _AIRSM[] PROGMEM = PASSWORD"AIRSM\0";
const char _SAVETOPROFILE[] PROGMEM = PASSWORD"SPROF\0";
const char _READPROFILE[] PROGMEM = PASSWORD"PROFR\0";
const char _AIRUPQUICK[] PROGMEM = PASSWORD"AUQ\0";
const char _BASEPROFILE[] PROGMEM = PASSWORD"PRBOF\0";
const char _AIRHEIGHTA[] PROGMEM = PASSWORD"AIRHEIGHTA\0";
const char _AIRHEIGHTB[] PROGMEM = PASSWORD"AIRHEIGHTB\0";
const char _AIRHEIGHTC[] PROGMEM = PASSWORD"AIRHEIGHTC\0";
const char _AIRHEIGHTD[] PROGMEM = PASSWORD"AIRHEIGHTD\0";
const char _RISEONSTART[] PROGMEM = PASSWORD"RISEONSTART\0";
const char _RAISEONPRESSURESET[] PROGMEM = PASSWORD"ROPS\0";
#if TEST_MODE == true
const char _TESTSOL[] PROGMEM = PASSWORD"TESTSOL\0";
#endif

bool comp(char *str1, const char str2[]) {

  int len1 = strlen(str1);
  int len2 = strlen_P(str2);
  if (len1 >= len2) {
    for (int i = 0; i < len2; i++) {
      char c1 = str1[i];
      char c2 = pgm_read_byte_near(str2 + i);
      if (c1 != c2) {
        return false;
      }
    }
    return true;
  }
  return false;
}

bool runInput() {
  //run input
  Serial.print(F("inBuffer: "));
  Serial.println(inBuffer);
  if (comp(inBuffer,_AIRUP)) {
    airUp();
    return true;
  }
  else if (comp(inBuffer,_AIROUT)) {
    airOut();
    return true;
  }
  else if (comp(inBuffer,_AIRSM)) {
    int value = trailingInt(_AIRSM);
    airUpRelativeToAverage(value);
    skipPerciseSet = true;//will be reset by any call to Wheel::initPressureGoal
    return true;
  }
  else if (comp(inBuffer,_SAVETOPROFILE)) {
    unsigned long profileIndex = trailingInt(_SAVETOPROFILE);
    if (profileIndex > MAX_PROFILE_COUNT) {
      return false;
    }
    writeProfile(profileIndex);
    return true;
  }
  else if (comp(inBuffer,_BASEPROFILE)) {
    unsigned long profileIndex = trailingInt(_BASEPROFILE);
    if (profileIndex > MAX_PROFILE_COUNT) {
      return false;
    }
    setBaseProfile(profileIndex);
    return true;
  }
  else if (comp(inBuffer,_READPROFILE)) {
    unsigned long profileIndex = trailingInt(_READPROFILE);
    if (profileIndex > MAX_PROFILE_COUNT) {
      return false;
    }
    readProfile(profileIndex);
    return true;
  }
  else if (comp(inBuffer,_AIRUPQUICK)) {
    unsigned long profileIndex = trailingInt(_AIRUPQUICK);
    if (profileIndex > MAX_PROFILE_COUNT) {
      return false;
    }
    //load profile then air up
    readProfile(profileIndex);
    airUp();
    skipPerciseSet = true;//will be reset by any call to Wheel::initPressureGoal
    return true;
  }
  else if (comp(inBuffer,_AIRHEIGHTA)) {
    unsigned long height = trailingInt(_AIRHEIGHTA);
    setRideHeightFrontPassenger(height);
    return true;
  }
  else if (comp(inBuffer,_AIRHEIGHTB)) {
    unsigned long height = trailingInt(_AIRHEIGHTB);
    setRideHeightRearPassenger(height);
    return true;
  }
  else if (comp(inBuffer,_AIRHEIGHTC)) {
    unsigned long height = trailingInt(_AIRHEIGHTC);
    setRideHeightFrontDriver(height);
    return true;
  }
  else if (comp(inBuffer,_AIRHEIGHTD)) {
    unsigned long height = trailingInt(_AIRHEIGHTD);
    setRideHeightRearDriver(height);
    return true;
  }
  else if (comp(inBuffer,_RISEONSTART)) {
    unsigned long ros = trailingInt(_RISEONSTART);
    if (ros == 0) {
      setRiseOnStart(false);
    } else {
      setRiseOnStart(true);
    }
    return true;
  }
  else if (comp(inBuffer,_RAISEONPRESSURESET)) {
    unsigned long rops = trailingInt(_RAISEONPRESSURESET);
    if (rops == 0) {
      setRaiseOnPressureSet(false);
    } else {
      setRaiseOnPressureSet(true);
    }
    return true;
  }
  #if TEST_MODE == true
    else if (comp(inBuffer,_TESTSOL)) {
      unsigned long pin = trailingInt(_TESTSOL);
      Serial.println(pin);
      if (pin >= 6 && pin <= 13) {
        digitalWrite(pin, HIGH);
        delay(1000);//sleep 100ms
        digitalWrite(pin, LOW);
      }
      return true;
    }
  #endif
  return false;
}
