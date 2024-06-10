#include "Creds.h"
#include <cstring> // memset use

namespace FlashWrite {

Credentials::Credentials(const char* nameSpace, UI::IDisplay &OLED) : 
    nameSpace(nameSpace), OLED(OLED) {

    // sets the arrays to all 0's for the length of the MAX or array length
    memset(ssid, 0, SSID_MAX);
    memset(pass, 0, PASS_MAX);
    memset(phone, 0, PHONE_MAX);
    memset(WAPpass, 0, WAP_PASS_MAX);
}

// Writes the key value pair to the NVS.
bool Credentials::write(const char* type, const char* buffer) {
    this->prefs.begin(this->nameSpace);
    uint16_t bytesWritten = 0;
    bytesWritten = this->prefs.putString(type, buffer);
    this->setChecksum(); // Will end in the checksum block
  
    // returns true if written bytes match the strlen
    return (bytesWritten == strlen(buffer));
}

// Reads the value of the provided key and copies it to the private class
// arrays.
void Credentials::read(const char* type) {
    this->prefs.begin(this->nameSpace);
    char error[30] = "Possible Corrupt Data";

    if (!this->getChecksum()) {
        OLED.displayError(error);
    }

    // Copies what is stored in prefs to the actual arrays stored by the class.
    // This is beneficial when the prefs goes out of scope.
    auto Copy = [this, type](char* array, uint16_t size) {
        strncpy(array, this->prefs.getString(type, "").c_str(), size - 1);
        array[size - 1] = '\0';
    };

    if (strcmp(type, "ssid") == 0) Copy(this->ssid, sizeof(this->ssid));
    if (strcmp(type, "pass") == 0) Copy(this->pass, sizeof(this->pass));
    if (strcmp(type, "phone") == 0) Copy(this->phone, sizeof(this->phone));
    if (strcmp(type, "WAPpass") == 0) Copy(this->WAPpass, sizeof(this->WAPpass));

    this->prefs.end();
}

// Iterates all the values
void Credentials::setChecksum() {
    this->prefs.begin(this->nameSpace);
    uint32_t totalSize = 0;
    char buffer[75];
    const char* keys[] = {"ssid", "pass", "phone", "WAPpass"};
    size_t numKeys = sizeof(keys) / sizeof(keys[0]);
    size_t bufferMax = sizeof(buffer) - 1;

    for (size_t i = 0; i < numKeys; i++) {
        strncpy(buffer, this->prefs.getString(keys[i], "").c_str(), bufferMax);
        buffer[bufferMax] = '\0';
        size_t length = strlen(buffer);

        for (int j = 0; j < length; j++) {
            totalSize += static_cast<unsigned char>(buffer[j]);
        }
    }

    this->prefs.putUInt("checksum", totalSize % 256);
    this->prefs.end();
}

bool Credentials::getChecksum() {
    this->prefs.begin(this->nameSpace);
    uint32_t totalSize = 0;
    uint8_t checksum = 0;
    uint8_t storedChecksum = 0;
    char buffer[75];
    const char* keys[] = {"ssid", "pass", "phone", "WAPpass"};
    size_t numKeys = sizeof(keys) / sizeof(keys[0]);
    size_t bufferMax = sizeof(buffer) - 1;

    for (size_t i = 0; i < numKeys; i++) {
        strncpy(buffer, this->prefs.getString(keys[i], "").c_str(), bufferMax);
        buffer[bufferMax] = '\0';
        size_t length = strlen(buffer);

        for (int j = 0; j < length; j++) {
            totalSize += static_cast<unsigned char>(buffer[j]);
        }
    }

    checksum = totalSize % 256;
    storedChecksum = this->prefs.getUInt("checksum", 0);
 
    return checksum == storedChecksum;
}



const char* Credentials::getSSID() const {
    return this->ssid;
}

const char* Credentials::getPASS() const {
    return this->pass;
}

const char* Credentials::getPhone() const {
    return this->phone;
}

const char* Credentials::getWAPpass() const {
    return this->WAPpass;
}
}