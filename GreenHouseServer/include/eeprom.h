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

#define EEPROM_UP 0
#define EEPROM_INITIALIZED 1
#define EEPROM_INIT_FAILED 2


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
    uint8_t initialSetup(
        uint16_t addr1, // pick an address beyond the data block
        uint8_t expVal1, 
        uint16_t addr2, // pick another address beyond the data block
        uint8_t expVal2,
        uint16_t dataBlockSize);
    bool eepromWrite(const char* type, const char* buffer);
    // bool eepromWrite(const String &ssid, const String &pass, const String &phone);
    void eepromRead(uint8_t source, bool fromWrite = false);
    const char* getSSID() const;
    const char* getPASS() const;
    const char* getPhone() const;
    const char* getWAPpass() const;
};

#endif // EEPROM_H