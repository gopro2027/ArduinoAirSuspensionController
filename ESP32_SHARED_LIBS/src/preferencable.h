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

void saveBytes(const char *name, const void *bytes, size_t len);
size_t readBytes(const char *name, void *buf, size_t maxLen);

#define createSaveFuncInt(VARNAME, _TYPE) \
    _TYPE get##VARNAME()                  \
    {                                     \
        return _SaveData.VARNAME.get().i; \
    }                                     \
    void set##VARNAME(_TYPE value)        \
    {                                     \
        if (get##VARNAME() != value)      \
        {                                 \
            _SaveData.VARNAME.set(value); \
        }                                 \
    }

#define headerDefineSaveFunc(VARNAME, _TYPE) \
    _TYPE get##VARNAME();                    \
    void set##VARNAME(_TYPE value);

#endif