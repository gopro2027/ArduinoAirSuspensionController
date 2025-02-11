#include "preferencable.h"

// Tutorial with preferences code https://randomnerdtutorials.com/esp32-save-data-permanently-preferences/

#define SAVEDATA_NAMESPACE "savedata"

Preferences preferences;

void openNamespace(char *ns, bool ro)
{
    preferences.begin(ns, ro);
}

void endNamespace()
{
    preferences.end();
}

void Preferencable::load(char *name, int defaultValue)
{
    memset(this->name, 0, sizeof(this->name));     // make sure it's 0 terminated
    strncpy(this->name, name, sizeof(this->name)); // cap it to 14 with 0 termination at end
    openNamespace(SAVEDATA_NAMESPACE, true);
    if (preferences.isKey(this->name) == false)
    {
        endNamespace();
        openNamespace(SAVEDATA_NAMESPACE, false); // reopen as read write
        preferences.putInt(this->name, defaultValue);
        this->value.i = defaultValue;
    }
    else
    {
        this->value.i = preferences.getInt(name, defaultValue);
    }
    endNamespace();
}

void Preferencable::set(int val)
{
    if (this->value.i != val)
    {
        this->value.i = val;
        openNamespace(SAVEDATA_NAMESPACE, false);
        preferences.putInt(this->name, val);
        endNamespace();
    }
}

void Preferencable::loadFloat(char *name, float defaultValue)
{
    strncpy(this->name, name, sizeof(this->name));
    openNamespace(SAVEDATA_NAMESPACE, true);
    if (preferences.isKey(name) == false)
    {
        endNamespace();
        openNamespace(SAVEDATA_NAMESPACE, false); // reopen as read write
        preferences.putFloat(this->name, defaultValue);
        this->value.f = defaultValue;
    }
    else
    {
        this->value.f = preferences.getFloat(name, defaultValue);
    }
    endNamespace();
}

void Preferencable::setFloat(float val)
{
    if (this->value.f != val)
    {
        this->value.f = val;
        openNamespace(SAVEDATA_NAMESPACE, false);
        preferences.putFloat(this->name, val);
        endNamespace();
    }
}
