#ifndef preferencable_h
#define preferencable_h

#include <Arduino.h>
#include <Preferences.h>

union PreferencableValue
{
    int i;
    float f;
};

class Preferencable
{

public:
    char name[15]; // 15 is max len. Note for future devs: I didn't add any code to make sure it is 0 terminated so be careful how you choose a name i guess
    PreferencableValue value;
    void load(char *name, int defaultValue);
    void set(int val);
    void loadFloat(char *name, float defaultValue);
    void setFloat(float val);
    PreferencableValue get()
    {
        return value;
    }
};

#endif