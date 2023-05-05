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

 //solenoid front (driver) in to D10
 //solenoid front (driver) out to D11
 //solenoid rear (driver) in to D12
 //solenoid rear (driver) out to D13
 

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

#include <SoftwareSerial.h> // use the software uart

#include "solenoid.h"
#include "wheel.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define TEST_MODE false
#define SCREEN_MOODE true

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#if SCREEN_MOODE == true
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#endif

#define WHEEL_FRONT_PASSENGER 0
#define WHEEL_REAR_PASSENGER 1
#define WHEEL_FRONT_DRIVER 2
#define WHEEL_REAR_DRIVER 3

#define MAX_PROFILE_COUNT 4

//Digital pins
SoftwareSerial bt(3, 2); // RX, TX
const int buttonRisePin = 4;
const int buttonFallPin = 5;
#define solenoidFrontPassengerInPin 6
#define solenoidFrontPassengerOutPin 8
#define solenoidRearPassengerInPin 7
#define solenoidRearPassengerOutPin 10
#define solenoidFrontDriverInPin 9
#define solenoidFrontDriverOutPin 12
#define solenoidRearDriverInPin 11
#define solenoidRearDriverOutPin 13

//Analog pins
const int pressureInputFrontPassenger = A0; //select the analog input pin for the pressure transducer FRONT
const int pressureInputRearPassenger = A1; //select the analog input pin for the pressure transducer REAR
const int pressureInputFrontDriver = A2; //select the analog input pin for the pressure transducer FRONT
const int pressureInputRearDriver = A3; //select the analog input pin for the pressure transducer REAR
//A4 and A5 are the screen
const int pressureInputTank = A6; //select the analog input pin for the pressure transducer TANK

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

//const int rideHeightFrontPassengerAddr = 0;
//const int rideHeightRearPassengerAddr = 1;
//const int rideHeightFrontDriverAddr = 2;
//const int rideHeightRearDriverAddr = 3;
const int riseOnStartAddr = 0;
const int baseProfileAddr = 1;
const int raiseOnPressureAddr = 2;
#define profileStartAddress 3

byte currentProfile[4];
/*
#define WHEEL_FRONT_PASSENGER 0
#define WHEEL_REAR_PASSENGER 1
#define WHEEL_FRONT_DRIVER 2
#define WHEEL_REAR_DRIVER 3
*/

bool sendProfileBT = false;

void readProfile(byte profileIndex) {
  byte base = profileStartAddress + (4 * profileIndex);
  currentProfile[WHEEL_FRONT_PASSENGER] = EEPROM.read(base + WHEEL_FRONT_PASSENGER);
  currentProfile[WHEEL_REAR_PASSENGER] = EEPROM.read(base + WHEEL_REAR_PASSENGER);
  currentProfile[WHEEL_FRONT_DRIVER] = EEPROM.read(base + WHEEL_FRONT_DRIVER);
  currentProfile[WHEEL_REAR_DRIVER] = EEPROM.read(base + WHEEL_REAR_DRIVER);

  sendProfileBT = true;
}

void writeWithCheck(byte addr, byte val) {
  if (EEPROM.read(addr) != val) {
    EEPROM.write(addr, val);
  }
}

