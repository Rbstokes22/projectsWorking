#include "NVS2/NVS.hpp"
#include "nvs.h"
#include "nvs_flash.h"
#include "string.h"

namespace NVS {

// Requires key, uint8_t pointer to data to be written, and the size in bytes.
// Reads first from the NVS to ensure an overwrite doesnt occur in the memory
// since NVS is subject to wear due to writing. If the data to write does 
// not equal the data that is read, it will be re-written to include the 
// checksum value. Returns NVS_WRITE_OK if data is successfully written,
// or NVS_WRITE_FAIL if not.
nvs_ret_t NVSctrl::writeToNVS(
    const char* key, 
    uint8_t* data, 
    size_t bytes) {

    const char* TAG = "NVS Write";

    // first read and then write if different
    uint8_t tempData[MAX_NVS_ENTRY](0);
    char keyCS[MAX_NAMESPACE](0);

    NVS_SAFE_OPEN(this->conf);// RAII open NVS. Closes once out of scope.

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
        
        default:
        break;
    }

    // If new data is set to be written, will begin here.
    esp_err_t write = nvs_set_blob(this->conf.handle, key, data, bytes); 
    bool dataSafe = false;
    uint32_t crc = 0;

    switch (write) { 
        case ESP_OK: {
            int written = snprintf(keyCS, MAX_NAMESPACE, "CS%s", key);

            // Warns about truncated values if the total size written exceeds the 
            // maximum allowed chars. A key of 12 chars, which will be prepended to
            // 14 chars + 1 for the null terminator is the requirement. If 17 chars
            // are to be written, + 1 for the null term to equal 18, then the key
            // exceed the requirment by 5 chars. 17 - 15 + 3 = 5.
            if (written > MAX_NAMESPACE) {
                printf("%s: WARNING, checksum key has been "
                    "truncated due to exceeding char limits by %d\n", 
                    TAG, written - MAX_NAMESPACE + 3); 
            }
        }
        
        crc = this->crc32(data, bytes, dataSafe); // Compute crc32.
        break;

        default:
        this->defaultPrint(TAG, write);
    }

    if (dataSafe) {
        // Doesn't require a read before write. This will only be called
        // if the value in memory has changed, which implies the checksum
        // has also changed.
        write = nvs_set_u32(this->conf.handle, keyCS, crc);
    } else {
        return nvs_ret_t::NVS_WRITE_FAIL; 
    }

    switch (write) {
        case ESP_OK:
        return nvs_ret_t::NVS_WRITE_OK;
        break;

        default:
        this->defaultPrint(TAG, write);
    }

    return nvs_ret_t::NVS_WRITE_FAIL;


}

// Requires:
// - Key, const char, not to exceed 13 chars, 12 + null terminator.
// - Data, void pointer of any type.
// - Bytes, pass this data using sizeof(data) ONLY.
// Data will be qual to 0 if an error occurs.
// WARNING!!! To avoid program crashes, use sizeof(data).
// Writes the data passed, to the NVS using the key. Returns NVS_WRITE_OK,
// NVS_WRITE_FAIL, NVS_WRITE_BAD_PARAMS if nullptrs are passed or bytes = 0,
// and NVS_KEYLENGTH_ERROR if key is too short or too long.
nvs_ret_t NVSctrl::write(const char* key, void* data, size_t bytes) {

    const char* TAG = "NVS Write";

    // Filters out garbage data and returns if bad.
    if (key == nullptr || data == nullptr || bytes == 0) {
        printf("%s: nullptr passed or bytes = 0\n", TAG);
        return nvs_ret_t::NVS_WRITE_BAD_PARAMS;
    }

    // Filters key data first to ensure length is within bounds.
    // Max is 12 chars, plus the null term.
    size_t keyLen = strlen(key); 
    if (keyLen >= (MAX_NAMESPACE - 3) || keyLen == 0) {
        printf("%s: Key length out of scope at %zu chars long\n", TAG, keyLen);
        return nvs_ret_t::NVS_KEYLENGTH_ERROR;
    } 

    // Converts void data into uint8_t data in preparation for the 
    // NVS blob write. No concerns about data going out of scope 
    // since caller will be expecting a return.
    uint8_t* toWrite = reinterpret_cast<uint8_t*>(data);
    return writeToNVS(key, toWrite, bytes);
}

}