#ifndef user_defines_h
#define user_defines_h

#define SERIAL_BAUD_RATE 115200

#define BT_NAME "OASMan"

#define MAX_PRESSURE_SAFETY 200

// original full amount is 500
#define LEARN_SAVE_COUNT 250

/* Bags generally do not like to sit at exactly 0psi. Please choose which pressure is desired for air out */
/* Not really used anymore, just using presets! Only kept here as legacy for og app */
#define AIR_OUT_PRESSURE_PSI 30

/* Set to false if you don't plan to ever use the PS3 controller. MAC address can be left alone. Instructions for controller at https://github.com/gopro2027/ArduinoAirSuspensionController/tree/main/PS3_Controller_Tool */
#define ENABLE_PS3_CONTROLLER_SUPPORT true

/* This is the private passcode you need to access your system from the app. Set the same value in the app settings after launching the app. */
/* This is legacy bt, and ota but ota is only enabled when chosen so we can leave it as is */
#define PASSWORD "12345678"

// pass key is 6 digits, add zeros to the beginning of it to make it 6 in total
// For example, if your pass is 2027 please type 002027
#define BLE_PASSKEY 202777

/* turns on/off some test features for debug use. Best left false */
#define TEST_MODE false

/* LCD screen definitions */
#define SCREEN_ENABLED true
#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 64    // OLED display height, in pixels
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

/* By default the android app is set up for 4 profiles */
#define MAX_PROFILE_COUNT 5

/* These are the pin numbers used for our manifold solenoids */

#ifdef BOARD_VERSION_ATLEAST_4

#define solenoidFrontPassengerInPin new InputType(18, OUTPUT)
#define solenoidFrontPassengerOutPin new InputType(17, OUTPUT)
#define solenoidRearPassengerInPin new InputType(19, OUTPUT)
#define solenoidRearPassengerOutPin new InputType(33, OUTPUT)
#define solenoidFrontDriverInPin new InputType(23, OUTPUT)
#define solenoidFrontDriverOutPin new InputType(25, OUTPUT)
#define solenoidRearDriverInPin new InputType(27, OUTPUT)
#define solenoidRearDriverOutPin new InputType(26, OUTPUT)

#else

/* Default pin numbers before switch to 4 layer board. v2.X through v3.X */
#define solenoidFrontPassengerInPin new InputType(33, OUTPUT)
#define solenoidFrontPassengerOutPin new InputType(25, OUTPUT)
#define solenoidRearPassengerInPin new InputType(23, OUTPUT)
#define solenoidRearPassengerOutPin new InputType(19, OUTPUT)
#define solenoidFrontDriverInPin new InputType(26, OUTPUT)
#define solenoidFrontDriverOutPin new InputType(27, OUTPUT)
#define solenoidRearDriverInPin new InputType(18, OUTPUT)
#define solenoidRearDriverOutPin new InputType(17, OUTPUT)

#endif

/* Pressure Sensor Inputs. Why are the ads pin nums in this specific order? Oh the world may never know */
#define pressureSensorInput0 new InputType(0, &ADS1115A) // ADSA/0   Previous: D36/VP/A4   Default: pressureInputFrontPassenger
#define pressureSensorInput1 new InputType(3, &ADS1115A) // ADSA/3   Previous: D35/A3      Default: pressureInputRearPassenger
#define pressureSensorInput2 new InputType(2, &ADS1115A) // ADSA/2   Previous: D34/A2      Default: pressureInputFrontDriver
#define pressureSensorInput3 new InputType(1, &ADS1115A) // ADSA/1   Previous: D39/VN/A7   Default: pressureInputRearDriver
#define pressureSensorInput4 new InputType(32, INPUT)    // D32/A0, pressure sensor        Default: pressureInputTank

/* Compressor/tank */
#define compressorRelayPin new InputType(13, OUTPUT) // D13, solenoid

/* Accessory Wire */
#define outputKeepAlivePin new InputType(12, OUTPUT) // D12, output high while accessory input is low to keep input on. Should always output high while accessory is on. Output low when accessory is low to turn off system.
#define accessoryInput new InputType(35, INPUT)      // D34 because it's adc1 input only //D14, digital in high or low. 0 = acc on, 1 = acc off (it's on a pullup resistor)
#define SYSTEM_SHUTOFF_TIME_M 15                     // 15 minutes

