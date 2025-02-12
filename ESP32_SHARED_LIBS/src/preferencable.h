#ifndef preferencable_h
#define preferencable_h

#include <Arduino.h>
#include <Preferences.h>

union PreferencableValue
{
    uint64_t i;
    double d;
};

class Preferencable
{

public:
    char name[15]; // 15 is max len. Note for future devs: I didn't add any code to make sure it is 0 terminated so be careful how you choose a name i guess
    PreferencableValue value;
    void load(const char *name, uint64_t defaultValue);
    void set(uint64_t val);
    void loadDouble(const char *name, double defaultValue);
    void setDouble(double val);
    PreferencableValue get()
    {
        return value;
    }
};

#endif