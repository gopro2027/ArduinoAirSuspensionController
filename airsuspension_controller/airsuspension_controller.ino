/**************************************************************************
 This is an example for our Monochrome OLEDs based on SSD1306 drivers

 Pick one up today in the adafruit shop!
 ------> http://www.adafruit.com/category/63_98

 This example is for a 128x64 pixel display using I2C to communicate
 3 pins are required to interface (two I2C and one reset).

 Adafruit invests time and resources providing this open
 source code, please support Adafruit and open-source
 hardware by purchasing products from Adafruit!

 Written by Limor Fried/Ladyada for Adafruit Industries,
 with contributions from the open source community.
 BSD license, check license.txt for more information
 All text above, and the splash screen below must be
 included in any redistribution.
 **************************************************************************/

 //Pressure Sensor Front (passenger) to A0
 //Pressure Sensor Rear (passenger) to A1
 //Pressure Sensor Front (driver) to A2
 //Pressure Sensor Rear (driver) to A3
 //Pressure Sensor Tank to A6
 //
 //screen red to A4 (cant change)
 //screen black to A5 (cant change)
 //
 //bluetooth RX to D2
 //bluetooth TX to D3
 //button up (black) to D4
 //button down (red) to D5
 //
 //solenoid front (passenger) in to D6
 //solenoid front (passenger) out to D7
 //solenoid rear (passenger) in to D8
 //solenoid rear (passenger) out to D9

//unused yet
 //solenoid fd (driver) in to D10
 //solenoid fd (driver) out to D11
 //solenoid rd (driver) in to D12
 //solenoid rd (driver) out to D13
 

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

#include <SoftwareSerial.h> // use the software uart
SoftwareSerial bt(2, 3); // RX, TX

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define TEST_MODE true

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);



const int pressureInputFront = A0; //select the analog input pin for the pressure transducer FRONT
const int pressureInputRear = A1; //select the analog input pin for the pressure transducer REAR
const int pressureInputTank = A2; //select the analog input pin for the pressure transducer TANK
const float pressureZero = 102.4; //analog reading of pressure transducer at 0psi
const float pressureMax = 921.6; //analog reading of pressure transducer at 100psi
const int pressuretransducermaxPSI = 300; //psi value of transducer being used
int sensorreadDelay = 100; //constant integer to set the sensor read delay in milliseconds
float pressureValueFront = 0; //variable to store the value coming from the pressure transducer
float pressureValueRear = 0; //variable to store the value coming from the pressure transducer
float pressureValueTank = 0; //variable to store the value coming from the pressure transducer
unsigned long lastPressureReadTime = 0;
int maxPressureSafety = 150;
const int solenoidFrontInPin = 6;
const int solenoidFrontOutPin = 7;
const int solenoidRearInPin = 8;
const int solenoidRearOutPin = 9;

const int buttonRisePin = 4;
const int buttonFallPin = 5;


//https://www.dcode.fr/binary-image

  //regex for website output to array:
  //([0-1][0-1][0-1][0-1][0-1][0-1][0-1][0-1])
  //0b$1, 


