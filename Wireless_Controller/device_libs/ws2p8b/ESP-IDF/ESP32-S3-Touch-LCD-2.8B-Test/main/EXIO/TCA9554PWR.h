#pragma once


#include <stdio.h>
#include "driver/i2c.h"

#include "Buzzer.h"

#define TCA9554_EXIO1 0x01
#define TCA9554_EXIO2 0x02
#define TCA9554_EXIO3 0x03
#define TCA9554_EXIO4 0x04
#define TCA9554_EXIO5 0x05
#define TCA9554_EXIO6 0x06
#define TCA9554_EXIO7 0x07
#define TCA9554_EXIO8 0x08

#define I2C_MASTER_SDA_IO           15     
#define I2C_MASTER_SCL_IO           7  
#define I2C_MASTER_NUM              0                       // Specify the I2C bus port to use. ESP32 chips typically have two I2C bus ports: I2C_NUM_0 and I2C_NUM_1
#define I2C_MASTER_FREQ_HZ          400000                  // I2C master clock frequency, set to 400KHz 
#define I2C_MASTER_TIMEOUT_MS       1000

/****************************************************** The macro defines the TCA9554PWR information ******************************************************/ 

#define TCA9554_ADDRESS             0x20                    // TCA9554PWR I2C address
// TCA9554PWR寄存器地址
#define TCA9554_INPUT_REG           0x00                    // Input register,input level
#define TCA9554_OUTPUT_REG          0x01                    // Output register, high and low level output 
#define TCA9554_Polarity_REG        0x02                    // The Polarity Inversion register (register 2) allows polarity inversion of pins defined as inputs by the Configuration register.  
#define TCA9554_CONFIG_REG          0x03                    // Configuration register, mode configuration


// esp_err_t i2c_master_init(void);                            // Example Initialize I2C to host mode
/*****************************************************  Operation register REG   ****************************************************/   
uint8_t Read_REG(uint8_t REG);                              // Read the value of the TCA9554PWR register REG
void Write_REG(uint8_t REG,uint8_t Data);                   // Write Data to the REG register of the TCA9554PWR
/********************************************************** Set EXIO mode **********************************************************/       
void Mode_EXIO(uint8_t Pin,uint8_t State);                  // Set the mode of the TCA9554PWR Pin. The default is Output mode (output mode or input mode). State: 0= Output mode 1= input mode   
void Mode_EXIOS(uint8_t PinState);                          // Set the mode of the 7 pins from the TCA9554PWR with PinState  
/********************************************************** Read EXIO status **********************************************************/       
uint8_t Read_EXIO(uint8_t Pin);                             // Read the level of the TCA9554PWR Pin
uint8_t Read_EXIOS(void);                                   // Read the level of all pins of TCA9554PWR, the default read input level state, want to get the current IO output state, pass the parameter TCA9554_OUTPUT_REG, such as Read_EXIOS(TCA9554_OUTPUT_REG);
/********************************************************** Set the EXIO output status **********************************************************/  
void Set_EXIO(uint8_t Pin,uint8_t State);                   // Sets the level state of the Pin without affecting the other pins
void Set_EXIOS(uint8_t PinState);                           // Set 7 pins to the PinState state such as :PinState=0x23, 0010 0011 state (the highest bit is not used)
/********************************************************** Flip EXIO state **********************************************************/  
void Set_Toggle(uint8_t Pin);                               // Flip the level of the TCA9554PWR Pin
/********************************************************* TCA9554PWR Initializes the device ***********************************************************/  
void TCA9554PWR_Init(uint8_t PinState);                     // Set the seven pins to PinState state, for example :PinState=0x23, 0010 0011 State (the highest bit is not used) (Output mode or input mode) 0= Output mode 1= Input mode. The default value is output mode

esp_err_t EXIO_Init(void);
