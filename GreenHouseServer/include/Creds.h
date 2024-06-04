#ifndef CREDS_H
#define CREDS_H

// CHANGE FROM EEPROM TO CREDS OR SOMETHING LIKE THAT
// CHANGE STAsettings as well

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

#include <Arduino.h>

class Credentials {
    private:
    char ssid[SSID_MAX];
    char pass[PASS_MAX];
    char phoneNum[PHONE_MAX];
    char WAPpass[WAP_PASS_MAX];
    uint16_t EEPROMsize;

    public:
    Credentials(uint16_t size);
    uint8_t initialSetup(
        uint16_t addr1, // pick an address beyond the data block
        uint8_t expVal1, 
        uint16_t addr2, // pick another address beyond the data block
        uint8_t expVal2,
        uint16_t dataBlockSize);
    bool eepromWrite(const char* type, const char* buffer);
    void eepromRead(uint8_t source, bool fromWrite = false);
    const char* getSSID() const;
    const char* getPASS() const;
    const char* getPhone() const;
    const char* getWAPpass() const;
};

#endif // CREDS_H