static const unsigned char PROGMEM logo_bmp_corvette[] = 
{

0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000111, 0b11111111, 0b11100111, 0b11111111, 0b11100011, 0b11111111, 0b11110111, 0b00000000, 0b01110011, 0b11111111, 0b11110111, 0b11111111, 0b11111111, 0b11111101, 0b11111111, 0b11111000, 
0b00000111, 0b11111111, 0b11101111, 0b11111111, 0b11110011, 0b11111111, 0b11110011, 0b10000000, 0b11100111, 0b11111111, 0b11101111, 0b11111111, 0b11111111, 0b11111001, 0b11111111, 0b11111000, 
0b00001110, 0b00000000, 0b00001110, 0b00000000, 0b01110011, 0b10000000, 0b01110011, 0b10000001, 0b11000110, 0b00000000, 0b00000000, 0b00111000, 0b00000110, 0b00000011, 0b10000000, 0b00000000, 
0b00001110, 0b00000000, 0b00001100, 0b00000000, 0b01100111, 0b00000000, 0b01110011, 0b10000001, 0b10001110, 0b00000000, 0b00000000, 0b00110000, 0b00001110, 0b00000011, 0b10000000, 0b00000000, 
0b00001110, 0b00000000, 0b00011100, 0b00000000, 0b11100111, 0b01111111, 0b11110001, 0b10000011, 0b10001110, 0b11111111, 0b11000000, 0b01110000, 0b00001110, 0b00000011, 0b01111111, 0b11110000, 
0b00001100, 0b00000000, 0b00011100, 0b00000000, 0b11100111, 0b01111111, 0b11100001, 0b11000111, 0b00001101, 0b11111111, 0b11000000, 0b01110000, 0b00001110, 0b00000111, 0b01111111, 0b11110000, 
0b00011100, 0b00000000, 0b00011100, 0b00000000, 0b11100110, 0b01111110, 0b00000001, 0b11001110, 0b00001100, 0b00000000, 0b00000000, 0b01110000, 0b00001100, 0b00000111, 0b00000000, 0b00000000, 
0b00011100, 0b00000000, 0b00011000, 0b00000000, 0b11000110, 0b00011110, 0b00000001, 0b11001100, 0b00011100, 0b00000000, 0b00000000, 0b11100000, 0b00011100, 0b00000111, 0b00000000, 0b00000000, 
0b00011111, 0b11111111, 0b00011111, 0b11111111, 0b11001110, 0b00001111, 0b10000000, 0b11111100, 0b00011111, 0b11111111, 0b10000000, 0b11100000, 0b00011100, 0b00000111, 0b11111111, 0b11000000, 
0b00011111, 0b11111111, 0b10011111, 0b11111111, 0b11001110, 0b00000011, 0b11100000, 0b11111000, 0b00011111, 0b11111111, 0b10000000, 0b11100000, 0b00011100, 0b00000111, 0b11111111, 0b11100000, 
0b00001111, 0b11111111, 0b00011111, 0b11111111, 0b10001100, 0b00000001, 0b11110000, 0b11110000, 0b00001111, 0b11111111, 0b10000000, 0b11000000, 0b00011000, 0b00000111, 0b11111111, 0b11000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 

  
};
static const unsigned char PROGMEM logo_bmp_airtekk[] =
{

0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000011, 0b11000000, 0b00001111, 0b11001111, 0b11111111, 0b11111011, 0b11111111, 0b11111100, 0b01111111, 0b11111110, 0b11111110, 0b00000111, 0b11100111, 0b11110000, 0b00111111, 
0b00000000, 0b00000111, 0b11000000, 0b00001111, 0b11001111, 0b11111111, 0b11111111, 0b11111111, 0b11111101, 0b11111111, 0b11111110, 0b11111110, 0b00001111, 0b11001111, 0b11100000, 0b11111100, 
0b00000000, 0b00001111, 0b11000000, 0b00011111, 0b11011111, 0b11000111, 0b11111010, 0b01111111, 0b11001011, 0b11111100, 0b00000100, 0b11111100, 0b00111111, 0b00001111, 0b11100011, 0b11110000, 
0b00000000, 0b00011111, 0b11100000, 0b00011111, 0b10011111, 0b10000011, 0b11111000, 0b00111111, 0b00000111, 0b11111000, 0b00000001, 0b11111100, 0b11111100, 0b00001111, 0b11000111, 0b11100000, 
0b00000000, 0b00111111, 0b11100000, 0b00111111, 0b10111111, 0b10000111, 0b11110000, 0b01111111, 0b00000111, 0b11110000, 0b00000001, 0b11111101, 0b11110000, 0b00011111, 0b11011111, 0b10000000, 
0b00000000, 0b01111111, 0b11110000, 0b00111111, 0b10111111, 0b11111111, 0b11110000, 0b01111111, 0b00001111, 0b11110111, 0b11111001, 0b11111111, 0b11110000, 0b00011111, 0b11111111, 0b00000000, 
0b00000001, 0b11001111, 0b11110000, 0b00111111, 0b00111111, 0b01111111, 0b11100000, 0b01111111, 0b00001111, 0b11101111, 0b11111011, 0b11111111, 0b11110000, 0b00011111, 0b10111111, 0b10000000, 
0b00000011, 0b10001111, 0b11111000, 0b01111111, 0b01111111, 0b01111111, 0b10000000, 0b11111110, 0b00011111, 0b11100000, 0b00010011, 0b11110011, 0b11111000, 0b00111111, 0b10111111, 0b10000000, 
0b00000111, 0b00000111, 0b11111000, 0b01111111, 0b01111111, 0b00111111, 0b10000000, 0b11111110, 0b00011111, 0b11100000, 0b00000011, 0b11110011, 0b11111000, 0b00111111, 0b10011111, 0b11000000, 
0b00001110, 0b00000111, 0b11111000, 0b01111110, 0b01111110, 0b00111111, 0b10000000, 0b11111100, 0b00011111, 0b11100000, 0b00000111, 0b11110001, 0b11111100, 0b00111111, 0b00011111, 0b11100000, 
0b00011100, 0b00000111, 0b11111100, 0b11111110, 0b11111110, 0b00011111, 0b11000001, 0b11111100, 0b00011111, 0b11100000, 0b00000111, 0b11100001, 0b11111110, 0b01111111, 0b00001111, 0b11100000, 
0b00111111, 0b11110011, 0b11111100, 0b11111110, 0b11111110, 0b00011111, 0b11000001, 0b11111100, 0b00001111, 0b11111111, 0b11101111, 0b11100000, 0b11111110, 0b01111111, 0b00001111, 0b11110000, 
0b11111111, 0b11110011, 0b11111110, 0b11111100, 0b11111100, 0b00001111, 0b11100001, 0b11111000, 0b00001111, 0b11111111, 0b11101111, 0b11100000, 0b11111111, 0b01111110, 0b00000111, 0b11110000, 
0b11111111, 0b11100001, 0b11111100, 0b11111100, 0b11111100, 0b00001111, 0b11100001, 0b11111000, 0b00000011, 0b11111111, 0b11001111, 0b11000000, 0b01111111, 0b01111110, 0b00000011, 0b11110000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000001, 0b11100000, 0b00001110, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000001, 0b11111000, 0b00111110, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000001, 0b11111000, 0b01111110, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b11111110, 0b11111100, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00111111, 0b11111000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00011111, 0b11110000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00011111, 0b11000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b01111111, 0b10000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b11111111, 0b11000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00001111, 0b11111011, 0b11000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00001111, 0b11100011, 0b11000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00001111, 0b11000001, 0b11100000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00001110, 0b00000001, 0b11100000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000001, 0b11100000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b01111001, 0b11000011, 0b00000000, 0b00000011, 0b00000000, 0b11000000, 0b00000001, 0b10000000, 0b00000000, 0b00000000, 0b00000000, 0b01100000, 0b00000000, 0b00000000, 0b00011000, 0b00000000, 
0b00111001, 0b10000000, 0b00000000, 0b00000000, 0b00000001, 0b11000000, 0b00000001, 0b11000000, 0b00000000, 0b00000000, 0b00000000, 0b11100000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00011001, 0b10000000, 0b00000000, 0b00000000, 0b00000000, 0b11000000, 0b00000001, 0b11000000, 0b00000000, 0b00000000, 0b00110000, 0b01100000, 0b00000000, 0b00001000, 0b00000000, 0b00000000, 
0b00011001, 0b10000000, 0b00000000, 0b00000000, 0b00000000, 0b11000000, 0b00000011, 0b11000000, 0b00000000, 0b00000000, 0b00110000, 0b01100000, 0b00000000, 0b00011000, 0b00000000, 0b00000000, 
0b00011001, 0b00000011, 0b00001111, 0b11000011, 0b00000011, 0b11000000, 0b00000011, 0b11000000, 0b01111000, 0b00111000, 0b01111000, 0b01111000, 0b00011100, 0b00111100, 0b00111000, 0b00111000, 
0b00001111, 0b00000111, 0b00000110, 0b10000011, 0b00000111, 0b11000000, 0b00000011, 0b11000000, 0b11011000, 0b01111000, 0b00110000, 0b01111100, 0b00110110, 0b00011000, 0b00111000, 0b01111100, 
0b00001111, 0b00000011, 0b00000110, 0b10000011, 0b00000110, 0b11000000, 0b00000010, 0b01100000, 0b11111000, 0b01100000, 0b00110000, 0b01101100, 0b00111110, 0b00011000, 0b00111000, 0b01100000, 
0b00001111, 0b00000011, 0b00000111, 0b10000011, 0b00000110, 0b11000000, 0b00000111, 0b11100000, 0b11000000, 0b00111000, 0b00110000, 0b01101100, 0b00110000, 0b00011000, 0b00111000, 0b01100000, 
0b00001110, 0b00000011, 0b00000111, 0b00000011, 0b00000110, 0b11000000, 0b00000110, 0b01100000, 0b11000000, 0b00011100, 0b00110000, 0b01101100, 0b00110000, 0b00011000, 0b00111000, 0b01100000, 
0b00000110, 0b00000011, 0b00000011, 0b00000011, 0b00000111, 0b11100000, 0b00000110, 0b01110000, 0b11101000, 0b01101100, 0b00111000, 0b01101100, 0b00110110, 0b00011100, 0b00111000, 0b01101100, 
0b00000110, 0b00000111, 0b00000011, 0b00000011, 0b10000111, 0b11000000, 0b00001110, 0b01110000, 0b01111000, 0b01111000, 0b00111000, 0b11111100, 0b00011100, 0b00011100, 0b00111000, 0b00111100, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 
0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 



};


