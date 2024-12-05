#include "NVS2/NVS.hpp"
#include "nvs.h"
#include "nvs_flash.h"
#include "string.h"

namespace NVS {

// Requires key, uint8_t pointer to data carrier, size in bytes, and bool to
// indicate if it is being called from an internal writing function, which is
// default to false. Populates carrier with requested data, and runs a 
// checksum with it and the stored crc32 value. Returns NVS_READ_OK if data 
// is good to use, NVS_NEW_ENTRY iff a new value is being written to the NVS
// indicating that it has no data to compare against, and NVS_READ_FAIL for 
// an unsuccessful read.
nvs_ret_t NVSctrl::readFromNVS(
    const char* key, 
    uint8_t* carrier, 
    size_t bytes,
    bool fromWrite) {

    esp_err_t read = nvs_get_blob(this->conf.handle, key, carrier, &bytes);
    const char* TAG = "NVS Read";

    switch (read) {
        case ESP_OK: 

        // Checks the stored checksum against a checksum of the carrier.
        // If matched, returns NVS_READ_OK.
        if (this->checkCRC(key, carrier, bytes) == nvs_ret_t::CHECKSUM_OK) {
            return nvs_ret_t::NVS_READ_OK;
        }
        
        return nvs_ret_t::NVS_READ_FAIL;
        break;

        case ESP_ERR_NVS_NOT_FOUND: 

        // Before data is written to the NVS, a read is attempted to 
        // ensure that duplicate data is not written to the NVS. This
        // help prevent wear on the NVS system. If during the write,
        // the key is passed to the read, and does not yet exist, it
        // is flagged as a new entry. Typically there is checksum 
        // verification which is bypassed in this case.
        if (fromWrite) {
            printf("%s: New Entry\n", TAG);
            return nvs_ret_t::NVS_NEW_ENTRY;
        }

        printf("%s: Requested key does not exist\n", TAG);
        break;

        default: // Print statements that require no action.
        NVSctrl::defaultPrint(TAG, read);
        break;
    }

    return nvs_ret_t::NVS_READ_FAIL;
}

// START HERE AND FIX NOTES BELOW. Ensure that we include filters for too short of key !!!!!!!!!!!!!!!!!!!!!
// In the read and write methods. Ensure that we are not doubling down on errors
// meaning if checksum has an error, the same error isnt called in read or write.
// 






// Requires key (MAX 13 chars with null terminator), data, and data 
// size in bytes. Reads from the NVS as blob to the carrier, exactly 
// as stored. WARNING! if you pass the wrong carrier type, you may 
// receive garbage data. Returns NVS_READ_OK if successful, 
// NVS_READ_FAIL if unsuccessful, NVS_READ_BAD_PARAMS if passed 
// parameters are incorrect, or NVS_KEY_TOO_LONG if key exceeds
// 13 chars with the null terminator.
// -Value(s) will be equal to 0 if error.
// -WARNING!!! Include byte for null terminator if using char array.
nvs_ret_t NVSctrl::read(const char* key, void* carrier, size_t bytes) {
    const char* TAG = "NVS Read";

    memset(carrier, 0, bytes); // Zeros out.

    if (strlen(key) >= MAX_NAMESPACE - 3) { // Leave room for null Term
        return nvs_ret_t::NVS_KEY_TOO_LONG;
    }

    // Filters out garbage data and returns if bad.
    if (key == nullptr || carrier == nullptr || bytes == 0) {
        printf("%s: nullptr passed or bytes = 0\n", TAG);
        return nvs_ret_t::NVS_READ_BAD_PARAMS;
    }

    // Converts void data into uint8_t in preparation for the 
    // NVS blob read. No concerns about data going out of scope
    // since caller will be expecting a return.
    uint8_t* toRead = reinterpret_cast<uint8_t*>(carrier);
    return readFromNVS(key, toRead, bytes);
}

}