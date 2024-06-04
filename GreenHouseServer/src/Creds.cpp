#include <EEPROM.h>
#include "Creds.h"
#include <cstring> // memset use

Credentials::Credentials(uint16_t size) : EEPROMsize(size)

{
    // sets the arrays to all 0's for the length of the MAX or array length
    memset(ssid, 0, SSID_MAX);
    memset(pass, 0, PASS_MAX);
    memset(phoneNum, 0, PHONE_MAX);
    memset(WAPpass, 0, WAP_PASS_MAX);
}

// When initially starting, checks EEPROM for two known values at chosen
// addresses. If they do not exist, system knows the EEPROM is being started
// for the first time, it will then null out your dataBlock and write the 
// chosen values at the chosen addresses. This will allow Network cred validation
// to see if a null char exists as the expected address and know which password
// to begin for the WAP setting.
uint8_t Credentials::initialSetup(
    uint16_t addr1, 
    uint8_t expVal1, 
    uint16_t addr2, 
    uint8_t expVal2,
    uint16_t dataBlockSize) {

    EEPROM.begin(this->EEPROMsize);
    uint8_t firstInt = EEPROM.read(addr1);
    uint8_t secondInt = EEPROM.read(addr2);
    Serial.println(firstInt);
    Serial.println(secondInt);

    if (EEPROM.read(addr1) == expVal1 && EEPROM.read(addr2) == expVal2) {
        return EEPROM_UP;
    } else {
        
        for (int i = 0; i < dataBlockSize; i++) { // Nullifies
            EEPROM.write(i, '\0');
        }

        EEPROM.write(addr1, expVal1);
        EEPROM.write(addr2, expVal2);
        EEPROM.commit(); 

        if (EEPROM.read(addr1) == expVal1 && EEPROM.read(addr2) == expVal2) {
            return EEPROM_INITIALIZED;
        } else {
            return EEPROM_INIT_FAILED;
        }
    }
}

bool Credentials::eepromWrite(const char* type, const char* buffer) {
    uint16_t address = 0;

    // the added integer accounts for the null terminator, this sets the 
    // beginning address of the block of data reserved for each item.
    if (type == "ssid") address = 0;
    if (type == "pass") address = SSID_MAX + 1;
    if (type == "phone") address = SSID_MAX + PASS_MAX + 2;
    if (type == "WAPpass") address = SSID_MAX + PASS_MAX + PHONE_MAX + 3;

    // total size will not exceed 64
    uint16_t totalSizeBuffer = strlen(buffer);
    // uint16_t totalSizeEEPROM = address + 30;

    // write to EEPROM address the iterated buffer chars
    // EEPROM.begin(512);
    for (int i = 0; i < totalSizeBuffer; i++) {
        EEPROM.write(address++, buffer[i]);
    }

    // Terminate the memory block with a null char
    EEPROM.write(address, '\0'); 
    EEPROM.commit();

    // This is used to compare that the buffer was written to EEPROM correctly
    if (type == "ssid") {
        eepromRead(SSID_EEPROM, true);
        if (strcmp(buffer, this->ssid) == 0) return true;
    } else if (type == "pass") {
        eepromRead(PASS_EEPROM, true);
        if (strcmp(buffer, this->pass) == 0) return true;
    } else if (type == "phone") {
        eepromRead(PHONE_EEPROM, true);
        if (strcmp(buffer, this->phoneNum) == 0) return true;
    } else if (type == "WAPpass") {
        eepromRead(WAP_PASS_EEPROM, true);
        if (strcmp(buffer, this->WAPpass) == 0) return true;
    }

    return false; // If it wasnt written correctly
}

// Doesnt return, just sets the class private variables to eeprom values
void Credentials::eepromRead(uint8_t source, bool fromWrite) {

    // The EEPROM will begin if call outside of the eepromWrite(), this 
    // prevents EEPROM from calling begin twice
    // if (!fromWrite) EEPROM.begin(512);

    uint16_t address = 0;

    // Each of these are depedinging on the memory source passed to read. 
    // They will sort thorough the entire array until the terminate or 
    // hit a null terminator, and then set that memory to the class 
    // variable.
    if (source == SSID_EEPROM) {
        address = 0;
        for (int i = 0; i < SSID_MAX; i++) {
            this->ssid[i] = EEPROM.read(address++);
            if (this->ssid[i] == '\0') break;
        }

    } else if (source == PASS_EEPROM) {
        address = SSID_MAX + 1;
        for (int i = 0; i < PASS_MAX; i++) {
            this->pass[i] = EEPROM.read(address++);
            if (this->pass[i] == '\0') break;
        }

    } else if (source == PHONE_EEPROM) {
        address = SSID_MAX + PASS_MAX + 2;
        for (int i = 0; i < PHONE_MAX; i++) {
            this->phoneNum[i] = EEPROM.read(address++);
            if (this->phoneNum[i] == '\0') break;
        }

    } else if (source == WAP_PASS_EEPROM) {
        address = SSID_MAX + PASS_MAX + PHONE_MAX + 3;
        for (int i = 0; i < WAP_PASS_MAX; i++) {
            this->WAPpass[i] = EEPROM.read(address++);
            if (this->WAPpass[i] == '\0') break;
        }
    }

    // EEPROM.end();
}

const char* Credentials::getSSID() const {
    return this->ssid;
}

const char* Credentials::getPASS() const {
    return this->pass;
}

const char* Credentials::getPhone() const {
    return this->phoneNum;
}

const char* Credentials::getWAPpass() const {
    return this->WAPpass;
}
