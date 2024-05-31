#include "eeprom.h"
#include <cstring> // memset use

STAsettings::STAsettings(){
    // sets the arrays to all 0's for the length of the MAX or array length
    memset(ssid, 0, SSID_MAX);
    memset(pass, 0, PASS_MAX);
    memset(phoneNum, 0, PHONE_MAX);
}

bool STAsettings::eepromWrite(const String &ssid, const String &pass, const String &phone) {
    int totalSize = ssid.length() + pass.length() + phone.length() + 3; // 3 for null term char
    
    EEPROM.begin(totalSize);
    int addr = 0;

    for (int i = 0; i < ssid.length(); i++) {
        EEPROM.write(addr++, ssid[i]);
    }
    EEPROM.write(addr++, '\0'); // place null terminating char between data

    for (int i = 0; i < pass.length(); i++) {
        EEPROM.write(addr++, pass[i]);
    }
    EEPROM.write(addr++, '\0');

    for (int i = 0; i < phone.length(); i++) {
        EEPROM.write(addr++, phone[i]);
    }
    EEPROM.write(addr++, '\0');

    EEPROM.commit(); // commit to memory

    eepromRead(1); // Will set the values to the private variables
    
    // Validates that the eeprom reads the same way it wrote
    if (
        ssid == this->ssid && pass == this->pass && phone == phoneNum) {
        return 1;
    } else {
        return 0;
    }
}

// Doesnt return, just sets the class private variables to eeprom values
void STAsettings::eepromRead(bool action) {

    // The EEPROM will begin if call outside of the eepromWrite()
    if (!action) {EEPROM.begin(128);}

    int addr = 0;

    for (int i = 0; i < SSID_MAX; i++) {
        ssid[i] = EEPROM.read(addr++);
        if (ssid[i] == '\0') {break;}
    }

    for (int i = 0; i < PASS_MAX; i++) {
        pass[i] = EEPROM.read(addr++);
        if (pass[i] == '\0') {break;}
    }

    for (int i = 0; i < PHONE_MAX; i++) {
        phoneNum[i] = EEPROM.read(addr++);
        if (phoneNum[i] == '\0') {break;}
    }

    EEPROM.end();
}

const char* STAsettings::getSSID() const {
    return ssid;
}

const char* STAsettings::getPASS() const {
    return pass;
}

const char* STAsettings::getPhone() const {
    return phoneNum;
}
