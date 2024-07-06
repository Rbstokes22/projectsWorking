#include "Network/Creds.h"
#include <cstring> // memset use

namespace NVS {

// STATIC SETUP
char Credentials::ssid[static_cast<int>(Comms::IDXSIZE::SSID)] = "";
char Credentials::pass[static_cast<int>(Comms::IDXSIZE::PASS)] = "";
char Credentials::phone[static_cast<int>(Comms::IDXSIZE::PHONE)] = "";
char Credentials::WAPpass[static_cast<int>(Comms::IDXSIZE::PASS)] = "";

Credentials::Credentials(
    const char* nameSpace, Messaging::MsgLogHandler &msglogerr) : 

    nameSpace{nameSpace}, msglogerr(msglogerr), checkSumSafe{true} {

    // Used for iteration in the read method to copy the appropriate value.
    this->credInfo[0] = {
        Comms::keys[static_cast<int>(Comms::KI::ssid)], 
        Credentials::ssid, sizeof(Credentials::ssid)
    };

    this->credInfo[1] = {
        Comms::keys[static_cast<int>(Comms::KI::pass)], 
        Credentials::pass, sizeof(Credentials::pass)
    };

    this->credInfo[2] = {
        Comms::keys[static_cast<int>(Comms::KI::phone)], 
        Credentials::phone, sizeof(Credentials::phone)
    };

    this->credInfo[3] = {
        Comms::keys[static_cast<int>(Comms::KI::WAPpass)], 
        Credentials::WAPpass, sizeof(Credentials::WAPpass)
    };
}

// Writes KV pair to NVS. If bytesWritten == buffer passed, returns true.
bool Credentials::write(const char* key, const char* buffer) {
    if (!this->prefs.begin(this->nameSpace)) {
        msglogerr.handle(
            Messaging::Levels::ERROR,
            "NVS Creds did not begin during write",
            Messaging::Method::SRL); 
    }

    size_t bufferLength = strlen(buffer) + 1; // +1 for null term.
    uint16_t bytesWritten = this->prefs.putBytes(key, buffer, bufferLength);
    
    this->setChecksum(key, buffer); // Will end in the checksum block
  
    return (bytesWritten == (strlen(buffer) + 1)); // +1 for null term
}

// Reads value of key passed by iterating through the credInfo struct array and 
// returning the value for the matched key passed in argument.
void Credentials::read(const char* key) { 
    this->prefs.begin(this->nameSpace);
    char error[75]{};

    auto handleError = [this, &error](const char* key){
        snprintf(error, sizeof(error), 
        "(%s) checksum fail, either corrupt or unwritten",
        key); 

        this->msglogerr.handle(
            Messaging::Levels::ERROR, 
            error, 
            Messaging::Method::OLED, Messaging::Method::SRL);
    };

    // Copies the value stored in NVS to the Network variable array (i.e. SSID).
    auto Copy = [this, key, handleError](char* array, uint16_t size) {
        size_t readSize = this->prefs.getBytes(key, array, size - 1);
        array[readSize] = '\0';
        if (!this->getChecksum(key, array)) handleError(key);

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

// (VERBOSE DEFINITIION DUE TO COMPLEXITY)
// Uses the crc32 method to check for corruption in the data. If error, responds
// with all 1's but sets check sum safe to false indicating bad data. If data
// is safe, indication is set, and the crc32 is set to max value at all 1's. 
// The buffersize is computed allowing each char of the buffer to be analyzed
// in the iteration. 

// This will xor the crc32 with the char, which will set the
// crc32 bit to a 1 iff the bits are unequal, which detects change or disrepancy.
// This ensures that any change in the data alters the crc32, and this action is
// propegated throughout subsequent operations. 

// Each byte of the 


uint32_t Credentials::computeChecksum(const char* buffer) {
    if (buffer == nullptr || *buffer == '\0') {
        this->checkSumSafe = false; 
        return 0xFFFFFFFF;
    } else {
        this->checkSumSafe = true;
        uint32_t crc32 = 0xFFFFFFFF;
        size_t bufferSize = strlen(buffer);

        for (size_t addr = 0; addr < bufferSize; addr++) {
            crc32 ^= static_cast<unsigned char>(buffer[addr]); // Added safety if signed char

            for (size_t bit = 0; bit < 8; bit++) {
                if (crc32 & 1) {
                    crc32 = (crc32 >> 1) ^ 0xEDB88320; // Reversed polynomial
                } else {
                    crc32 >>= 1;
                }
            }
        }
        
        return crc32 ^ 0xFFFFFFFF;
    }
}

// Stores an unsigned int into the NVS returned by computeChecksum().  Ensures the 
// bytes written are that of an uint. 
void Credentials::setChecksum(const char* key, const char* buffer) {
    char CSname[20]{0};
    snprintf(CSname, sizeof(CSname), "%sCS", key);

    uint16_t bytesWritten = 
        this->prefs.putUInt(CSname, computeChecksum(buffer));
        
    // use unsigned int instead of uint32_t to be safe from compiler issues
    if (bytesWritten != sizeof(unsigned int)) { 
        this->msglogerr.handle(
            Messaging::Levels::ERROR, 
            "Creds checksum write fail", 
            Messaging::Method::SRL);
    }

    this->prefs.end(); 
}

// Compares the stored checksum with the computed checksum from what is in the 
// NVS.
bool Credentials::getChecksum(const char* key, const char* buffer) {
    char CSname[20]{0};
    snprintf(CSname, sizeof(CSname), "%sCS", key);

    uint32_t storedChecksum = this->prefs.getUInt(CSname, 0);
 
    return (this->computeChecksum(buffer) == storedChecksum && this->checkSumSafe);
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