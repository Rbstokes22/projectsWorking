#include "eeprom.h"
#include <cstring> // memset use

STAsettings::STAsettings(){
    // sets the arrays to all 0's for the length of the MAX or array length
    memset(ssid, 0, SSID_MAX);
    memset(pass, 0, PASS_MAX);
    memset(phoneNum, 0, PHONE_MAX);
    memset(WAPpass, 0, WAP_PASS_MAX);
}

// FIGURE OUT HOW TO REWRITE EEPROM TO HANDLE THE SINGLE CHANGES COMING IN AND 
// APPROPRIATE ADDRESSING. ALSO WORK THIS IN THE BEGINNING TO START BY READING EEPROM
// ALSO ANOTHER CONSIDERATION CAN BE A FIRST START EEPROM BY CHECKING 3 DIFFERENT 
// ADDRESSES WITH KNOWN VALUES, IF THEY ARE NOT RIGHT, MAKE THEM RIGHT AND IN THE 
// FUTURE THE EEPROM KNOWS IT HAS BEEN WRITTEN.

bool STAsettings::eepromWrite(const char* type, const char* buffer) {
    uint16_t address = 0;

    // the added integer accounts for the null terminator, this sets the 
    // beginning address of the block of data reserved for each item.
    if (type == "ssid") address = 0;
    if (type == "pass") address = SSID_MAX + 1;
    if (type == "phone") address = SSID_MAX + PASS_MAX + 2;
    if (type == "WAPpass") address = SSID_MAX + PASS_MAX + PHONE_MAX + 3;

    // total size will not exceed 64
    uint16_t totalSizeBuffer = strlen(buffer);
    uint16_t totalSizeEEPROM = address + 30;

    // write to EEPROM address the iterated buffer chars
    EEPROM.begin(totalSizeEEPROM);
    for (int i = 0; i < totalSizeBuffer; i++) {
        EEPROM.write(address++, buffer[i]);
    }

    // Terminate the memory block with a null char
    EEPROM.write(address, '\0');
    EEPROM.commit();

    // This is used to compare that the buffer was written to EEPROM correctly
    if (type == "ssid") {
        eepromRead(SSID_EEPROM, true);
        if (buffer == this->ssid) return true;
    } else if (type == "pass") {
        eepromRead(PASS_EEPROM, true);
        if (buffer == this->pass) return true;
    } else if (type == "phone") {
        eepromRead(PHONE_EEPROM, true);
        if (buffer == this->phoneNum) return true;
    } else if (type == "WAPpass") {
        eepromRead(WAP_PASS_EEPROM, true);
        if (buffer == this->WAPpass) return true;
    }

    return false; // If it wasnt written correctly
}

// Doesnt return, just sets the class private variables to eeprom values
void STAsettings::eepromRead(uint8_t source, bool fromWrite) {

    // The EEPROM will begin if call outside of the eepromWrite(), this 
    // prevents EEPROM from calling begin twice
    if (!fromWrite) EEPROM.begin(200);

    uint16_t address = 0;

    // Each of these are depedinging on the memory source passed to read. 
    // They will sort thorough the entire array until the terminate or 
    // hit a null terminator, and then set that memory to the class 
    // variable.
    if (source == SSID_EEPROM) {
        address = 0;
        for (int i = 0; i < SSID_MAX; i++) {
            ssid[i] = EEPROM.read(address++);
            if (ssid[i] == '\0') break;
        }

    } else if (source == PASS_EEPROM) {
        address = SSID_MAX + 1;
        for (int i = 0; i < PASS_MAX; i++) {
            pass[i] = EEPROM.read(address++);
            if (pass[i] == '\0') break;
        }

    } else if (source == PHONE_EEPROM) {
        address = SSID_MAX + PASS_MAX + 2;
        for (int i = 0; i < PHONE_MAX; i++) {
            phoneNum[i] = EEPROM.read(address++);
            if (phoneNum[i] == '\0') break;
        }

    } else if (source == WAP_PASS_EEPROM) {
        address = SSID_MAX + PASS_MAX + PHONE_MAX + 3;
        for (int i = 0; i < WAP_PASS_MAX; i++) {
            WAPpass[i] = EEPROM.read(address++);
            if (WAPpass[i] == '\0') break;
        }
    }

    EEPROM.end();
}

const char* STAsettings::getSSID() const {
    return this->ssid;
}

const char* STAsettings::getPASS() const {
    return this->pass;
}

const char* STAsettings::getPhone() const {
    return this->phoneNum;
}

const char* STAsettings::getWapPass() const {
    return this->WAPpass;
}
