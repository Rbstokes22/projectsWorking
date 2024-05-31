#ifndef EEPROM_H
#define EEPROM_H

#define SSID_MAX 32 
#define PASS_MAX 64 
#define PHONE_MAX 15

#include <EEPROM.h>
#include <Arduino.h>

class STAsettings {
    private:
    char ssid[SSID_MAX];
    char pass[PASS_MAX];
    char phoneNum[PHONE_MAX];

    public:
    STAsettings();
    bool eepromWrite(const char* type, const char* buffer);
    // bool eepromWrite(const String &ssid, const String &pass, const String &phone);
    void eepromRead(bool action);
    const char* getSSID() const;
    const char* getPASS() const;
    const char* getPhone() const;
};

#endif // EEPROM_H