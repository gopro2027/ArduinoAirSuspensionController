#ifndef user_defines_h
#define user_defines_h

#define SERIAL_BAUD_RATE 115200

#define BT_NAME "OASMan"

#define MAX_PRESSURE_SAFETY 200

/* Bags generally do not like to sit at exactly 0psi. Please choose which pressure is desired for air out */
#define AIR_OUT_PRESSURE_PSI 30

/* Set to false if you don't plan to ever use the PS3 controller. MAC address can be left alone. Instructions for controller at https://github.com/gopro2027/ArduinoAirSuspensionController/tree/main/PS3_Controller_Tool */
#define ENABLE_PS3_CONTROLLER_SUPPORT true

/* This is the private passcode you need to access your system from the app. Set the same value in the app settings after launching the app. */
#define PASSWORD "12345678"

/* turns on/off some test features for debug use. Best left false */
#define TEST_MODE false

/* LCD screen definitions */
#define SCREEN_ENABLED false
#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 64    // OLED display height, in pixels
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

/* By default the android app is set up for 4 profiles */
#define MAX_PROFILE_COUNT 4

/* These are the pin numbers used for our manifold solenoids */
#define solenoidFrontPassengerInPin new InputType(33, OUTPUT)
#define solenoidFrontPassengerOutPin new InputType(25, OUTPUT)
#define solenoidRearPassengerInPin new InputType(23, OUTPUT)
#define solenoidRearPassengerOutPin new InputType(19, OUTPUT)
#define solenoidFrontDriverInPin new InputType(26, OUTPUT)
#define solenoidFrontDriverOutPin new InputType(27, OUTPUT)
#define solenoidRearDriverInPin new InputType(18, OUTPUT)
#define solenoidRearDriverOutPin new InputType(17, OUTPUT)

/* Compressor/tank */
#define compressorRelayPin new InputType(13, OUTPUT) // D13, solenoid
#define pressureInputTank new InputType(32, INPUT)   // D32/A0, pressure sensor

// These will not be exact depending on how accurate your pressure sensors are.
// For example: Mine will read 220psi when the actual pressure is 180psi
#define COMPRESSOR_ON_BELOW_PSI 140
#define COMPRESSOR_MAX_PSI 180

/* Pressure sensor pins */
#define pressureInputFrontPassenger new InputType(0, &ADS1115A) // ADSA/0   Previous: D36/VP/A4
#define pressureInputRearPassenger new InputType(3, &ADS1115A)  // ADSA/3   Previous: D35/A3
#define pressureInputFrontDriver new InputType(2, &ADS1115A)    // ADSA/2   Previous: D34/A2
#define pressureInputRearDriver new InputType(1, &ADS1115A)     // ADSA/1   Previous: D39/VN/A7

/* Level sensor pins */
#define levelInputFrontPassenger new InputType(0, ADS1115B) // ADSB/0
#define levelInputRearPassenger new InputType(3, ADS1115B)  // ADSB/3
#define levelInputFrontDriver new InputType(2, ADS1115B)    // ADSB/2
#define levelInputRearDriver new InputType(1, ADS1115B)     // ADSB/1

/* Set to true if in any of the InputType's above you use ADS (Adafruit_ADS1115) */
#define USE_ADS true
/* Set to true if you use 2 ads board (low and high) */
#define USE_2_ADS true
#define ADS_A_ADDRESS 0x48 // 0x48 is address pin to low
#define ADS_B_ADDRESS 0x49 // 0x49 is address pin to high

/* Disable the hang if ads fails to load */
#define ADS_MOCK_BYPASS false

/* For testing purposes: mock tank pressure to 200psi */
#define TANK_PRESSURE_MOCK false

/* Values for pressure calculations */
#define pressureZeroAnalogValue (float)409.6 // analog reading of pressure transducer at 0psi.          for nano: (0.5 volts / 5 volts) * 1024 = 102.4. for esp32: (0.5 volts / 5 volts) * 4096 = 409.6
#define pressureMaxAnalogValue (float)3686.4 // analog reading of pressure transducer at max psi.       for nano: (4.5 volts / 5 volts) * 1024 = 921.6. for esp32: (4.5 volts / 5 volts) * 4096 = 3686.4
#define pressuretransducermaxPSI 232         // psi value of transducer being used. (1.6MPA)

/* DO NOT CHANGE ANY PAST THIS LINE */
#define WHEEL_FRONT_PASSENGER 0
#define WHEEL_REAR_PASSENGER 1
#define WHEEL_FRONT_DRIVER 2
#define WHEEL_REAR_DRIVER 3

/* The amount of time the pressure routine will try to reach the goal pressure before 'giving up' (usually due to lower pressure in tank or something) Default is 10 seconds. */
#define ROUTINE_TIMEOUT_MS 10 * 1000

#endif

/*
Note: if you want to test esp32 without being on the assembled pcb turn these values:
ADS_MOCK_BYPASS true
TANK_PRESSURE_MOCK true
SCREEN_ENABLED false

For prod use you should turn any mock or test features off
 */