int rideHeightFrontAddr = 0;
int rideHeightRearAddr = 1;
int riseOnStartAddr = 2;

void setRideHeightFront(byte value) {
   EEPROM.write(rideHeightFrontAddr, value);
}
void setRideHeightRear(byte value) {
   EEPROM.write(rideHeightRearAddr, value);
}
void setRiseOnStart(bool value) {
   EEPROM.write(riseOnStartAddr, value);
}
byte getRideHeightFront() {
   return EEPROM.read(rideHeightFrontAddr);
}
byte getRideHeightRear() {
   return EEPROM.read(rideHeightRearAddr);
}
bool getRiseOnStart() {
   return EEPROM.read(riseOnStartAddr);
}

bool frontAiringOut = false;
bool frontAiringIn = false;
bool rearAiringOut = false;
bool rearAiringIn = false;
int frontPressureGoal = 0;
int rearPressureGoal = 0;
void setSolenoidFrontIn(bool _open) {
  frontAiringIn = _open;
  if (_open) {
    Serial.println("Starting front in");
    digitalWrite(solenoidFrontInPin, HIGH);
  } else {
    Serial.println("Stopping front in");
    digitalWrite(solenoidFrontInPin, LOW);
  }
  //true opens it
}
void setSolenoidFrontOut(bool _open) {
  frontAiringOut = _open;
  if (_open) {
    digitalWrite(solenoidFrontOutPin, HIGH);
  } else {
    digitalWrite(solenoidFrontOutPin, LOW);
  }
}
void setSolenoidRearIn(bool _open) {
  rearAiringIn = _open;
  if (_open) {
    digitalWrite(solenoidRearInPin, HIGH);
  } else {
    digitalWrite(solenoidRearInPin, LOW);
  }
}
void setSolenoidRearOut(bool _open) {
  rearAiringOut = _open;
  if (_open) {
    digitalWrite(solenoidRearOutPin, HIGH);
  } else {
    digitalWrite(solenoidRearOutPin, LOW);
  }
}

