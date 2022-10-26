#include <FastLED.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

//desk has 121 leds
#define NUM_LEDS 144
//number of led's per meter
#define LED_DENSITY 144
#define FOUR_STRIPS false

bool ledRequestShow = false;
void showLED() {
  //FastLED.show();
  ledRequestShow = true;
}

namespace MEM {
  enum MEM {
    SAVED,
    blinkMode,
    blinkCurrent,
    blinkInterval,
    rotatingLedInterval,
    primaryColorRed,
    primaryColorGreen,
    primaryColorBlue,
    secondaryColorRed,
    secondaryColorGreen,
    secondaryColorBlue,
    enableRotationBool,
    defaultMode,
    solidColorMode,
    rainbowMode,
    chaserMode,
    dotRunnerMode,
    phaserMode,
    percentage,
    SIZE
  };
};


class LEDStrip {
  private:
    int pin;
    CRGB *led;//NUM_LEDS will be the same for every strip
    int stripIndex;

    //blink variables
    bool blinkMode = false;
    bool blinkCurrent = false;
    unsigned long blinkInterval = 200;//ms
    unsigned long lastBlink = 0;
    unsigned long flickerEndTime = 0;

    //rotating led variables
    int rotatingLedIndex = 0;
    unsigned long rotatingLedInterval = 100;//ms
    unsigned long lastRotatingLedInterval = 0;

    unsigned long rotatingLedCounter = 0;//a counter that increments at the same pace as the led

    //modes
    CRGB primaryColor = CRGB(255, 0, 0); //red
    CRGB secondaryColor = CRGB(0, 0, 0); //black
    bool enableRotationBool = false;
    bool defaultMode = true;
    bool solidColorMode = false;
    bool rainbowMode = false;
    bool chaserMode = false;
    bool dotRunnerMode = false;
    bool phaserMode = false;

    int percentage = 100;//0 to 100

  public:

    int read(int addr) {
      return EEPROM.read(addr + (MEM::SIZE * stripIndex));
    }

    void save(int addr, int data) {
      EEPROM.write(addr + (MEM::SIZE * stripIndex),data);
    }

    void saveAllData() {
      save(MEM::blinkMode, blinkMode);
      save(MEM::blinkCurrent, blinkCurrent);
      save(MEM::blinkInterval, blinkInterval);
      save(MEM::rotatingLedInterval, rotatingLedInterval);
      save(MEM::primaryColorRed, primaryColor.r);
      save(MEM::primaryColorGreen, primaryColor.g);
      save(MEM::primaryColorBlue, primaryColor.b);
      save(MEM::secondaryColorRed, secondaryColor.r);
      save(MEM::secondaryColorGreen, secondaryColor.g);
      save(MEM::secondaryColorBlue, secondaryColor.b);
      save(MEM::enableRotationBool, enableRotationBool);
      save(MEM::defaultMode, defaultMode);
      save(MEM::solidColorMode, solidColorMode);
      save(MEM::rainbowMode, rainbowMode);
      save(MEM::chaserMode, chaserMode);
      save(MEM::dotRunnerMode, dotRunnerMode);
      save(MEM::phaserMode, phaserMode);
      save(MEM::percentage, percentage);
    }

    void loadAllData() {
      blinkMode = read(MEM::blinkMode);
      blinkCurrent = read(MEM::blinkCurrent);
      blinkInterval = read(MEM::blinkInterval);
      rotatingLedInterval = read(MEM::rotatingLedInterval);
      primaryColor = CRGB(read(MEM::primaryColorRed),read(MEM::primaryColorGreen),read(MEM::primaryColorBlue));
      secondaryColor = CRGB(read(MEM::secondaryColorRed),read(MEM::secondaryColorGreen),read(MEM::secondaryColorBlue));
      enableRotationBool = read(MEM::enableRotationBool);
      defaultMode = read(MEM::defaultMode);
      solidColorMode = read(MEM::solidColorMode);
      rainbowMode = read(MEM::rainbowMode);
      chaserMode = read(MEM::chaserMode);
      dotRunnerMode = read(MEM::dotRunnerMode);
      phaserMode = read(MEM::phaserMode);
      percentage = read(MEM::percentage);
    }
  
