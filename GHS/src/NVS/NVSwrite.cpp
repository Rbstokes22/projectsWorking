#include "NVS/NVS.hpp"
#include "nvs.h"
#include "string.h"

namespace NVS {

// Writes to NVS by taking data, casting it to a byte aray, and writing
// it as bytes into the NVS. This is cut and dry for all types and the
// size will be precise. Returns NVS_WRITE_OK and NVS_WRITE_FAIL.
nvs_ret_t NVSctrl::writeToNVS(const char* key, const void* data, size_t size, bool isChar) {

    bool isNewEntry{false}; // allows bypass of checksum fail on new entries.

    if(this->basicErrCheck(key, data, size) == nvs_ret_t::NVS_FAIL) {
        return nvs_ret_t::NVS_WRITE_FAIL;
    }

    // Creates a temp object to be filled by the byte data in NVS
    // sending temp as a carrier. 
    uint8_t temp[NVSctrl::maxEntry]{0}; 

    // If readFromNVS is bad due to being a new entry, it 
    // will not block the remaining code.
    nvs_ret_t success = this->readFromNVS(key, temp, size, isChar);

    if (success != nvs_ret_t::NVS_READ_OK) {
        return nvs_ret_t::NVS_WRITE_FAIL;
    }

    // Casts the passed data to a pointer to its array of byte data.
    const uint8_t* bytes = static_cast<const uint8_t*>(data);

    // Compares memory from each byte array, if equal, it does not
    // overwrite data to NVS preventing wear.
    if (memcmp(bytes, temp, size) == 0) {
        return nvs_ret_t::NVS_WRITE_OK; 
    } else {

        this->err = nvs_set_blob(this->handle, key, bytes, size);
        nvs_ret_t success = this->errHandlingNVS();

        if (success == nvs_ret_t::NVS_NEW_ENTRY) {
            isNewEntry = true;
        }

        if (success == nvs_ret_t::NVS_FAIL) {
            return nvs_ret_t::NVS_WRITE_FAIL;
        } else {
            if (this->setChecksum(key, bytes, size) == nvs_ret_t::CHECKSUM_OK) {
                return nvs_ret_t::NVS_WRITE_OK;
            } else {
                if (isNewEntry) {return nvs_ret_t::NVS_WRITE_OK;}
                else {return nvs_ret_t::NVS_WRITE_FAIL;}
            }
        }
    }
}

// Sends refined data to be written to NVS. This method can be used directly,
// with the other methods simplifying the process. In the event of a unique type
// such as an array of structs, you can write directly here by passing the 
// data by pointer and including the size. Returns NVS_WRITE_OK and NVS_WRITE_FAIL.
nvs_ret_t NVSctrl::write(const char* key, const void* data, size_t size, bool isChar) {

    // ERROR CHECKS
    if (key == nullptr || *key == '\0') {
        this->sendErr("NVS Write, Key is not defined", errDisp::SRL);
        return nvs_ret_t::NVS_WRITE_FAIL;
    }

    if (data == nullptr) {
        this->sendErr("NVS Write, data cannot be a nullptr", errDisp::SRL);
        return nvs_ret_t::NVS_WRITE_FAIL;
    } 

    if (size <= 0) {
        this->sendErr("NVS Write, size must be > 0", errDisp::SRL);
        return nvs_ret_t::NVS_WRITE_FAIL;
    }

    // All opening and closing of the NVS exist in the read and write methods. This 
    // is to allow the code to run in its entirety and to simplify everything. 
    if (this->safeStart() == nvs_ret_t::NVS_OPEN) {
        if (this->writeToNVS(key, data, size, isChar) == nvs_ret_t::NVS_WRITE_OK) {
            this->safeClose();
            return nvs_ret_t::NVS_WRITE_OK;
        } else {
            this->safeClose();
            return nvs_ret_t::NVS_WRITE_FAIL;
        }
    } else {
        this->safeClose();
        return nvs_ret_t::NVS_WRITE_FAIL;
    }
}
    
}