void setupSolenoidPins() {
  pinMode(solenoidFrontInPin, OUTPUT);
  pinMode(solenoidFrontOutPin, OUTPUT);
  pinMode(solenoidRearInPin, OUTPUT);
  pinMode(solenoidRearOutPin, OUTPUT);

  pinMode(buttonRisePin, INPUT);
  pinMode(buttonFallPin, INPUT);

  //pinMode(13, OUTPUT);
  //digitalWrite(13, HIGH);
}

int pressureDelta = 3;//Pressure will go to +- 3 psi to verify
unsigned long routineTimeout = 10 * 1000;//10 seconds is too long

unsigned long frontRoutineStartTime = 0;
void initPressureGoalFront(int newPressure) {
  int frontPressure = getFrontPressure();
  int pressureDif = newPressure - frontPressure;//negative if airing out, positive if airing up
  if (abs(pressureDif) <= pressureDelta) {
    //literally do nothing, it's close enough already
    return;
  } else {
    //okay we need to set the values
    frontRoutineStartTime = millis();
    frontPressureGoal = newPressure;
    if (pressureDif < 0) {
      Serial.println("Airing out front!");
      setSolenoidFrontOut(true);//start airing out
    } else {
      if (getTankPressure() > newPressure) {
        Serial.println("Airing in front!");
        setSolenoidFrontIn(true);//start airing in
      } else {
        Serial.println("not enough tank pressure (front)!");
        //don't even bother trying cuz there won't be enough pressure in the tank lol but i guess it won't hurt anything even if it did it just might act weird
      }
    }
  }
}

