#include "Network/Creds.h"
#include <cstring> // memset use

namespace NVS {

// STATIC SETUP
char Credentials::ssid[static_cast<int>(CredsSize::SSID_MAX)] = "";
char Credentials::pass[static_cast<int>(CredsSize::PASS_MAX)] = "";
char Credentials::phone[static_cast<int>(CredsSize::PHONE_MAX)] = "";
char Credentials::WAPpass[static_cast<int>(CredsSize::WAP_PASS_MAX)] = "";

// Used as modulo in order to get remaining 8bit value to set and read checksum.
const uint16_t Credentials::checksumConst{256}; 

// Change if data needs to be modified.
const char* Credentials::keys[static_cast<int>(CredsSize::networkCredKeyQty)]
{"ssid", "pass", "phone", "WAPpass"};

enum KI {ssid, pass, phone, WAPpass}; // Key Index for keys above.

Credentials::Credentials(
    const char* nameSpace, Messaging::MsgLogHandler &msglogerr) : 

    nameSpace{nameSpace}, msglogerr(msglogerr) {

    // Used for iteration in the read method to copy the appropriate value.
    this->credInfo[0] = {
        Credentials::keys[KI::ssid], 
        Credentials::ssid, sizeof(Credentials::ssid)
    };

    this->credInfo[1] = {
        Credentials::keys[KI::pass], 
        Credentials::pass, sizeof(Credentials::pass)
    };

    this->credInfo[2] = {
        Credentials::keys[KI::phone], 
        Credentials::phone, sizeof(Credentials::phone)
    };

    this->credInfo[3] = {
        Credentials::keys[KI::WAPpass], 
        Credentials::WAPpass, sizeof(Credentials::WAPpass)
    };
}

// Writes KV pair to NVS. If bytesWritten == buffer passed, returns true.
bool Credentials::write(const char* key, const char* buffer) {
    if (!this->prefs.begin(this->nameSpace)) {
        msglogerr.handle(
            Levels::ERROR,
            "NVS Creds did not begin during write",
            Method::SRL);
    }

    size_t bufferLength = strlen(buffer) + 1; // +1 for null term.
    uint16_t bytesWritten = this->prefs.putBytes(key, buffer, bufferLength);
    
    this->setChecksum(); // Will end in the checksum block
  
    return (bytesWritten == (strlen(buffer) + 1)); // +1 for null term
}

// Reads value of key passed by iterating through the credInfo struct array and 
// returning the value for the matched key passed in argument.
void Credentials::read(const char* key) { 
    this->prefs.begin(this->nameSpace);
    char error[75]{};

    if (!this->getChecksum()) {
        snprintf(error, sizeof(error), 
        "Network creds (%s) checksum fail, potentially corrupt",
        key); 

        this->msglogerr.handle(Levels::ERROR, error, Method::OLED, Method::SRL);
    } 

    // Copies the value stored in NVS to the Network variable array (i.e. SSID).
    auto Copy = [this, key](char* array, uint16_t size) {
        size_t readSize = this->prefs.getBytes(key, array, size - 1);
        array[readSize] = '\0';
    };

    // Iterates the credInfo array, compares the key to its keys, and copies the
    // data upon a match.
    for (const auto &info : this->credInfo) {
        if (strcmp(key, info.key) == 0) {
            Copy(info.destination, info.size);
        }
    }

    this->prefs.end();
}

// Iterates through the keys in this block of NVS. Gets the number of keys, since they
// are stored as pointers, it will be the size / size of the first element. The iteration
// will read the bytes and store them into the buffer, with the buffer max being the upper
// limit. It will then append a null terminator and read the length of the buffer. Each
// char of that buffer will be iterated increasing the total size by the corresponding
// char size of that letter. The total size is then divided by the checkSumConst, 256 in
// this case which will always return a uint8_t value which is the checksum.
uint8_t Credentials::computeChecksum() {
    uint32_t totalSize{0};
    size_t numKeys{sizeof(Credentials::keys) / sizeof(Credentials::keys[0])};
    size_t readBytes{0};
    char buffer[75]{};
    size_t bufferMax{sizeof(buffer) - 1}; 

    for (size_t key = 0; key < numKeys; key++) {
        size_t readBytes = 
            this->prefs.getBytes(Credentials::keys[key], buffer, bufferMax);

        buffer[readBytes] = '\0';
        size_t length{strlen(buffer)};

        for (int chr = 0; chr < length; chr++) {
            totalSize += static_cast<unsigned char>(buffer[chr]);
        }
    }

    return totalSize % Credentials::checksumConst; // returns a uint8_t value always
}

// Stores an unsigned int into the NVS returned by computeChecksum().  Ensures the 
// bytes written are that of an uint. 
void Credentials::setChecksum() {

    uint16_t bytesWritten = 
        this->prefs.putUInt("checksum", computeChecksum());
        
    // use unsigned int instead of uint32_t to be safe from compiler issues
    if (bytesWritten != sizeof(unsigned int)) { 
        this->msglogerr.handle(
            Levels::ERROR, "Creds checksum write fail", Method::SRL);
    }

    this->prefs.end(); 
}

// Compares the stored checksum with the computed checksum from what is in the 
// NVS.
bool Credentials::getChecksum() {
    uint8_t storedChecksum = this->prefs.getUInt("checksum", 0);
 
    return (this->computeChecksum() == storedChecksum);
}

const char* Credentials::getSSID() {
    return Credentials::ssid;
}

const char* Credentials::getPASS() {
    return Credentials::pass;
}

const char* Credentials::getPhone() {
    return Credentials::phone;
}

const char* Credentials::getWAPpass() {
    return Credentials::WAPpass;
}

}