void writeProfile(byte profileIndex) {
  byte base = profileStartAddress + (4 * profileIndex);
  writeWithCheck(base + WHEEL_FRONT_PASSENGER, currentProfile[WHEEL_FRONT_PASSENGER]);
  writeWithCheck(base + WHEEL_REAR_PASSENGER, currentProfile[WHEEL_REAR_PASSENGER]);
  writeWithCheck(base + WHEEL_FRONT_DRIVER, currentProfile[WHEEL_FRONT_DRIVER]);
  writeWithCheck(base + WHEEL_REAR_DRIVER, currentProfile[WHEEL_REAR_DRIVER]);
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

void setRiseOnStart(bool value) {
  if (getRiseOnStart() != value)
   EEPROM.write(riseOnStartAddr, value);
}
bool getRiseOnStart() {
   return EEPROM.read(riseOnStartAddr);
}

void setBaseProfile(byte value) {
  if (getBaseProfile() != value)
   EEPROM.write(baseProfileAddr, value);
}
byte getBaseProfile() {
   return EEPROM.read(baseProfileAddr);
}

void setRaiseOnPressureSet(bool value) {
  if (getRaiseOnPressureSet() != value)
   EEPROM.write(raiseOnPressureAddr, value);
}
bool getRaiseOnPressureSet() {
   return EEPROM.read(raiseOnPressureAddr);
}


int frontPressureGoal = 0;
int rearPressureGoal = 0;

void setupSolenoidPins() {

  pinMode(solenoidFrontPassengerInPin, OUTPUT);
  pinMode(solenoidFrontPassengerOutPin, OUTPUT);
  pinMode(solenoidRearPassengerInPin, OUTPUT);
  pinMode(solenoidRearPassengerOutPin, OUTPUT);

  pinMode(solenoidFrontDriverInPin, OUTPUT);
  pinMode(solenoidFrontDriverOutPin, OUTPUT);
  pinMode(solenoidRearDriverInPin, OUTPUT);
  pinMode(solenoidRearDriverOutPin, OUTPUT);

  //pinMode(buttonRisePin, INPUT);
  //pinMode(buttonFallPin, INPUT);

  //pinMode(13, OUTPUT);
  //digitalWrite(13, HIGH);
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
      for (byte i = 0; i < 4; i++) {
        if (shouldDoPressureGoalOnWheel(i)) {
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

  getWheel(WHEEL_FRONT_PASSENGER)->initPressureGoal(0);
  getWheel(WHEEL_REAR_PASSENGER)->initPressureGoal(0);
  getWheel(WHEEL_FRONT_DRIVER)->initPressureGoal(0);
  getWheel(WHEEL_REAR_DRIVER)->initPressureGoal(0);
  
}

void airUpRelativeToAverage(int value) {
  
  getWheel(WHEEL_FRONT_PASSENGER)->initPressureGoal(getWheel(WHEEL_FRONT_PASSENGER)->getPressureAverage() + value);
  getWheel(WHEEL_REAR_PASSENGER)->initPressureGoal(getWheel(WHEEL_REAR_PASSENGER)->getPressureAverage() + value);
  getWheel(WHEEL_FRONT_DRIVER)->initPressureGoal(getWheel(WHEEL_FRONT_DRIVER)->getPressureAverage() + value);
  getWheel(WHEEL_REAR_DRIVER)->initPressureGoal(getWheel(WHEEL_REAR_DRIVER)->getPressureAverage() + value);
  
}

Wheel *wheel[4];
Wheel *getWheel(int i) {
  return wheel[i];
}


void setup() {
  Serial.begin(9600);

  bt.begin(9600); // start the bluetooth uart at 9600 which is its default
  delay(200); // wait for voltage stabilize

  //delay(1000);
  //bt.print("AT+NAMEvetteair");
  //delay(1000);
  //bt.print("AT+PIN0000");
  //delay(1000);
  //return;

  #if SCREEN_MOODE == true
  Serial.println(F("Startup!"));
  #else
  Serial.println(F("Startup (s)!"));
  #endif

  setupSolenoidPins();
  #if SCREEN_MOODE == true
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  #endif
  
  delay(20);

  wheel[WHEEL_FRONT_PASSENGER] = new Wheel(solenoidFrontPassengerInPin, solenoidFrontPassengerOutPin, pressureInputFrontPassenger, WHEEL_FRONT_PASSENGER);
  wheel[WHEEL_REAR_PASSENGER] = new Wheel(solenoidRearPassengerInPin, solenoidRearPassengerOutPin, pressureInputRearPassenger, WHEEL_REAR_PASSENGER);
  wheel[WHEEL_FRONT_DRIVER] = new Wheel(solenoidFrontDriverInPin, solenoidFrontDriverOutPin, pressureInputFrontDriver, WHEEL_FRONT_DRIVER);
  wheel[WHEEL_REAR_DRIVER] = new Wheel(solenoidRearDriverInPin, solenoidRearDriverOutPin, pressureInputRearDriver, WHEEL_REAR_DRIVER);

  readProfile(getBaseProfile());

  readPressures();

#if SCREEN_MOODE == true
  drawairtekklogo();
#endif

  

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.

  //testdrawchar();
  //testdrawstyles();
  //testscrolltext();
  //initPressureGoalFront(100);

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

float readPinPressure(int pin);
const int time_solenoid_movement_delta = 500;//ms
const int time_solenoid_open_time = 1;//ms
void readPressures() {
  pressureValueTank = readPinPressure(pressureInputTank);

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

const int sensorreadDelay = 100; //constant integer to set the sensor read delay in milliseconds
unsigned long lastPressureReadTime = 0;
bool pause_exe = false;
void loop() {
  bt_cmd();
  if (pause_exe == false) {
    if (millis() - lastPressureReadTime > sensorreadDelay) {
      if (!isAnyWheelActive()) {
        readPressures();
      }
      /*for (int i = 0; i < 8; i++) {
          Serial.print((char)('0'+i));
          Serial.print(": ");
          Serial.print(analogRead(A0+i));
          Serial.print("(");
          Serial.print(readPinPressure(A0+i));
          Serial.print(")");
          Serial.print(", ");
      }
      Serial.println();*/
      lastPressureReadTime = millis();
    }
#if SCREEN_MOODE == true
    drawPSIReadings();
#endif

    pressureGoalRoutine();

    //readButtonInput();
  
  }
  
  delay(10);
}
/*unsigned long lastButtonReadTime = 0;
void readButtonInput() {
  if (digitalRead(buttonRisePin) == HIGH && millis() > (lastButtonReadTime + 3000)) {
    lastButtonReadTime = millis();
    airUp();
  }
  if (digitalRead(buttonFallPin) == HIGH && millis() > (lastButtonReadTime + 3000)) {
    lastButtonReadTime = millis();
    airOut();
  }
}*/

#if SCREEN_MOODE == true
void drawPSIReadings() {
  display.clearDisplay();

  display.drawBitmap(0,0,logo_bmp_corvette, 128, 20, 1);
  
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  int textHeightPx = 10;
  int secondRowXPos = SCREEN_WIDTH/2;

  /*display.setCursor(0,2*textHeightPx);
  display.print(F("Front"));

  display.setCursor(secondRowXPos,2*textHeightPx);
  display.print(F("Rear"));*/

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

  /*display.setTextSize(3);
  display.setTextColor(SSD1306_WHITE);
  
  display.setCursor(0,3*textHeightPx);
  display.print(int(getFrontPressure()));

  display.setCursor(secondRowXPos,3*textHeightPx);
  display.print(int(getRearPressure()));*/

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
#endif

#define PASSWORD     "35264978"
#define PASSWORDSEND "56347893"
void sendHeartbeat() {
  bt.print(F(PASSWORDSEND));
  bt.print(F("PRES"));
  bt.print(int(getWheel(WHEEL_FRONT_PASSENGER)->getPressureAverage()));
  bt.print(F("|"));
  bt.print(int(getWheel(WHEEL_REAR_PASSENGER)->getPressureAverage()));
  bt.print(F("|"));
  bt.print(int(getWheel(WHEEL_FRONT_DRIVER)->getPressureAverage()));
  bt.print(F("|"));
  bt.print(int(getWheel(WHEEL_REAR_DRIVER)->getPressureAverage()));
  bt.print(F("|"));
  bt.print(int(getTankPressure()));
  bt.print(F("\n"));
  //Serial.println(int(getTankPressure()));
  //Serial.println(int(wheel[WHEEL_REAR_DRIVER].getPressure()));//this is wrong
  //Serial.println(int(readPinPressure(pressureInputRearDriver)));//thhis is right
}

void sendCurrentProfileData() {
  bt.print(F(PASSWORDSEND));
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
void bt_cmd() {
  if (millis() - lastHeartbeat > 500) {

    if (strlen(outString) > 0) {
      bt.print(F(PASSWORDSEND));
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
      for (int i = 0; i < 4; i++) {
        getWheel(i)->calcAvg();
      }
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
    //bt.println(F("OKAY1234"));
    pause_exe = true;
    return;
  }
  }
  
  while (bt.available()) {
    char c = bt.read();
    
    if (c == '\n') {
      //inString = String(inBuffer);
      bool valid = runInput();//execute command
      if (valid == true) {
        outString = "Received command";
      }
      //inString = "";
      memset(inBuffer, 0, sizeof(inBuffer));
      pause_exe = false;//unpause
      continue;//just to skip writing out the original \n, could also be break but whatever
    }
    //bool completed = inString.concat(c);
    inBuffer[strlen(inBuffer)] = c;
    Serial.print(c);
    //Serial.println(completed);
  }
  bt.read();

  }
}


//print to COM
void println(String str) {
  Serial.println(str);
}

int trailingInt(const char str[]) {
  //return inString.substring(inString.indexOf(str) + str.length()).toInt();
  return atoi( &inBuffer[strlen_P(str)] );
}


const char _AIRUP[] PROGMEM = PASSWORD"AIRUP\0";
const char _AIROUT[] PROGMEM = PASSWORD"AIROUT\0";
const char _AIRSM[] PROGMEM = PASSWORD"AIRSM\0";
const char _SAVETOPROFILE[] PROGMEM = PASSWORD"SPROF\0";
const char _READPROFILE[] PROGMEM = PASSWORD"PROFR\0";
const char _BASEPROFILE[] PROGMEM = PASSWORD"PRBOF\0";
const char _AIRHEIGHTA[] PROGMEM = PASSWORD"AIRHEIGHTA\0";
const char _AIRHEIGHTB[] PROGMEM = PASSWORD"AIRHEIGHTB\0";
const char _AIRHEIGHTC[] PROGMEM = PASSWORD"AIRHEIGHTC\0";
const char _AIRHEIGHTD[] PROGMEM = PASSWORD"AIRHEIGHTD\0";
const char _RISEONSTART[] PROGMEM = PASSWORD"RISEONSTART\0";
const char _RAISEONPRESSURESET[] PROGMEM = PASSWORD"ROPS\0";
const char _TESTSOL[] PROGMEM = PASSWORD"TESTSOL\0";

bool comp(char *str1, const char str2[]) {
  //return strstr(str1,str2) != NULL;

  int len1 = strlen(str1);
  int len2 = strlen_P(str2);
  //Serial.print(F("Len: "));
  //Serial.print(len1);
  //Serial.print(F(" "));
  //Serial.println(len2);
  if (len1 >= len2) {
    for (int i = 0; i < len2; i++) {
      char c1 = str1[i];
      char c2 = pgm_read_byte_near(str2 + i);;
      //Serial.print(i);
      //Serial.print(F(": "));
      //Serial.print(c1);
      //Serial.print(F(" "));
      //Serial.println(c2);
      if (c1 != c2) {
        //Serial.println(F("False"));
        return false;
      }
    }
    //Serial.println(F("True"));
    return true;
  }
  //Serial.println(F("False"));
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
  if (comp(inBuffer,_AIROUT)) {
    airOut();
    return true;
  }
  if (comp(inBuffer,_AIRSM)) {
    int value = trailingInt(_AIRSM);
    airUpRelativeToAverage(value);
    return true;
  }
  if (comp(inBuffer,_SAVETOPROFILE)) {
    unsigned long profileIndex = trailingInt(_SAVETOPROFILE);
    if (profileIndex > MAX_PROFILE_COUNT) {
      return false;
    }
    writeProfile(profileIndex);
    return true;
  }
  if (comp(inBuffer,_BASEPROFILE)) {
    unsigned long profileIndex = trailingInt(_BASEPROFILE);
    if (profileIndex > MAX_PROFILE_COUNT) {
      return false;
    }
    setBaseProfile(profileIndex);
    return true;
  }
  if (comp(inBuffer,_READPROFILE)) {
    unsigned long profileIndex = trailingInt(_READPROFILE);
    if (profileIndex > MAX_PROFILE_COUNT) {
      return false;
    }
    readProfile(profileIndex);
    return true;
  }
  if (comp(inBuffer,_AIRHEIGHTA)) {
    unsigned long height = trailingInt(_AIRHEIGHTA);//inString.substring(inString.indexOf(F((PASSWORD"AIRHEIGHTA"))) + strlen(F((PASSWORD"AIRHEIGHTA")))).toInt();
    setRideHeightFrontPassenger(height);
    return true;
  }
  if (comp(inBuffer,_AIRHEIGHTB)) {
    unsigned long height = trailingInt(_AIRHEIGHTB);//inString.substring(inString.indexOf(F(PASSWORD"AIRHEIGHTB")) + strlen(F(PASSWORD"AIRHEIGHTB"))).toInt();
    setRideHeightRearPassenger(height);
    return true;
  }
  if (comp(inBuffer,_AIRHEIGHTC)) {
    unsigned long height = trailingInt(_AIRHEIGHTC);//inString.substring(inString.indexOf(F(PASSWORD"AIRHEIGHTC")) + strlen(F(PASSWORD"AIRHEIGHTC"))).toInt();
    setRideHeightFrontDriver(height);
    return true;
  }
  if (comp(inBuffer,_AIRHEIGHTD)) {
    unsigned long height = trailingInt(_AIRHEIGHTD);//inString.substring(inString.indexOf(F(PASSWORD"AIRHEIGHTD")) + strlen(F(PASSWORD"AIRHEIGHTD"))).toInt();
    setRideHeightRearDriver(height);
    return true;
  }
  if (comp(inBuffer,_RISEONSTART)) {
    unsigned long ros = trailingInt(_RISEONSTART);//inString.substring(inString.indexOf(F(PASSWORD"RISEONSTART")) + strlen(F(PASSWORD"RISEONSTART"))).toInt();
    if (ros == 0) {
      setRiseOnStart(false);
    } else {
      setRiseOnStart(true);
    }
    return true;
  }
  if (comp(inBuffer,_RAISEONPRESSURESET)) {
    unsigned long rops = trailingInt(_RAISEONPRESSURESET);
    if (rops == 0) {
      setRaiseOnPressureSet(false);
    } else {
      setRaiseOnPressureSet(true);
    }
    return true;
  }
  #if TEST_MODE == true
    if (comp(inBuffer,_TESTSOL)) {
      unsigned long pin = trailingInt(_TESTSOL);//inString.substring(inString.indexOf(F(PASSWORD"RISEONSTART")) + strlen(F(PASSWORD"RISEONSTART"))).toInt();
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
