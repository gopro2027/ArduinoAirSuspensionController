#include "preferencable.h"

// Tutorial with preferences code https://randomnerdtutorials.com/esp32-save-data-permanently-preferences/

#define SAVEDATA_NAMESPACE "savedata"

Preferences preferences;

void openNamespace(const char *ns, bool ro)
{
    preferences.begin(ns, ro);
}

void endNamespace()
{
    preferences.end();
}

void deletePreference(const char *name) {
    openNamespace(SAVEDATA_NAMESPACE, false);
    preferences.remove(name);
    endNamespace();
}

void writeBytes(const char *name, const void *bytes, size_t len, const char *mode) {
    File file = SPIFFS.open(name, mode, true);
    if (!file) {
        Serial.println("Failed to open file for writing");
    } else {
        file.write(static_cast<const uint8_t *>(bytes), len);
        file.close();
    }
}


size_t readBytes(const char *name, void *buf, size_t maxLen) {
    File file = SPIFFS.open(name, "r");
    if (!file) {
        Serial.println("Failed to open file for reading");
        return static_cast<size_t>(-1);
    }

    const size_t toRead = file.size() < maxLen ? file.size() : maxLen;
    const size_t got = file.readBytes(static_cast<char *>(buf), toRead);
    file.close();
    return got;
}

void deleteFile(const char *name) {
    SPIFFS.remove(name);
}

void Preferencable::load(const char *name, uint64_t defaultValue)
{
    memset(this->name, 0, sizeof(this->name));     // make sure it's 0 terminated
    strncpy(this->name, name, sizeof(this->name)); // cap it to 14 with 0 termination at end
    openNamespace(SAVEDATA_NAMESPACE, true);
    if (preferences.isKey(this->name) == false)
    {
        endNamespace();
        openNamespace(SAVEDATA_NAMESPACE, false); // reopen as read write
        preferences.putULong64(this->name, defaultValue);
        this->value.i = defaultValue;
    }
    else
    {
        this->value.i = preferences.getULong64(name, defaultValue);
    }
    endNamespace();
}

void Preferencable::set(uint64_t val)
{
    if (this->value.i != val)
    {
        this->value.i = val;
        openNamespace(SAVEDATA_NAMESPACE, false);
        preferences.putULong64(this->name, val);
        endNamespace();
    }
}

void Preferencable::loadDouble(const char *name, double defaultValue)
{
    strncpy(this->name, name, sizeof(this->name));
    openNamespace(SAVEDATA_NAMESPACE, true);
    if (preferences.isKey(name) == false)
    {
        endNamespace();
        openNamespace(SAVEDATA_NAMESPACE, false); // reopen as read write
        preferences.putDouble(this->name, defaultValue);
        this->value.d = defaultValue;
    }
    else
    {
        this->value.d = preferences.getDouble(name, defaultValue);
    }
    endNamespace();
}

void Preferencable::setDouble(double val)
{
    if (this->value.d != val)
    {
        this->value.d = val;
        openNamespace(SAVEDATA_NAMESPACE, false);
        preferences.putDouble(this->name, val);
        endNamespace();
    }
}

void Preferencable::loadString(const char *name, String defaultValue) {
    strncpy(this->name, name, sizeof(this->name));
    openNamespace(SAVEDATA_NAMESPACE, true);
    if (preferences.isKey(name) == false)
    {
        endNamespace();
        openNamespace(SAVEDATA_NAMESPACE, false); // reopen as read write
        preferences.putString(this->name, defaultValue);
    }
    endNamespace();
}
void Preferencable::setString(String val) {
    openNamespace(SAVEDATA_NAMESPACE, false);
    preferences.putString(this->name, val);
    endNamespace();
}
String Preferencable::getString() {
    openNamespace(SAVEDATA_NAMESPACE, true);
    String str = preferences.getString(this->name,"");
    endNamespace();
    return str;
}

void Preferencable::deletePreference()
{
    ::deletePreference(this->name);
}