    LEDStrip(int _pin, CRGB *_led, int _stripIndex) : pin(_pin), led(_led), stripIndex(_stripIndex) {
    }

    void begin() {
      if (read(MEM::SAVED) != 1) {
        //save initial data
        saveAllData();
        save(MEM::SAVED,1);
      } else {
        loadAllData();
      }
      
      for (int i = 0; i < NUM_LEDS; i++) {
        led[i % NUM_LEDS] = primaryColor; //CRGB(255, 0, 0); //set all to red to begin
      }
      FastLED.setBrightness(100);
      showLED();
    }

    void setPercentage(int prct) {
      percentage = prct;
    }


    void setFade(int basePixel, int pixelStart, int pixelEnd, int r1, int g1, int b1, int r2, int g2, int b2, bool reversed) {
      int ledCount = pixelEnd - pixelStart;
      float fade_r_dif = (float)(r1 - r2) / (float)(ledCount - 1); //you must subtract 1 because the first one is the first value, then the next 5 converge to the new value
      float fade_g_dif = (float)(g1 - g2) / (float)(ledCount - 1);
      float fade_b_dif = (float)(b1 - b2) / (float)(ledCount - 1);
      
      for (int i = 0; i < ledCount; i++) {
        //r1 + i * fade_r_dif, g1 + i * fade_g_dif, b1 + i * fade_b_dif
        int r = r1 - i * fade_r_dif;
        int g = g1 - i * fade_g_dif;
        int b = b1 - i * fade_b_dif;

        int i2 = i;
        if (reversed) {//this handles mirroring so the fade is in the correct direction
          i2 = ledCount - 1 - i;
        }
        
        int index = ((pixelStart + i2) % ledCount);
        
        led[index + basePixel] = CRGB(r, g, b);
      }
      
      showLED();
    }

    

    //endinred should be set to false in situations like where the pixel immediately after 'pixelEnd' is 'pixelStart', aka it's looped, and we don't want the second red at the end
    void setRainbow(int pixelStart, int pixelEnd, bool endInRed) {
      int len = pixelEnd - pixelStart;
      for (int i = 0; i < len/2; i++) {
        //CRGB val = RainbowToCRGB((float)i/(len-(endInRed?1:0)));
        int index = (i + pixelStart) % NUM_LEDS;
        led[index] = CHSV((float)i / (len - (endInRed ? 1 : 0)) * 255, 255, 255); //val;
        led[firstHalfToSecondHalf(index)] = led[index];
      }
      showLED();
    }

    bool lastWasOff = false;
    void doFlickerFunction() {

      if (lastWasOff) {
        for (int i = 0; i < NUM_LEDS; i++) {
          led[i % NUM_LEDS] = CRGB(0, 0, 0);
        }
      }

      if (millis() % 30 != 0)
        return;

      int r = random(0, 2);
      if (r == 1) {
        lastWasOff = false;
        //FastLED.setBrightness(100);
        /*for (int i = 0; i < NUM_LEDS; i++) {
          led[i % NUM_LEDS] = primaryColor;
          }*/
        println("on");
      } else {
        lastWasOff = true;
        //FastLED.setBrightness(0);
        for (int i = 0; i < NUM_LEDS; i++) {
          led[i % NUM_LEDS] = CRGB(0, 0, 0);
        }
        println("off");
      }
      showLED();
    }


