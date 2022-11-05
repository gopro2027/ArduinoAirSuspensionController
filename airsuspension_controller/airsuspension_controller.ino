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
#define TEST_MODE true

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define WHEEL_FRONT_PASSENGER 0
#define WHEEL_REAR_PASSENGER 1
#define WHEEL_FRONT_DRIVER 2
#define WHEEL_REAR_DRIVER 3


//Digital pins
SoftwareSerial bt(3, 2); // RX, TX
const int buttonRisePin = 4;
const int buttonFallPin = 5;
const int solenoidFrontPassengerInPin = 6;
const int solenoidFrontPassengerOutPin = 7;
const int solenoidRearPassengerInPin = 8;
const int solenoidRearPassengerOutPin = 9;
const int solenoidFrontDriverInPin = 10;
const int solenoidFrontDriverOutPin = 11;
const int solenoidRearDriverInPin = 12;
const int solenoidRearDriverOutPin = 13;

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

const int rideHeightFrontPassengerAddr = 0;
const int rideHeightRearPassengerAddr = 1;
const int rideHeightFrontDriverAddr = 2;
const int rideHeightRearDriverAddr = 3;
const int riseOnStartAddr = 4;

void setRideHeightFrontPassenger(byte value) {
   EEPROM.write(rideHeightFrontPassengerAddr, value);
}
void setRideHeightRearPassenger(byte value) {
   EEPROM.write(rideHeightRearPassengerAddr, value);
}
void setRideHeightFrontDriver(byte value) {
   EEPROM.write(rideHeightFrontDriverAddr, value);
}
void setRideHeightRearDriver(byte value) {
   EEPROM.write(rideHeightRearDriverAddr, value);
}
void setRiseOnStart(bool value) {
   EEPROM.write(riseOnStartAddr, value);
}
byte getRideHeightFrontPassenger() {
   return EEPROM.read(rideHeightFrontPassengerAddr);
}
byte getRideHeightRearPassenger() {
   return EEPROM.read(rideHeightRearPassengerAddr);
}
byte getRideHeightFrontDriver() {
   return EEPROM.read(rideHeightFrontDriverAddr);
}
byte getRideHeightRearDriver() {
   return EEPROM.read(rideHeightRearDriverAddr);
}
bool getRiseOnStart() {
   return EEPROM.read(riseOnStartAddr);
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

  pinMode(buttonRisePin, INPUT);
  pinMode(buttonFallPin, INPUT);

  //pinMode(13, OUTPUT);
  //digitalWrite(13, HIGH);
}


void pressureGoalRoutine() {
  bool active = false;
  for (int i = 0; i < 4; i++) {
    if (getWheel(i).isActive()) {
      active = true;
    }
  }
  if (active) {
    readPressures();
  }
  for (int i = 0; i < 4; i++) {
    getWheel(i).pressureGoalRoutine();
  }
}

void airUp() {
  
  getWheel(WHEEL_FRONT_PASSENGER).initPressureGoal(getRideHeightFrontPassenger());
  getWheel(WHEEL_REAR_PASSENGER).initPressureGoal(getRideHeightRearPassenger());
  getWheel(WHEEL_FRONT_DRIVER).initPressureGoal(getRideHeightFrontDriver());
  getWheel(WHEEL_REAR_DRIVER).initPressureGoal(getRideHeightRearDriver());
  
}

void airOut() {

  getWheel(WHEEL_FRONT_PASSENGER).initPressureGoal(10);
  getWheel(WHEEL_REAR_PASSENGER).initPressureGoal(10);
  getWheel(WHEEL_FRONT_DRIVER).initPressureGoal(10);
  getWheel(WHEEL_REAR_DRIVER).initPressureGoal(10);
  
}

Wheel wheel[4];
Wheel getWheel(int i) {
  return wheel[i];
}


