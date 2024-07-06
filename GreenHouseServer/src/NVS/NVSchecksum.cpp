#include "NVS/NVS.h"

namespace NVS {

// Called in the writetoNVS method. Ensures that the checksum is computed
// using crc32, and the uint32_t value is written to the NVS with the same
// keyname appended with a CS. Despite the new key buffer size, if the data
// exceeds it, regardless of the key input, it will be consistenly wrong
// throughout the program, thus correct.
bool NVSctrl::setChecksum(const char* key, const uint8_t* data, size_t size) {
    if(!this->basicErrCheck(key, data, size)) return false;

    uint32_t checkSum{this->computeChecksum(data, size)};
    char newKey[20]{0};
    snprintf(newKey, sizeof(newKey), "CS%s", key);

    if (this->prefs.putULong(newKey, checkSum) == sizeof(uint32_t)) {
        this->prefs.end();
        return true;
    } else {
        this->sendErr("NVS, Checksum not written");
        return false;
    }
}

// Called in the readfromNVS method. Sends the read data via byte array,
// which will then be sent to the crc32 to compute the checksum value. 
// This will then be compared, and if even, will return true.
bool NVSctrl::getChecksum(const char* key,const uint8_t* data, size_t size) {
    if(!this->basicErrCheck(key, data, size)) return false;

    char newKey[20]{0};
    snprintf(newKey, sizeof(newKey), "CS%s", key);

    uint32_t storedCheckSum = this->prefs.getULong(newKey, 0);
    uint32_t computedCS = this->computeChecksum(data, size);

    if (storedCheckSum == computedCS) {
        this->prefs.end();
        return true;
    } else {
        this->sendErr("NVS Checksum does not match, either corrupt or missing");
        return false;
    }
}

// (VERBOSE COMMENTS DUE TO COMPLEXITY)
// Uses the Cyclic Redundancy Check 32 (CRC32) to detect for 
// errors and corruption of data. See CPP references on CRC32
// for very detailed description.
uint32_t NVSctrl::computeChecksum(const uint8_t* data, size_t size) {

    // If bad data is sent to checksum, returns a max value and
    // sets the flag for bad data.
    if (data == nullptr || size == 0) {
        this->isCheckSumSafe = false; 
        return 0xFFFFFFFF;

    } else {

        // Creates a pointer that points to the bytes of the data
        // by casting the pointer as a uint8_t type.
        const uint8_t* bytes = static_cast<const uint8_t*>(data);

        // Good data was sent, so flag is set to true.
        this->isCheckSumSafe = true;

        // Inits the crc32 at max value to ensure all leading bits 
        // are set which helps process data starting with zeros.
        uint32_t crc32 = 0xFFFFFFFF;

        // Each byte is iterated for the size passed, which is why
        // the correct size is crucial.
        for(size_t addr = 0; addr < size; addr++) {

            // The crc32 is then xor with the byte data, which is a method
            // to integrate the bytes into the crc32 in a non destructive 
            // way. Everything is reversable and bits. This iteration
            // goes bit by bit shifting  or shifting and xoring with the 
            // polynomial when the least significant bit == 1. 
            crc32 ^= bytes[addr];

            for(size_t bit = 0; bit < 8; bit++) {
                if (crc32 & 1) {
                    crc32 = (crc32 >> 1) ^ 0xEDB88320;
                } else {
                    crc32 >>= 1;
                }
            }
        }

        // Ensures it does not return 0 with a 0 input.
        return crc32 ^ 0xFFFFFFFF;
    }
}
    
}