    void blinkLoop() {
      if (blinkMode == true) {
        if (millis() - lastBlink > blinkInterval) {
          blinkCurrent = !blinkCurrent;
          lastBlink = millis();
          if (blinkCurrent) {
            FastLED.setBrightness(0);
            showLED();
          } else {
            FastLED.setBrightness(100);
            showLED();
          }
        }
      } else {
        if (blinkCurrent == true) {
          blinkCurrent = false;
          FastLED.setBrightness(100);
          showLED();
        }
      }
    }

    void enableBlink() {
      FastLED.setBrightness(100);
      showLED();
      blinkMode = true;
    }
    void disableBlink() {
      FastLED.setBrightness(100);
      showLED();
      blinkMode = false;
    }
    void toggleBlink() {
      if (blinkMode == false) {
        enableBlink();
      } else {
        disableBlink();
      }
    }

    void setBlinkSpeed(int ms) {
      blinkInterval = ms;//default 200
    }

    void rotatingLedLoop() {
      if (millis() - lastRotatingLedInterval > rotatingLedInterval) {
        lastRotatingLedInterval = millis();
        rotatingLedIndex++;
        rotatingLedCounter++;
        if (rotatingLedIndex >= NUM_LEDS) {
          rotatingLedIndex = 0;
        }
      }
    }

    void enableRotation() {
      enableRotationBool = true;
    }
    void disableRotation() {
      enableRotationBool = false;
    }
    void toggleRotation() {
      enableRotationBool = !enableRotationBool;
    }
    void setRotationSpeed(float movementInMetersPerSec) {

      float ledDistInMeter = 1.0f/(float)LED_DENSITY;//Unit: Meter
      //float oneSec = 1000.0f;//Unit: Millisec
      //movementinmps = ledDistInMeter/x
      //x*mimps = ledist
      //x = ledist/mimps
      
      rotatingLedInterval = ledDistInMeter/movementInMetersPerSec   * 1000;//default 100
    }

    void setIndividualDot(int pixel, int r, int g, int b, int br, int bg, int bb) {
      for (int i = 0; i < NUM_LEDS; i++) {
        led[i % NUM_LEDS] = CRGB(br, bg, bb);
      }
      led[pixel % NUM_LEDS] = CRGB(r, g, b);
      //FastLED.setBrightness(100);
      showLED();
    }


    void disableModes() {
      defaultMode = false;
      solidColorMode = false;
      rainbowMode = false;
      chaserMode = false;
      dotRunnerMode = false;
      phaserMode = false;
    }

    void enableDefaultMode() {
      disableModes();
      disableBlink();//if blink is enabled, disable it
      //FastLED.setBrightness(100);
      //for (int i = 0; i < NUM_LEDS; i++) {
      //  led[i % NUM_LEDS] = CRGB(255, 0, 0); //set all to red to begin
      //}
      //showLED();
      defaultMode = true;
    }

    void enableRainbowMode() {
      disableModes();
      rainbowMode = true;
    }
    void enableDotRunnerMode() {
      disableModes();
      dotRunnerMode = true;
    }
    void enableChaserMode() {
      disableModes();
      chaserMode = true;
    }
    void enablePhaserMode() {
      disableModes();
      phaserMode = true;
    }
    void enableSolidColorMode() {
      disableModes();
      solidColorMode = true;
    }


    void setPrimaryColor(int r, int g, int b) {
      primaryColor = CRGB(r, g, b);
    }
    void setSecondaryColor(int r, int g, int b) {
      secondaryColor = CRGB(r, g, b);
    }

    void setPrimaryColor(CRGB c) {
      primaryColor = c;
    }
    void setSecondaryColor(CRGB c) {
      secondaryColor = c;
    }

    void flicker(unsigned long timeMS) {
      if (flickerEndTime > millis()) {
        flickerEndTime = 0;//set it to end in 100 ms
      } else {
        flickerEndTime = millis() + timeMS;
      }
    }

    bool isFlickerEnabled() {
      if (millis() < flickerEndTime) {
        return true;
      }
      return false;
    }

