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


void saveBytes(const char *name, const void *bytes, size_t len) {
    openNamespace(SAVEDATA_NAMESPACE, false);
    preferences.putBytes(name, bytes, len);
    endNamespace();
}


size_t readBytes(const char *name, void *buf, size_t maxLen) {
    openNamespace(SAVEDATA_NAMESPACE, true);

    if (preferences.isKey(name) == false)
    {
        endNamespace();
        openNamespace(SAVEDATA_NAMESPACE, false); // reopen as read write
        char def[1] = {0};
        preferences.putBytes(name, def, 1);
        return 0;
    }

    maxLen = preferences.getBytes(name, buf, maxLen);
    endNamespace();
    return maxLen;
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