unsigned long rearRoutineStartTime = 0;
void initPressureGoalRear(int newPressure) {
  int rearPressure = getRearPressure();
  int pressureDif = newPressure - rearPressure;//negative if airing out, positive if airing up
  if (abs(pressureDif) <= pressureDelta) {
    //literally do nothing, it's close enough already
    return;
  } else {
    //okay we need to set the values
    rearRoutineStartTime = millis();
    rearPressureGoal = newPressure;
    if (pressureDif < 0) {
      Serial.println("Airing out rear!");
      setSolenoidRearOut(true);//start airing out
    } else {
      if (getTankPressure() > newPressure) {
        Serial.println("Airing in front!");
        setSolenoidRearIn(true);//start airing in
      } else {
        Serial.println("not enough tank pressure (front)!");
        //don't even bother trying cuz there won't be enough pressure in the tank lol but i guess it won't hurt anything even if it did it just might act weird
      }
    }
  }
}

void pressureGoalFrontRoutine() {
  int frontPressure = getFrontPressure();
  if (frontAiringIn) {
    if (frontPressure > maxPressureSafety) {
      setSolenoidFrontIn(false);
    }
    if (frontPressure >= frontPressureGoal) {
      //stop
      setSolenoidFrontIn(false);
    } else {
      if (millis() > frontRoutineStartTime + routineTimeout) {
        setSolenoidFrontIn(false);
      }
    }
  }
  if (frontAiringOut) {
    if (frontPressure <= frontPressureGoal) {
      //stop
      setSolenoidFrontOut(false);
    } else {
      if (millis() > frontRoutineStartTime + routineTimeout) {
        setSolenoidFrontOut(false);
      }
    }
  }
}

void pressureGoalRearRoutine() {
  int rearPressure = getRearPressure();
  if (rearAiringIn) {
    if (rearPressure > maxPressureSafety) {
      setSolenoidRearIn(false);
    }
    if (rearPressure >= rearPressureGoal) {
      //stop
      setSolenoidRearIn(false);
    } else {
      if (millis() > rearRoutineStartTime + routineTimeout) {
        setSolenoidRearIn(false);
      }
    }
  }
  if (rearAiringOut) {
    if (rearPressure <= rearPressureGoal) {
      //stop
      setSolenoidRearOut(false);
    } else {
      if (millis() > rearRoutineStartTime + routineTimeout) {
        setSolenoidRearOut(false);
      }
    }
  }
}

void pressureGoalRoutine() {
  if (frontAiringIn || frontAiringOut || rearAiringIn || rearAiringOut) {
    readPressures();
  }
  pressureGoalFrontRoutine();
  pressureGoalRearRoutine();
}

void airUp() {
  initPressureGoalFront(getRideHeightFront());
  initPressureGoalRear(getRideHeightRear());
}