    int firstHalfToSecondHalf(int firstHalf) {
      return NUM_LEDS - firstHalf - 1;
    }


    void doPhaserMode() {
      
      //setFade(0 + rotatingLedIndex, NUM_LEDS + rotatingLedIndex, primaryColor.r, primaryColor.g, primaryColor.b, secondaryColor.r, secondaryColor.g, secondaryColor.b);
      //BOUNCING AFFECT! int normalizedHalfLed = (NUM_LEDS/2) - abs((NUM_LEDS/2) - rotatingLedIndex);//will give a number between 0 and (NUM_LEDS/2), when rotatingLedIndex is 0 we get 0
      
      int ledLength = (percentage/100.0f) * (NUM_LEDS/2);
      for (int i = 0; i < (NUM_LEDS/2); i++) {
        int index = i + rotatingLedCounter;
        CRGB col = (index / ledLength) % 2 == 0 ? primaryColor : secondaryColor;
        int i2 = i % (NUM_LEDS/2);
        led[i2] = col;
        led[firstHalfToSecondHalf(i2)] = col;
      }

      showLED();
      
    }


    void loop() {



      if (phaserMode || dotRunnerMode || enableRotationBool) {
        rotatingLedLoop();
      }
      blinkLoop();
      if (defaultMode == true) {
        /*for (int i = 0; i < NUM_LEDS; i++) {
          led[i % NUM_LEDS] = CRGB(255, 0, 0); //set all to red to begin
        }*/
        FastLED.setBrightness(0);
        showLED();
      }
      if (solidColorMode == true) {
        for (int i = 0; i < NUM_LEDS; i++) {
          led[i % NUM_LEDS] = primaryColor;
        }
        showLED();
      }
      if (rainbowMode == true) {
        setRainbow(0 + rotatingLedIndex, NUM_LEDS + rotatingLedIndex, false);
      }
      if (chaserMode == true) {
        setFade(0         , 0 + rotatingLedIndex, NUM_LEDS/2 + rotatingLedIndex, primaryColor.r, primaryColor.g, primaryColor.b, secondaryColor.r, secondaryColor.g, secondaryColor.b, false);
        setFade(NUM_LEDS/2, 0 + ((NUM_LEDS-1)-rotatingLedIndex), NUM_LEDS/2 + ((NUM_LEDS-1)-rotatingLedIndex), primaryColor.r, primaryColor.g, primaryColor.b, secondaryColor.r, secondaryColor.g, secondaryColor.b, true);
        //setFade(NUM_LEDS/2, 0 + rotatingLedIndex, NUM_LEDS/2 + rotatingLedIndex, primaryColor.r, primaryColor.g, primaryColor.b, secondaryColor.r, secondaryColor.g, secondaryColor.b,true);
      }
      if (dotRunnerMode == true) {
        setIndividualDot(rotatingLedIndex, primaryColor.r, primaryColor.g, primaryColor.b, secondaryColor.r, secondaryColor.g, secondaryColor.b);
      }

      if (phaserMode == true) {
        doPhaserMode();
      }

      if (isFlickerEnabled()) {
        doFlickerFunction();
      }

      

      
      
    }

};

/*

  //mode list
  enableDefaultMode();
  enableSolidColorMode();
  enableRainbowMode();
  enableDotRunnerMode();
  enableChaserMode();
  flicker(int timeMS);//will do white colored flicker for that many ms few ms

  //mode modifiers
  enableRotation();
  disableRotation();
  toggleRotation();
  setRotationSpeed(int ms);//default 100ms
  enableBlink();
  disableBlink();
  toggleBlink();
  setBlinkSpeed(int ms);//default 200ms

  //mode color modifiers
  setPrimaryColor(int r, int g, int b);//default red
  setSecondaryColor(int r, int g, int b);//default black

  setPercentage(int prct);//0 to 100

  saveAllData();

*/

