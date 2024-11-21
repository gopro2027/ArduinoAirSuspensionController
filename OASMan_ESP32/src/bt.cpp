#include "bt.h"

BluetoothSerial bt;

void sendHeartbeat()
{
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

void sendCurrentProfileData()
{
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

// https://www.seeedstudio.com/blog/2020/01/02/how-to-control-arduino-with-bluetooth-module-and-shields-to-get-started/

char *outString = "";
// String inString = "";
char inBuffer[30];
unsigned long lastHeartbeat = 0;
void bt_cmd()
{
    if (millis() - lastHeartbeat > 500)
    {

        if (strlen(outString) > 0)
        {
            bt.print(F(PASSWORD));
            bt.print(F("NOTIF"));
            bt.print(outString);
            bt.print(F("\n"));
            Serial.println(outString);
            outString = "";
        }
        else if (sendProfileBT == true)
        {
            sendProfileBT = false;
            sendCurrentProfileData();
        }
        else
        {
            sendHeartbeat();
        }
        lastHeartbeat = millis();
    }
    else
    {

        // Get input as string
        if (pause_exe == false)
        {
            if (bt.available())
            {
                while (Serial.available() > 0)
                {
                    Serial.read();
                }
                delay(200);
                pause_exe = true;
                return;
            }
        }

        while (bt.available())
        {
            char c = bt.read();

            if (c == '\n')
            {
                bool valid = runInput(); // execute command
                if (valid == true)
                {
                    outString = "SUCC";
                }
                else
                {
                    outString = "ERRUNK";
                }
                memset(inBuffer, 0, sizeof(inBuffer));
                pause_exe = false; // unpause
                continue;          // just to skip writing out the original \n, could also be break but whatever
            }
            inBuffer[strlen(inBuffer)] = c;
            Serial.print(c);
        }
        bt.read();
    }
}

int trailingInt(const char str[])
{
    return atoi(&inBuffer[strlen_P(str)]);
}

const char _AIRUP[] PROGMEM = PASSWORD "AIRUP\0";
const char _AIROUT[] PROGMEM = PASSWORD "AIROUT\0";
const char _AIRSM[] PROGMEM = PASSWORD "AIRSM\0";
const char _SAVETOPROFILE[] PROGMEM = PASSWORD "SPROF\0";
const char _READPROFILE[] PROGMEM = PASSWORD "PROFR\0";
const char _AIRUPQUICK[] PROGMEM = PASSWORD "AUQ\0";
const char _BASEPROFILE[] PROGMEM = PASSWORD "PRBOF\0";
const char _AIRHEIGHTA[] PROGMEM = PASSWORD "AIRHEIGHTA\0";
const char _AIRHEIGHTB[] PROGMEM = PASSWORD "AIRHEIGHTB\0";
const char _AIRHEIGHTC[] PROGMEM = PASSWORD "AIRHEIGHTC\0";
const char _AIRHEIGHTD[] PROGMEM = PASSWORD "AIRHEIGHTD\0";
const char _RISEONSTART[] PROGMEM = PASSWORD "RISEONSTART\0";
const char _RAISEONPRESSURESET[] PROGMEM = PASSWORD "ROPS\0";
#if TEST_MODE == true
const char _TESTSOL[] PROGMEM = PASSWORD "TESTSOL\0";
#endif

bool comp(char *str1, const char str2[])
{

    int len1 = strlen(str1);
    int len2 = strlen_P(str2);
    if (len1 >= len2)
    {
        for (int i = 0; i < len2; i++)
        {
            char c1 = str1[i];
            char c2 = pgm_read_byte_near(str2 + i);
            if (c1 != c2)
            {
                return false;
            }
        }
        return true;
    }
    return false;
}

bool runInput()
{
    // run input
    Serial.print(F("inBuffer: "));
    Serial.println(inBuffer);
    if (comp(inBuffer, _AIRUP))
    {
        airUp();
        return true;
    }
    else if (comp(inBuffer, _AIROUT))
    {
        airOut();
        return true;
    }
    else if (comp(inBuffer, _AIRSM))
    {
        int value = trailingInt(_AIRSM);
        airUpRelativeToAverage(value);
        skipPerciseSet = true; // will be reset by any call to Wheel::initPressureGoal
        return true;
    }
    else if (comp(inBuffer, _SAVETOPROFILE))
    {
        unsigned long profileIndex = trailingInt(_SAVETOPROFILE);
        if (profileIndex > MAX_PROFILE_COUNT)
        {
            return false;
        }
        writeProfile(profileIndex);
        return true;
    }
    else if (comp(inBuffer, _BASEPROFILE))
    {
        unsigned long profileIndex = trailingInt(_BASEPROFILE);
        if (profileIndex > MAX_PROFILE_COUNT)
        {
            return false;
        }
        setBaseProfile(profileIndex);
        return true;
    }
    else if (comp(inBuffer, _READPROFILE))
    {
        unsigned long profileIndex = trailingInt(_READPROFILE);
        if (profileIndex > MAX_PROFILE_COUNT)
        {
            return false;
        }
        readProfile(profileIndex);
        return true;
    }
    else if (comp(inBuffer, _AIRUPQUICK))
    {
        unsigned long profileIndex = trailingInt(_AIRUPQUICK);
        if (profileIndex > MAX_PROFILE_COUNT)
        {
            return false;
        }
        // load profile then air up
        readProfile(profileIndex);
        airUp();
        skipPerciseSet = true; // will be reset by any call to Wheel::initPressureGoal
        return true;
    }
    else if (comp(inBuffer, _AIRHEIGHTA))
    {
        unsigned long height = trailingInt(_AIRHEIGHTA);
        setRideHeightFrontPassenger(height);
        return true;
    }
    else if (comp(inBuffer, _AIRHEIGHTB))
    {
        unsigned long height = trailingInt(_AIRHEIGHTB);
        setRideHeightRearPassenger(height);
        return true;
    }
    else if (comp(inBuffer, _AIRHEIGHTC))
    {
        unsigned long height = trailingInt(_AIRHEIGHTC);
        setRideHeightFrontDriver(height);
        return true;
    }
    else if (comp(inBuffer, _AIRHEIGHTD))
    {
        unsigned long height = trailingInt(_AIRHEIGHTD);
        setRideHeightRearDriver(height);
        return true;
    }
    else if (comp(inBuffer, _RISEONSTART))
    {
        unsigned long ros = trailingInt(_RISEONSTART);
        if (ros == 0)
        {
            setRiseOnStart(false);
        }
        else
        {
            setRiseOnStart(true);
        }
        return true;
    }
    else if (comp(inBuffer, _RAISEONPRESSURESET))
    {
        unsigned long rops = trailingInt(_RAISEONPRESSURESET);
        if (rops == 0)
        {
            setRaiseOnPressureSet(false);
        }
        else
        {
            setRaiseOnPressureSet(true);
        }
        return true;
    }
#if TEST_MODE == true
    else if (comp(inBuffer, _TESTSOL))
    {
        unsigned long pin = trailingInt(_TESTSOL);
        Serial.println(pin);
        if (pin >= 6 && pin <= 13)
        {
            digitalWrite(pin, HIGH);
            delay(1000); // sleep 100ms
            digitalWrite(pin, LOW);
        }
        return true;
    }
#endif
    return false;
}
