#include "NVS2/NVS.hpp"
#include "nvs.h"
#include "string.h"

namespace NVS {

// Requires key, pointer to uint8_t data, and size in bytes. Runs a comparison
// of the crc32 of the actual data, and compares it with what is stored in
// the NVS. Upon the two values being equal, returns CHECKSUM_OK. If unequal
// or data is corrupt, returns CHECKSUM_FAIL.
nvs_ret_t NVSctrl::checkCRC(const char* key, uint8_t* data, size_t bytes) {

    // used because this will return 0xFFFFFFFF if there is an error, which
    // will also set this variable to false. Only good data if true.
    bool dataSafe = false; 
    uint32_t crcStored = 0;

    char keyCS[MAX_NAMESPACE]; // Checksum key with "CS" prepended.
    int written = snprintf(keyCS, MAX_NAMESPACE, "CS%s", key);

    // Warns about truncated values if the total size written exceeds the 
    // maximum allowed chars. A key of 12 chars, which will be prepended to
    // 14 chars + 1 for the null terminator is the requirement. If 17 chars
    // are to be written, + 1 for the null term to equal 18, then the key
    // exceed the requirment by 5 chars. 17 - 15 + 3 = 5.
    if (written > MAX_NAMESPACE) {
        snprintf(this->log, sizeof(this->log), 
            "%s checksum key [%s] has been truncated due to exceeding char"
            " limits by %d", this->tag, keyCS, (written - MAX_NAMESPACE + 3));

        this->sendErr(this->log);
    }

    uint32_t CSactual = this->crc32(data, bytes, dataSafe);
    esp_err_t CSread = nvs_get_u32(this->conf.handle, keyCS, &crcStored);

    switch (CSread) {

        case ESP_OK:

        if (CSactual != crcStored || !dataSafe) { // Compares the two values
            
            snprintf(this->log, sizeof(this->log), 
                "%s Checksum error key [%s], data potentially corrupt." 
                " Act = %lu, Stored = %lu. dataSafe = %d", 
                this->tag, keyCS, CSactual, crcStored, dataSafe);

            this->sendErr(this->log);
        } 

        return nvs_ret_t::CHECKSUM_OK; // Implies that checksum matches stored.
        break;

        default: 
        snprintf(this->log, sizeof(this->log), "%s checksum key [%s] not read", 
            this->tag, keyCS);

        this->sendErr(this->log);
    }

    return nvs_ret_t::CHECKSUM_FAIL;
}

// Requires pointer to uint8_t data, size of data, and reference to boolean
// dataSafe. dataSafe is a redundancy to ensure that the return value is not
// an error. Computes checksum using cyclinc redundancy check (CRC) 32 bits,
// and returns value along with updated dataSafe boolean.
uint32_t NVSctrl::crc32(const uint8_t* data, size_t bytes, bool &dataSafe) {

    // If bad data is sent to checksum, returns a max value and
    // sets the flag for bad data.
    if (data == nullptr || bytes == 0) {
        dataSafe = false;
        return 0xFFFFFFFF;

    } else {

        // Inits the crc32 at max value to ensure all leading bits 
        // are set which helps process data starting with zeros.
        uint32_t crc = 0xFFFFFFFF;
        static const uint32_t polynomial = 0xEDB88320;

        // Each byte is iterated for the size passed, which is why
        // the correct size is crucial.
        for(size_t addr = 0; addr < bytes; addr++) {

            // The crc32 is then xor with the byte data, which is a method
            // to integrate the bytes into the crc32 in a non destructive 
            // way. Everything is reversable and bits. This iteration
            // goes bit by bit shifting  or shifting and xoring with the 
            // polynomial when the least significant bit == 1. 
            crc ^= data[addr];

            for(size_t bit = 0; bit < 8; bit++) {
                if (crc & 1) {
                    crc = (crc >> 1) ^ polynomial;
                } else {
                    crc >>= 1;
                }
            }
        }

        dataSafe = true;
        // Ensures it does not return 0 with a 0 input.
        return crc ^ 0xFFFFFFFF;
    }
}
    
}