LEDStrip *strip1;
#if FOUR_STRIPS == true
LEDStrip *strip2;
LEDStrip *strip3;
LEDStrip *strip4;
#endif

#define TX_PIN 2
#define RX_PIN 3

#define LED1_PIN 9
#if FOUR_STRIPS == true
#define LED2_PIN 10
#define LED3_PIN 11
#define LED4_PIN 12
#endif

CRGB led1[NUM_LEDS];
#if FOUR_STRIPS == true
CRGB led2[NUM_LEDS];
CRGB led3[NUM_LEDS];
CRGB led4[NUM_LEDS];
#endif


SoftwareSerial bt (TX_PIN, RX_PIN); // RX, TX
//#define bt Serial

void setup() {

  //pinMode(3,INPUT);

  Serial.begin(115200);//set up com for debug output


  FastLED.addLeds<NEOPIXEL, LED1_PIN>(led1, NUM_LEDS);
  #if FOUR_STRIPS == true
  FastLED.addLeds<NEOPIXEL, LED2_PIN>(led2, NUM_LEDS);
  FastLED.addLeds<NEOPIXEL, LED3_PIN>(led3, NUM_LEDS);
  FastLED.addLeds<NEOPIXEL, LED4_PIN>(led4, NUM_LEDS);
  #endif

  strip1 = new LEDStrip(LED1_PIN, led1, 0);
  #if FOUR_STRIPS == true
  strip2 = new LEDStrip(LED2_PIN, led2, 1);
  strip3 = new LEDStrip(LED3_PIN, led3, 2);
  strip4 = new LEDStrip(LED4_PIN, led4, 3);
  #endif

  strip1->begin();
  #if FOUR_STRIPS == true
  strip2->begin();
  strip3->begin();
  strip4->begin();
  #endif

  //some testing modes here
  /*strip1->enableRainbowMode();
    strip2->enableDefaultMode();
    strip3->enableChaserMode();
    strip4->enableDotRunnerMode();*/


  //bluetooth stuff
  bt.begin(38400);//arduino baud rate
  //while (!Serial) {}
  //bt.println("");
  //bt.println("Ready!");
  //bt.print("?: ");

}

bool pauseLED = false;

void loop() {
  cmd_prompt();
  if (pauseLED == false) {
    strip1->loop();
    #if FOUR_STRIPS == true
    strip2->loop();
    strip3->loop();
    strip4->loop();
    #endif
  }
}

#define PASSWORD "STNGxcBHT7bcQxeQ2U4Y"

//print to COM
void println(String str) {
  Serial.println(str);
}

void doLedShow() {
  if (ledRequestShow && pauseLED == false) {
    //Serial.println("Omg");
    ledRequestShow = false;
    FastLED.show();
    while (Serial.available() > 0) {
      Serial.read();
    }
  }
}

CRGB hexToCRGB(String hex) {
  char buf[2];
  hex.substring(0, 2).toCharArray(buf, 2);
  int r = strtol(buf, 0, 16);
  hex.substring(2, 4).toCharArray(buf, 2);
  int g = strtol(buf, 0, 16);
  hex.substring(4, 6).toCharArray(buf, 2);
  int b = strtol(buf, 0, 16);
  return CRGB(r, g, b);
}

String inString = "";
void cmd_prompt() {

  //Get input as string
  if (bt.available() && pauseLED == false) {
    while (Serial.available() > 0) {
      Serial.read();
    }
    delay(200);
    bt.println("OKAY1234");
    pauseLED = true;
    //delay(1000);
    return;
  }
  while (bt.available()) {
    //delay(30);  //delay to allow byte to arrive in input buffer
    char c = bt.read();
    Serial.print(c);
    if (c == '\n') {
      //bt.println();//make new line cuz normal \n doesn't work sometimes
      runInput();//execute command
      inString = "";
      pauseLED = false;//unpause the leds
      //bt.print("?: ");//await next command
      continue;//just to skip writing out the original \n, could also be break but whatever
    }
    //bt.write(c);//echo character back to them
    inString += c;
    //delay(30);

  }
  bt.read();
  /*if (bt.overflow()) {
    println("overflow!");
    }*/
  //End get input as string
  doLedShow();
}

