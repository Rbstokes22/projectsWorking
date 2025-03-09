#include "NVS2/NVS.hpp"
#include "nvs.h"
#include "nvs_flash.h"
#include "string.h"
#include "UI/MsgLogHandler.hpp"

namespace NVS {

// NO LOGGING FOR SUCCESSFUL READS TO AVOID LOGGING POLLUTION.

// Requires key, uint8_t pointer to data carrier, size in bytes, and bool to
// indicate if it is being called from an internal writing function, which is
// default to false. Populates carrier with requested data, and runs a 
// checksum with it and the stored crc32 value. Returns NVS_READ_OK if data 
// is good to use, NVS_NEW_ENTRY iff a new value is being written to the NVS
// indicating that it has no data to compare against, and NVS_READ_FAIL for 
// an unsuccessful read.
nvs_ret_t NVSctrl::readFromNVS(const char* key, uint8_t* carrier, size_t bytes,
    bool fromWrite) {
    
    NVS_SAFE_OPEN(this, this->conf); // RAII open NVS. Closes once out of scope.
    
    esp_err_t read = nvs_get_blob(this->conf.handle, key, carrier, &bytes);

    switch (read) {
        case ESP_OK: 

        // Checks the stored checksum against a checksum of the carrier.
        // If matched, returns NVS_READ_OK. Checksum errors handled within
        // its function.
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
            snprintf(this->log, sizeof(this->log),
                "%s new entry from key %s", this->tag, key);

            this->sendErr(this->log, Messaging::Levels::INFO);
            return nvs_ret_t::NVS_NEW_ENTRY;
        }

        // If not new entry, key must not exist. This will return a read fail.
        snprintf(this->log, sizeof(this->log),
            "%s requested key: %s does not exist", this->tag, key);

        this->sendErr(this->log);
        break;

        default: // Other read error.
        snprintf(this->log, sizeof(this->log),
            "%s requested key: %s read error", this->tag, key);

        this->sendErr(this->log);
        break;
    }

    memset(carrier, 0, bytes); // If error, memsets to be safe and zero out.
    return nvs_ret_t::NVS_READ_FAIL;
}

// Requires:
// - Key, const char, not to exceed 13 chars, 12 + null terminator.
// - Carrier, void pointer of any type.
// - Bytes, pass this data using sizeof(data) ONLY.
// Data will be qual to 0 if an error occurs.
// WARNING!!! To avoid program crashes, use sizeof(data).
// WARNING!!! Passing the wrong carrier type can lead to garbage data.
// Reads the NVS data stored from the key, and populates your carrier with 
// the required data bytes. Returns NVS_READ_OK, NVS_READ_FAIL, 
// NVS_KEYLENGTH_ERROR if key is too short or long, or NVS_READ_BAD_PARAMS if 
// nullptrs are passed, or bytes = 0.
nvs_ret_t NVSctrl::read(const char* key, void* carrier, size_t bytes) {

    memset(carrier, 0, bytes); // Zeros out upon receipt.

    // Filters out garbage data and returns if bad.
    if (key == nullptr || carrier == nullptr || bytes == 0) {
        snprintf(this->log, sizeof(this->log),
            "%s nullptr passed or bytes = 0", this->tag);

        this->sendErr(this->log);
        return nvs_ret_t::NVS_READ_BAD_PARAMS;
    }

    // Filters key data first to ensure length is within bounds.
    // Max is 12 chars, plus the null term.
    size_t keyLen = strlen(key); 
    if (keyLen >= (MAX_NAMESPACE - 3) || keyLen == 0) {

        snprintf(this->log, sizeof(this->log),
            "%s keylen out of scope at %zu chars long", this->tag, keyLen);

        this->sendErr(this->log);
        return nvs_ret_t::NVS_KEYLENGTH_ERROR;
    } 

    // Converts void data into uint8_t in preparation for the 
    // NVS blob read. No concerns about data going out of scope
    // since caller will be expecting a return.
    uint8_t* toRead = reinterpret_cast<uint8_t*>(carrier);
    return readFromNVS(key, toRead, bytes);
}

}