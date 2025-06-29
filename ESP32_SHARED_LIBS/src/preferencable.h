#ifndef preferencable_h
#define preferencable_h

#include <Arduino.h>
#include <Preferences.h>
#include <SPIFFS.h>

union PreferencableValue
{
    uint64_t i;
    double d;
};

class Preferencable
{

public:
    char name[15]; // 15 is max len. Note for future devs: I didn't add any code to make sure it is 0 terminated so be careful how you choose a name i guess
    // char buffer[4];
    PreferencableValue value;
    void load(const char *name, uint64_t defaultValue);
    void set(uint64_t val);
    void loadDouble(const char *name, double defaultValue);
    void setDouble(double val);

    // separate functions for stirng
    void loadString(const char *name, String defaultValue);
    void setString(String val);
    String getString();

    void deletePreference();
    PreferencableValue get()
    {
        return value;
    }
};

size_t readBytes(const char *name, void *buf, size_t maxLen);

void writeBytes(const char *name, const void *bytes, size_t len, const char *mode = "w");
void deleteFile(const char *name);

void deletePreference(const char *name);

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

#define createSaveFuncString(VARNAME)           \
    String get##VARNAME()                       \
    {                                           \
        return _SaveData.VARNAME.getString();   \
    }                                           \
    void set##VARNAME(String value)             \
    {                                           \
        if (get##VARNAME() != value)            \
        {                                       \
            _SaveData.VARNAME.setString(value); \
        }                                       \
    }

#define headerDefineSaveFunc(VARNAME, _TYPE) \
    _TYPE get##VARNAME();                    \
    void set##VARNAME(_TYPE value);

#endif