void setup() {
  Serial.begin(9600);

  bt.begin(9600); // start the bluetooth uart at 9600 which is its default
  delay(200); // wait for voltage stabilize

  Serial.println("Startup!");

  if (TEST_MODE) {
    //setRideHeightFrontPassenger(90);
    //setRideHeightRearPassenger(100);
    //setRideHeightFrontDriver(90);
    //setRideHeightRearDriver(100);
    //setRiseOnStart(false);
    //bt.print(F("AT+NAMEvaair")); // place your name in here to configure the bluetooth name.
                                       // will require reboot for settings to take affect. 
    //delay(3000); // wait for settings to take affect. 
  }

  setupSolenoidPins();
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  
  delay(20);

  wheel[WHEEL_FRONT_PASSENGER] = Wheel(solenoidFrontPassengerInPin, solenoidFrontPassengerOutPin, pressureInputFrontPassenger);
  wheel[WHEEL_REAR_PASSENGER] = Wheel(solenoidRearPassengerInPin, solenoidRearPassengerOutPin, pressureInputRearPassenger);
  wheel[WHEEL_FRONT_DRIVER] = Wheel(solenoidFrontDriverInPin, solenoidFrontDriverOutPin, pressureInputFrontDriver);
  wheel[WHEEL_REAR_DRIVER] = Wheel(solenoidRearDriverInPin, solenoidRearDriverOutPin, pressureInputRearDriver);

  readPressures();

  drawairtekklogo();

  

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.

  //testdrawchar();
  //testdrawstyles();
  //testscrolltext();
  //initPressureGoalFront(100);

  //if (getRiseOnStart() == true) {
  //  airUp();
  //}
}


float pressureValueTank = 0;
int getTankPressure() {
  //if (TEST_MODE)
  //  return 999;
  return pressureValueTank;
}

float readPinPressure(int pin);
void readPressures() {
  pressureValueTank = readPinPressure(pressureInputTank);
  for (int i = 0; i < 4; i++) {
    getWheel(i).readPressure();
  }
}