void runInput() {
  FastLED.setBrightness(100);
  //run input
  if (inString.indexOf("ON") != -1) {
    //digitalWrite(2, HIGH);
    //bt.println("Turned LED On!");
    println("led on!");
  }
  if (inString.indexOf("OFF") != -1) {
    //digitalWrite(2, LOW);
    //bt.println("Turned LED Off!");
    println("led off!");
  }
  if (inString.indexOf(PASSWORD"DEFAULT") != -1) {
    //digitalWrite(PIN_UNLOCK, HIGH);
    //delay(1000);
    //digitalWrite(PIN_UNLOCK, LOW);
    //bt.println("Unlocked!");
    strip1->enableDefaultMode();
    #if FOUR_STRIPS == true
    strip2->enableDefaultMode();
    strip3->enableDefaultMode();
    strip4->enableDefaultMode();
    #endif
    println("default");
  }
  if (inString.indexOf(PASSWORD"SOLID") != -1) {
    //digitalWrite(PIN_LOCK, HIGH);
    //delay(1000);
    //digitalWrite(PIN_LOCK, LOW);
    //bt.println("Locked!");
    strip1->enableSolidColorMode();
    #if FOUR_STRIPS == true
    strip2->enableSolidColorMode();
    strip3->enableSolidColorMode();
    strip4->enableSolidColorMode();
    #endif
    println("solid color");
  }
  if (inString.indexOf(PASSWORD"DOTRUNNER") != -1) {
    //digitalWrite(PIN_HATCH, HIGH);
    //delay(1000);
    //digitalWrite(PIN_HATCH, LOW);
    //bt.println("Hatch!");
    strip1->enableDotRunnerMode();
    #if FOUR_STRIPS == true
    strip2->enableDotRunnerMode();
    strip3->enableDotRunnerMode();
    strip4->enableDotRunnerMode();
    #endif
    println("dotrunner");
  }
  if (inString.indexOf(PASSWORD"RAINBOW") != -1) {
    //digitalWrite(PIN_ALARM, HIGH);
    //delay(1000);
    //digitalWrite(PIN_ALARM, LOW);
    //bt.println("Alarm!");
    strip1->enableRainbowMode();
    #if FOUR_STRIPS == true
    strip2->enableRainbowMode();
    strip3->enableRainbowMode();
    strip4->enableRainbowMode();
    #endif
    println("Rainbow");
  }
  if (inString.indexOf(PASSWORD"CHASER") != -1) {
    strip1->enableChaserMode();
    #if FOUR_STRIPS == true
    strip2->enableChaserMode();
    strip3->enableChaserMode();
    strip4->enableChaserMode();
    #endif
    println("Chaser");
  }
  if (inString.indexOf(PASSWORD"PHASER") != -1) {
    strip1->enablePhaserMode();
    #if FOUR_STRIPS == true
    strip2->enablePhaserMode();
    strip3->enablePhaserMode();
    strip4->enablePhaserMode();
    #endif
    println("Phaser");
  }
  if (inString.indexOf(PASSWORD"BLINK") != -1) {
    strip1->toggleBlink();
    #if FOUR_STRIPS == true
    strip2->toggleBlink();
    strip3->toggleBlink();
    strip4->toggleBlink();
    #endif
    println("Blink");
  }
  if (inString.indexOf(PASSWORD"ROTATION") != -1) {
    strip1->toggleRotation();
    #if FOUR_STRIPS == true
    strip2->toggleRotation();
    strip3->toggleRotation();
    strip4->toggleRotation();
    #endif
    println("Rotation");
  }
  if (inString.indexOf(PASSWORD"PCOLOR") != -1) {
    println(inString.substring(inString.length() - 8, inString.length() - 2));
    CRGB c = hexToCRGB(inString.substring(inString.length() - 8, inString.length() - 2));
    strip1->setPrimaryColor(c);
    #if FOUR_STRIPS == true
    strip2->setPrimaryColor(c);
    strip3->setPrimaryColor(c);
    strip4->setPrimaryColor(c);
    #endif
  }
  if (inString.indexOf(PASSWORD"SCOLOR") != -1) {
    println(inString.substring(inString.length() - 8, inString.length() - 2));
    CRGB c = hexToCRGB(inString.substring(inString.length() - 8, inString.length() - 2));
    strip1->setSecondaryColor(c);
    #if FOUR_STRIPS == true
    strip2->setSecondaryColor(c);
    strip3->setSecondaryColor(c);
    strip4->setSecondaryColor(c);
    #endif
  }
  if (inString.indexOf(PASSWORD"FLICKER") != -1) {
    //unsigned long flickerTime = inString.substring(inString.indexOf(PASSWORD"FLICKER") + strlen(PASSWORD"FLICKER")).toInt() * 100;
    //this is for unlimited flicker time
    unsigned long flickerTime = 1000L * 60L * 60L; //1 hour should be plenty lol
    //println(String(flickerTime) + "ayoooooooooooo");
    strip1->flicker(flickerTime);
    #if FOUR_STRIPS == true
    strip2->flicker(flickerTime);
    strip3->flicker(flickerTime);
    strip4->flicker(flickerTime);
    #endif
  }
  if (inString.indexOf(PASSWORD"PRCNT") != -1) {
    unsigned long percentage = inString.substring(inString.indexOf(PASSWORD"PRCNT") + strlen(PASSWORD"PRCNT")).toInt();
    println(String(percentage) + " percent");
    strip1->setPercentage(percentage);
    #if FOUR_STRIPS == true
    strip2->setPercentage(percentage);
    strip3->setPercentage(percentage);
    strip4->setPercentage(percentage);
    #endif
  }
  if (inString.indexOf(PASSWORD"SPD") != -1) {

    //TODO: Change speed from generic MS to make it how far the selectedLed moves in meters per sec based on the led density
    
    float rotSpeed = inString.substring(inString.indexOf(PASSWORD"SPD") + strlen(PASSWORD"SPD")).toFloat();//1 to 10 with 1 being the slowest
    //rotSpeed = (rotSpeed) * 20;
    println(String(rotSpeed) + " speed");
    strip1->setRotationSpeed(rotSpeed);
    #if FOUR_STRIPS == true
    strip2->setRotationSpeed(rotSpeed);
    strip3->setRotationSpeed(rotSpeed);
    strip4->setRotationSpeed(rotSpeed);
    #endif
  }
  /*if (inString.indexOf("AT") == 0) {
    bt.print(inString);
    delay(1000);
    bt.println("Ran command");
    }*/

  strip1->saveAllData();
  #if FOUR_STRIPS == true
  strip2->saveAllData();
  strip3->saveAllData();
  strip4->saveAllData();
  #endif
}












/*

   Input reading code, if I ever need it again


  int reading = digitalRead(3);
  Serial.print(reading);
  Serial.print(" 3 ");
  if (reading == HIGH) {
   Serial.println("HIGH");
   FastLED.setBrightness(100);
   showLED();
  } else {
   Serial.println("LOW");
   FastLED.setBrightness(0);
   showLED();
  }

  reading = digitalRead(4);
  Serial.print(reading);
  Serial.print(" 4 ");
  if (reading == HIGH) {
   Serial.println("HIGH");
   //FastLED.setBrightness(100);
   //showLED();
  } else {
   Serial.println("LOW");
   //FastLED.setBrightness(0);
   //showLED();
  }

*/