void airOut() {
  initPressureGoalFront(10);
  initPressureGoalRear(10);
}

void setup() {
  Serial.begin(9600);

  bt.begin(9600); // start the bluetooth uart at 9600 which is its default
  delay(200); // wait for voltage stabilize

  if (TEST_MODE) {
    setRideHeightFront(90);
    setRideHeightRear(100);
    setRiseOnStart(false);
    bt.print("AT+NAMEvaair"); // place your name in here to configure the bluetooth name.
                                       // will require reboot for settings to take affect. 
    delay(3000); // wait for settings to take affect. 
  }

  setupSolenoidPins();

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  
  delay(20);
  readPressures();

  drawairtekklogo();

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.

  //testdrawchar();
  //testdrawstyles();
  //testscrolltext();
  initPressureGoalFront(100);
}

float readPressure(int pin) {
  return float((float(analogRead(pin))-pressureZero)*pressuretransducermaxPSI)/(pressureMax-pressureZero); //conversion equation to convert analog reading to psi
}

int getTankPressure() {
  if (TEST_MODE)
    return 999;
  return pressureValueTank;
}
int getFrontPressure() {
  if (TEST_MODE)
    return 50;
  return pressureValueFront;
}
int getRearPressure() {
  if (TEST_MODE)
    return 50;
  return pressureValueRear;
}

void readPressures() {
  pressureValueFront = readPressure(pressureInputFront);
  pressureValueRear = readPressure(pressureInputRear);
  pressureValueTank = readPressure(pressureInputTank);
}

void loop() {
  if (millis() - lastPressureReadTime > sensorreadDelay) {
    readPressures();
    lastPressureReadTime = millis();
  }

  drawPSIReadings();

  pressureGoalRoutine();

  readButtonInput();
  
  delay(10);

  
}

unsigned long lastButtonReadTime = 0;
void readButtonInput() {
  if (digitalRead(buttonRisePin) == HIGH && millis() > (lastButtonReadTime + 3000)) {
    lastButtonReadTime = millis();
    airUp();
  }
  if (digitalRead(buttonFallPin) == HIGH && millis() > (lastButtonReadTime + 3000)) {
    lastButtonReadTime = millis();
    airOut();
  }
}

void drawPSIReadings() {
  display.clearDisplay();

  display.drawBitmap(0,0,logo_bmp_corvette, 128, 20, 1);
  
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  int textHeightPx = 10;

  display.setCursor(0,2*textHeightPx);
  display.print(F("Front"));

  int secondRowXPos = SCREEN_WIDTH/2;
  display.setCursor(secondRowXPos,2*textHeightPx);
  display.print(F("Rear"));


  display.setCursor(0,5*textHeightPx+5);
  display.print(F("Tank: "));
  display.print(int(getTankPressure()));
  

  display.setTextSize(3);
  display.setTextColor(SSD1306_WHITE);
  
  display.setCursor(0,3*textHeightPx);
  display.print(int(getFrontPressure()));

  display.setCursor(secondRowXPos,3*textHeightPx);
  display.print(int(getRearPressure()));

  display.display();
}

void drawairtekklogo(void) {
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



/*

#include <SoftwareSerial.h> // use the software uart
SoftwareSerial bt(3, 4); // RX, TX

unsigned long previousMillis = 0;        // will store last time
const long interval = 500;           // interval at which to delay
static uint32_t tmp; // increment this

void setup() {
  pinMode(13, OUTPUT); // for LED status
  bt.begin(9600); // start the bluetooth uart at 9600 which is its default
  delay(200); // wait for voltage stabilize
  bt.print("AT+NAMEvaair"); // place your name in here to configure the bluetooth name.
                                       // will require reboot for settings to take affect. 
  delay(3000); // wait for settings to take affect. 
}

void loop() {
  if (bt.available()) { // check if anything in UART buffer
    if(bt.read() == '1'){ // did we receive this character?
       digitalWrite(13,!digitalRead(13)); // if so, toggle the onboard LED
    }
  }
  
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    bt.print(tmp++); // print this to bluetooth module
  }

}
 */
