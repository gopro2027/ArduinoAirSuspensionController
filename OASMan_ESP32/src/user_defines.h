#ifndef user_defines_h
#define user_defines_h

#define SERIAL_BAUD_RATE 115200

#define BT_NAME "OASMan"

#define MAX_PRESSURE_SAFETY 200

/* This is the private passcode you need to access your system from the app. Set the same value in the app settings after launching the app. */
#define PASSWORD     "12345678"

/* turns on/off some test features for debug use. Best left false */
#define TEST_MODE false

/* LCD screen definitions */
#define SCREEN_MOODE true
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

/* By default the android app is set up for 4 profiles */
#define MAX_PROFILE_COUNT 4

/* These are the pin numbers used for our manifold solenoids */
#define solenoidFrontPassengerInPin     new InputType(33, OUTPUT)
#define solenoidFrontPassengerOutPin    new InputType(25, OUTPUT)
#define solenoidRearPassengerInPin      new InputType(23, OUTPUT)
#define solenoidRearPassengerOutPin     new InputType(19, OUTPUT)
#define solenoidFrontDriverInPin        new InputType(26, OUTPUT)
#define solenoidFrontDriverOutPin       new InputType(27, OUTPUT)
#define solenoidRearDriverInPin         new InputType(18, OUTPUT)
#define solenoidRearDriverOutPin        new InputType(17, OUTPUT)

/* Compressor/tank */
#define compressorRelayPin new InputType(13, OUTPUT) // D13, solenoid
#define pressureInputTank new InputType(32, INPUT) // D32/A0, pressure sensor

// These will not be exact depending on how accurate your pressure sensors are.
// For example: Mine will read 220psi when the actual pressure is 180psi
#define COMPRESSOR_ON_BELOW_PSI 140
#define COMPRESSOR_MAX_PSI 180

/* Pressure sensor pins */
#define pressureInputFrontPassenger new InputType(36, INPUT) // D36/VP/A4
#define pressureInputRearPassenger  new InputType(35, INPUT) // D35/A3
#define pressureInputFrontDriver    new InputType(34, INPUT) // D34/A2
#define pressureInputRearDriver     new InputType(39, INPUT) // D39/VN/A7

/* (Custom boards only) Set to true if in any of the InputType's above you use ADS (Adafruit_ADS1115) */
#define USE_ADS false
/* (Custom boards only) Set to true if you use 2 ads board (low and high) */
#define USE_2_ADS false
#define ADS_A_ADDRESS 0x48 // 0x48 is address pin to low
#define ADS_B_ADDRESS 0x49 // 0x49 is address pin to high

/* Values for pressure calculations */
#define pressureZeroAnalogValue (float)409.6 //analog reading of pressure transducer at 0psi.          for nano: (0.5 volts / 5 volts) * 1024 = 102.4. for esp32: (0.5 volts / 5 volts) * 4096 = 409.6
#define pressureMaxAnalogValue (float)3686.4 //analog reading of pressure transducer at max psi.       for nano: (4.5 volts / 5 volts) * 1024 = 921.6. for esp32: (4.5 volts / 5 volts) * 4096 = 3686.4
#define pressuretransducermaxPSI 300 //psi value of transducer being used.

#endif