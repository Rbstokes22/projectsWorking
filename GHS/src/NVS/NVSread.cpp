#include "NVS/NVS.hpp"
#include "UI/MsgLogHandler.hpp"
#include "nvs.h"
#include "string.h"

namespace NVS {

// When reading from the NVS, an explicit size is called, with the exception of
// char arrays. The flag isChar will allow the byte data to be reinterpreted
// as const char* data, to get a string length. The null terminator is always set
// for every char array written to NVS, due to the + 1 write. Returns 
// NVS_READ_OK and NVS_READ_FAIL.
nvs_ret_t NVSctrl::readFromNVS(const char* key, void* carrier, size_t size, bool isChar) {

    // allows bypass of checksum fail on new entries. This is because before anything 
    // is written to the NVS, it is first read to ensure it doesn't already exist.
    bool isNewEntry{false}; 

    if(this->basicErrCheck(key, carrier, size) == nvs_ret_t::NVS_FAIL) {
        return nvs_ret_t::NVS_READ_FAIL;
    }

    uint8_t* bytes = static_cast<uint8_t*>(carrier);
    memset(bytes, 0, size); // Clear to 0 to ensure null term.

    this->err = nvs_get_blob(this->handle, key, bytes, &size);

    nvs_ret_t success = this->errHandlingNVS();

    // memsets both failure and new entry, new entries will show
    // 0 values, as a default return type. New entries will have 
    // no data, so they are non blocking by nature.
    if (success == nvs_ret_t::NVS_FAIL) {
        memset(carrier, 0, size); 
        return nvs_ret_t::NVS_READ_FAIL;
    } else if (success == nvs_ret_t::NVS_NEW_ENTRY) {
        isNewEntry = true; // Non block
        memset(carrier, 0, size); 
    }

    if (isChar) { 
        size_t strLgth = strlen(reinterpret_cast<const char*>(bytes));
        size = strLgth + 1;
    } 

    if (this->getChecksum(key, bytes, size) == nvs_ret_t::CHECKSUM_OK) {
        return nvs_ret_t::NVS_READ_OK;
    } else {
        if (isNewEntry) {return nvs_ret_t::NVS_READ_OK;}
        else {return nvs_ret_t::NVS_READ_FAIL;}
    }
}

}

