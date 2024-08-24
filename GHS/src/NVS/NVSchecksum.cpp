#include "NVS/NVS.hpp"
#include "UI/MsgLogHandler.hpp"
#include "nvs.h"
#include "string.h"

namespace NVS {

// Called in the writetoNVS method. NVS already runs CRC32, but there is an
// additional redundancy built in below. This will set the checkSum to the 
// CRC32 computation, and write it to NVS using the passed key, prepending
// it with CS. Returns CHECKSUM_FAIL or CHECKSUM_OK.
nvs_ret_t NVSctrl::setChecksum(const char* key, const uint8_t* data, size_t size) {

    if(this->basicErrCheck(key, data, size) == nvs_ret_t::NVS_FAIL) {
        return nvs_ret_t::CHECKSUM_FAIL;
    }

    uint32_t checkSum{this->computeChecksum(data, size)};
    char newKey[15]{0}; // max limit per nvs docs.
    snprintf(newKey, sizeof(newKey), "CS%s", key); 

    this->err = nvs_set_u32(this->handle, newKey, checkSum);

    nvs_ret_t success = this->errHandlingNVS();

    if (success == nvs_ret_t::NVS_FAIL) {
        return nvs_ret_t::CHECKSUM_FAIL;
    } else {
        return nvs_ret_t::CHECKSUM_OK;
    }
}

// Called in the readFromNVS method. NVS already runs CRC32, but there is an
// additional redundancy built in below. Gets the checksum value stored in 
// NVS, if an error occurs, it will memset the stored checksum to 0 and return
// failure code. Nex the checksum based on the passed data is computed, and compared
// with the stored check sum. Returns either CHECKSUM_FAIL or CHECKSUM_OK.
nvs_ret_t NVSctrl::getChecksum(const char* key, const uint8_t* data, size_t size) {

    if(this->basicErrCheck(key, data, size) == nvs_ret_t::NVS_FAIL) {
        return nvs_ret_t::NVS_FAIL;
    }

    char newKey[15]{0}; // max size
    snprintf(newKey, sizeof(newKey), "CS%s", key);

    uint32_t storedChecksum{0};
    this->err = nvs_get_u32(this->handle, newKey, &storedChecksum);

    nvs_ret_t success = this->errHandlingNVS();

    if (success == nvs_ret_t::NVS_FAIL) {
        memset(&storedChecksum, 0, sizeof(storedChecksum));
        return nvs_ret_t::CHECKSUM_FAIL;
    } 

    uint32_t computedCS = this->computeChecksum(data, size);

    if (storedChecksum == computedCS && this->isCheckSumSafe) {
        return nvs_ret_t::CHECKSUM_OK;
    } else {
        this->sendErr("NVS Checksum mismatch, corrupt or missing", errDisp::SRL);
        return nvs_ret_t::CHECKSUM_FAIL;
    }
}

// (VERBOSE COMMENTS DUE TO COMPLEXITY)
// Uses the Cyclic Redundancy Check 32 (CRC32) to detect for 
// errors and corruption of data. See CPP references on CRC32
// for very detailed description. Returns uint32_t.
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