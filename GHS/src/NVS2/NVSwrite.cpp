#include "NVS2/NVS.hpp"
#include "nvs.h"
#include "nvs_flash.h"
#include "string.h"

namespace NVS {

nvs_ret_t NVSctrl::writeToNVS(
    const char* key, 
    uint8_t* data, 
    size_t bytes) {

    const char* TAG = "NVS Write";

    // first read and then write if different
    uint8_t tempData[MAX_NVS_ENTRY](0);
    char keyCS[MAX_NAMESPACE](0);

    nvs_ret_t readFirst = this->readFromNVS(key, tempData, bytes, true);

    switch (readFirst) {
        case nvs_ret_t::NVS_READ_OK: 

        // Compares the data passed to the data stored in that location on
        // the NVS. If the data is equal, returns to prevent writing wear.
        if (memcmp(data, tempData, bytes) == 0) {
            return nvs_ret_t::NVS_WRITE_OK;
        } 

        break;

        case nvs_ret_t::NVS_NEW_ENTRY: 
        // Requires a fresh write below. (Spaceholder)
        break;

        case nvs_ret_t::NVS_READ_FAIL: // reject
        return nvs_ret_t::NVS_WRITE_FAIL;
        break;
    }

    esp_err_t write = nvs_set_blob(this->conf.handle, key, data, bytes); 
    esp_err_t writeCS(ESP_FAIL);

    switch (write) { // START BACK HERE !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        case ESP_OK:
        snprintf("CS%s", MAX_NAMESPACE, key);

    }



    return nvs_ret_t::NVS_WRITE_FAIL;


}

// Requires key, data, and data size in bytes. Writes to NVS as blob.
// Returns NVS_WRITE_OK if successful, NVS_WRITE_FAIL if unsuccessful, 
// or NVS_WRITE_BAD_PARAMS if passed parameters are incorrect.
// -WARNING!!! Include byte for null terminator if using char array.
nvs_ret_t NVSctrl::write(const char* key, void* data, size_t bytes) {

    const char* TAG = "NVS Write";

    // Filters out garbage data and returns if bad.
    if (key == nullptr || data == nullptr || bytes == 0) {
        printf("%s: nullptr passed or bytes = 0\n", TAG);
        return nvs_ret_t::NVS_WRITE_BAD_PARAMS;
    }

    // Converts void data into uint8_t data in preparation for the 
    // NVS blob write. No concerns about data going out of scope 
    // since caller will be expecting a return.
    uint8_t* toWrite = reinterpret_cast<uint8_t*>(data);
    return writeToNVS(key, toWrite, bytes);
}



}