// These will not be exact depending on how accurate your pressure sensors are.
// For example: Mine will read 220psi when the actual pressure is 180psi
#define COMPRESSOR_ON_BELOW_PSI 140
#define COMPRESSOR_MAX_PSI 180

/* Level sensor pins */
#define levelInputFrontPassenger new InputType(0, &ADS1115B) // ADSB/0
#define levelInputRearPassenger new InputType(3, &ADS1115B)  // ADSB/3
#define levelInputFrontDriver new InputType(2, &ADS1115B)    // ADSB/2
#define levelInputRearDriver new InputType(1, &ADS1115B)     // ADSB/1

/* Set to true if in any of the InputType's above you use ADS (Adafruit_ADS1115) */
#define USE_ADS true
/* Set to true if you use 2 ads board (low and high) */
#define USE_2_ADS true
#define ADS_A_ADDRESS 0x48 // 0x48 is address pin to low
#define ADS_B_ADDRESS 0x49 // 0x49 is address pin to high

/* Disable the hang if ads fails to load */
#define ADS_MOCK_BYPASS true

/* For testing purposes: mock tank pressure to 200psi */
#define TANK_PRESSURE_MOCK true

/* Values for pressure calculations */
#define pressuretransducerRunningVoltage 5.0f                                                                                                    // most pressure sensors run on 5v
#define pressuretransducerVoltageZeroPSI 0.5f                                                                                                    // most say 0.5v but may differ  (I was reading 0.295f for 0psi on base esp32) (on multimeter tested and readings were consistent with 0.5v... .501, .503, .499, .502)
#define pressuretransducerVoltageMaxPSI 4.5f                                                                                                     // most say 4.5v but may differ
#define pressuretransducermaxPSI 232                                                                                                             // psi value of transducer being used. (1.6MPA = 232PSI)
#define microcontrollerMaxAnalogReading 4095                                                                                                     // esp32 built in adc goes to 4095
#define pressureZeroAnalogValue (float)((pressuretransducerVoltageZeroPSI / pressuretransducerRunningVoltage) * microcontrollerMaxAnalogReading) // analog reading of pressure transducer at 0psi.
#define pressureMaxAnalogValue (float)((pressuretransducerVoltageMaxPSI / pressuretransducerRunningVoltage) * microcontrollerMaxAnalogReading)   // analog reading of pressure transducer at max psi.

/* The amount of time the pressure routine will try to reach the goal pressure before 'giving up' (usually due to lower pressure in tank or something) Default is 10 seconds. */
#define ROUTINE_TIMEOUT_MS 10 * 1000

#define AIR_OUT_AFTER_SHUTDOWN_MS 5000

/* Use new BLE with latest features or old classic bluetooth with the classic android app */
#define USE_BLE true

#define THEME_COLOR_DARK 0x5A4673
#define THEME_COLOR_MEDIUM 0x9C4DCC
#define THEME_COLOR_LIGHT 0xBB86FC

/* DO NOT CHANGE ANY PAST THIS LINE */
#define WHEEL_FRONT_PASSENGER 0
#define WHEEL_REAR_PASSENGER 1
#define WHEEL_FRONT_DRIVER 2
#define WHEEL_REAR_DRIVER 3
#define _TANK_INDEX 4

enum SOLENOID_INDEX
{
    FRONT_PASSENGER_IN,
    FRONT_PASSENGER_OUT,
    REAR_PASSENGER_IN,
    REAR_PASSENGER_OUT,
    FRONT_DRIVER_IN,
    FRONT_DRIVER_OUT,
    REAR_DRIVER_IN,
    REAR_DRIVER_OUT
};
enum SOLENOID_AI_INDEX
{
    AI_MODEL_UP_FRONT,
    AI_MODEL_UP_REAR,
    AI_MODEL_DOWN_FRONT,
    AI_MODEL_DOWN_REAR,
    AI_MODEL_UNDEFINED
};
#define SOLENOID_COUNT 8

// platformio.ini toggleables

#if defined(OFFICIAL_RELEASE)
#define ADS_MOCK_BYPASS false
#define TANK_PRESSURE_MOCK false
#endif

#endif

/*
Note: if you want to test esp32 without being on the assembled pcb turn these values:
ADS_MOCK_BYPASS true
TANK_PRESSURE_MOCK true
SCREEN_ENABLED false

For prod use you should turn any mock or test features off
 */