#ifndef EEPROM_H
#define EEPROM_H

#define SSID_MAX 32 
#define PASS_MAX 64 
#define PHONE_MAX 15
#define WAP_PASS_MAX 64

#define SSID_EEPROM 0
#define PASS_EEPROM 1 
#define PHONE_EEPROM 2 
#define WAP_PASS_EEPROM 3


#include <EEPROM.h>
#include <Arduino.h>

class STAsettings {
    private:
    char ssid[SSID_MAX];
    char pass[PASS_MAX];
    char phoneNum[PHONE_MAX];
    char WAPpass[WAP_PASS_MAX];

    public:
    STAsettings();
    bool eepromWrite(const char* type, const char* buffer);
    // bool eepromWrite(const String &ssid, const String &pass, const String &phone);
    void eepromRead(uint8_t source, bool fromWrite = false);
    const char* getSSID() const;
    const char* getPASS() const;
    const char* getPhone() const;
    const char* getWapPass() const;
};

#endif // EEPROM_H