const int sensorreadDelay = 100; //constant integer to set the sensor read delay in milliseconds
unsigned long lastPressureReadTime = 0;
bool pause_exe = false;
void loop() {
  bt_cmd();
  if (pause_exe == false) {
    if (millis() - lastPressureReadTime > sensorreadDelay) {
      readPressures();
      lastPressureReadTime = millis();
    }
    drawPSIReadings();

    pressureGoalRoutine();

    readButtonInput();
  
  }
  
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
  display.print(int(wheel[WHEEL_FRONT_DRIVER].getPressure()));//front driver

  display.setCursor(secondRowXPos,2*textHeightPx+5);
  display.print(F("FP: "));
  display.print(int(wheel[WHEEL_FRONT_PASSENGER].getPressure()));//front passenger

//Rear
  display.setCursor(0,3.5*textHeightPx+5);
  display.print(F("RD: "));
  display.print(int(wheel[WHEEL_REAR_DRIVER].getPressure()));//rear driver

  display.setCursor(secondRowXPos,3.5*textHeightPx+5);
  display.print(F("RP: "));
  display.print(int(wheel[WHEEL_REAR_PASSENGER].getPressure()));//rear passenger

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

#define PASSWORD     "35264978"
#define PASSWORDSEND "56347893"
void sendHeartbeat() {
  bt.print(F(PASSWORDSEND));
  bt.print(int(wheel[WHEEL_FRONT_PASSENGER].getPressure()));
  bt.print(F(":"));
  bt.print(int(wheel[WHEEL_REAR_PASSENGER].getPressure()));
  bt.print(F(":"));
  bt.print(int(wheel[WHEEL_FRONT_DRIVER].getPressure()));
  bt.print(F(":"));
  bt.print(int(wheel[WHEEL_REAR_DRIVER].getPressure()));
  bt.print(F(":"));
  bt.print(int(getTankPressure()));
  bt.print(F("\n"));
  //Serial.println(int(wheel[WHEEL_REAR_DRIVER].getPressure()));
}

//https://www.seeedstudio.com/blog/2020/01/02/how-to-control-arduino-with-bluetooth-module-and-shields-to-get-started/

String inString = "";
unsigned long lastHeartbeat = 0;
void bt_cmd() {
  if (millis() - lastHeartbeat > 500) {
    sendHeartbeat();
    lastHeartbeat = millis();
  } else {

  //Get input as string
  if (bt.available() && pause_exe == false) {
    while (Serial.available() > 0) {
      Serial.read();
    }
    delay(200);
    //bt.println(F("OKAY1234"));
    pause_exe = true;
    return;
  }
  while (bt.available()) {
    char c = bt.read();
    Serial.print(c);
    if (c == '\n') {
      runInput();//execute command
      inString = "";
      pause_exe = false;//unpause
      continue;//just to skip writing out the original \n, could also be break but whatever
    }
    inString += c;
  }
  bt.read();

  }
}


//print to COM
void println(String str) {
  Serial.println(str);
}

int trailingInt(String str) {
  return inString.substring(inString.indexOf(str) + str.length()).toInt();
}

void runInput() {
  //run input
  String str = "";
  if (inString.indexOf(F(PASSWORD"AIRUP")) != -1) {
    airUp();
    return;
  }
  if (inString.indexOf(F(PASSWORD"AIROUT")) != -1) {
    airOut();
    return;
  }
  str = F(PASSWORD"AIRHEIGHTA");
  if (inString.indexOf(str) != -1) {
    unsigned long height = trailingInt(str);//inString.substring(inString.indexOf(F((PASSWORD"AIRHEIGHTA"))) + strlen(F((PASSWORD"AIRHEIGHTA")))).toInt();
    setRideHeightFrontPassenger(height);
    return;
  }
  str = F(PASSWORD"AIRHEIGHTB");
  if (inString.indexOf(str) != -1) {
    unsigned long height = trailingInt(str);//inString.substring(inString.indexOf(F(PASSWORD"AIRHEIGHTB")) + strlen(F(PASSWORD"AIRHEIGHTB"))).toInt();
    setRideHeightRearPassenger(height);
    return;
  }
  str = F(PASSWORD"AIRHEIGHTC");
  if (inString.indexOf(str) != -1) {
    unsigned long height = trailingInt(str);//inString.substring(inString.indexOf(F(PASSWORD"AIRHEIGHTC")) + strlen(F(PASSWORD"AIRHEIGHTC"))).toInt();
    setRideHeightFrontDriver(height);
    return;
  }
  str = F(PASSWORD"AIRHEIGHTD");
  if (inString.indexOf(str) != -1) {
    unsigned long height = trailingInt(str);//inString.substring(inString.indexOf(F(PASSWORD"AIRHEIGHTD")) + strlen(F(PASSWORD"AIRHEIGHTD"))).toInt();
    setRideHeightRearDriver(height);
    return;
  }
  str = F(PASSWORD"RISEONSTART");
  if (inString.indexOf(str) != -1) {
    unsigned long ros = trailingInt(str);//inString.substring(inString.indexOf(F(PASSWORD"RISEONSTART")) + strlen(F(PASSWORD"RISEONSTART"))).toInt();
    if (ros == 0) {
      setRiseOnStart(false);
    } else {
      setRiseOnStart(true);
    }
    return;
  }
  if (TEST_MODE) {
  str = F(PASSWORD"TESTSOL");
  if (inString.indexOf(str) != -1) {
    unsigned long pin = trailingInt(str);//inString.substring(inString.indexOf(F(PASSWORD"RISEONSTART")) + strlen(F(PASSWORD"RISEONSTART"))).toInt();
    if (pin >= 6 && pin <= 13) {
      digitalWrite(pin, HIGH);
      delay(100);//sleep 100ms
      digitalWrite(pin, LOW);
    }
    return;
  }
  }
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