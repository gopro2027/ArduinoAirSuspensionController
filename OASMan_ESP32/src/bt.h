// NOTE: This is the classic bluetooth version and will be depricated eventually
#ifndef bt_h
#define bt_h
#include "user_defines.h"
#include "Wheel.h"
#include "BluetoothSerial.h"

#include "airSuspensionUtil.h"

//values
extern BluetoothSerial bt;
extern bool pause_exe;


//functions
void sendHeartbeat();
void sendCurrentProfileData();
void bt_cmd();
bool runInput();
int trailingInt(const char str[]);
bool comp(char *str1, const char str2[]);
bool runInput();


#endif