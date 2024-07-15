#ifndef NVS_HPP
#define NVS_HPP

// This header includes several templates and they are both declared and 
// defined within this file which makes it lengthy.

#include "nvs_flash.h"
#include "nvs.h"
#include "UI/MsgLogHandler.hpp"
#include "string.h"

namespace NVS {

enum class data_t {
    INT8_T, INT16_T, INT32_T, INT64_T,
    UINT8_T, UINT16_T , UINT32_T, UINT64_T,
    FLOAT, DOUBLE, CHAR, OTHER
};

enum class nvs_ret_t {
    NVS_OK, NVS_FAIL, NVS_NEW_ENTRY, CHECKSUM_OK, CHECKSUM_FAIL,
    NVS_OPEN, NVS_CLOSED, NVS_READ_OK, NVS_READ_FAIL,
    NVS_WRITE_OK, NVS_WRITE_FAIL
};

extern const uint8_t dataSize[];

class NVSctrl {
    private:
    nvs_handle_t handle;
    esp_err_t err;
    Messaging::MsgLogHandler &msglogerr;
    const char* nameSpace; 
    bool isCheckSumSafe; // maybe delete
    nvs_ret_t NVSopen;
    static const size_t maxEntry;
    void sendErr(const char* msg, bool toOLED = false);
    nvs_ret_t basicErrCheck(const char* key, const void* data, size_t size);
    nvs_ret_t errHandlingNVS();
    nvs_ret_t safeStart();
    nvs_ret_t safeClose(); 
    nvs_ret_t writeToNVS(const char* key, const void* data, size_t size, bool isChar = false);
    nvs_ret_t readFromNVS(const char* key, void* carrier, size_t size, bool isChar = false);
    uint32_t computeChecksum(const uint8_t* data, size_t size);
    nvs_ret_t setChecksum(const char* key, const uint8_t* data, size_t size);
    nvs_ret_t getChecksum(const char* key, const uint8_t* data, size_t size);
    
    public:
    NVSctrl(Messaging::MsgLogHandler &msglogerr, const char nameSpace[12]);
    nvs_ret_t eraseAll();
    nvs_ret_t write(const char* key, const void* data, size_t size, bool isChar = false);

    // This is exclusive to arrays of known types, so no structs or classes. The length
    // and not size is passed, allowing the appropriate size to be computed based on 
    // the data type. When using char arrays use strlen as the length, for others,
    // put the exact size. Returns NVS_WRITE_OK and NVS_WRITE_FAIL.
    template<typename NVSWA>
    nvs_ret_t writeArray(const char* key, data_t dType, const NVSWA* data, size_t length) {
        printf("WRITE ARRAY\n");
        size_t arraySize{0};
        bool isChar{false};
        
        if (length <= 0) {
            this->sendErr("NVS Write, array length must be > 0");
            return nvs_ret_t::NVS_WRITE_FAIL;
        }

        size_t expSize = dataSize[static_cast<int>(dType)]; // expected size

        if (key == nullptr || *key == '\0') {
            this->sendErr("NVS Write, Key is not defined");
            return nvs_ret_t::NVS_WRITE_FAIL;
        }
        
        if (data == nullptr) {
            this->sendErr("NVS Write, data is not defined");
            return nvs_ret_t::NVS_WRITE_FAIL;
        }

        if (sizeof(*data) != expSize) {
            this->sendErr("NVS Write, size of data does not match data_t");
            return nvs_ret_t::NVS_WRITE_FAIL;
        }

        switch(dType) {
            
            // Flags if char array. This is because the size of the char array stored in
            // NVS will be variable but end with a null terminator. When written, the length
            // of the char array is known, when reading, it isnt. This will allow logic to 
            // extract the char array length only, at the checkSum level.
            case data_t::CHAR:
            arraySize = length + 1; // accounts for null terminator
            isChar = true;
            break;

            case data_t::OTHER:
            this->sendErr("NVS Write, only arrays of known datatypes permitted");
            return nvs_ret_t::NVS_WRITE_FAIL; 

            default:
            arraySize = (length * expSize);
        }

        return this->write(key, data, arraySize, isChar);
    }

    // This accepts only types with known sizes such as CHAR, INT32_T, etc...
    // Passes the appropriate data and sizes to write method. Returns 
    // NVS_WRITE_OK and NVS_WRITE_FAIL.
    template<typename NVSWN>
    nvs_ret_t writeNum(const char* key, data_t dType, const NVSWN &data) {
        printf("WRITE NUM\n");
        if (key == nullptr || *key == '\0') {
            this->sendErr("NVS Write, Key is not defined");
            return nvs_ret_t::NVS_WRITE_FAIL;
        }

        // No function purpose beside consistency and and enforcing correct 
        // function use.
        if (dType == data_t::OTHER) {
            this->sendErr("NVS Write, must be a number");
            return nvs_ret_t::NVS_READ_FAIL;
        } else {
            if (sizeof(data) == dataSize[static_cast<int>(dType)]) {
                return this->write(key, &data, sizeof(data));
            } else {
                this->sendErr("NVS Write, size of data does not match data_t");
                return nvs_ret_t::NVS_WRITE_FAIL;
            }
        }
    }

    // The accepts only the OTHER keyword, which can be any type. This is meant
    // for objects like structs. This can be used for all non-array objects and 
    // is less strict then the writeNum method. returns NVS_WRITE_OK and 
    // NVS_WRITE_FAIL.
    template<typename NVSWO>
    nvs_ret_t writeOther(const char* key, data_t dType, const NVSWO &data) {
        printf("WRITE OTHER\n");
        if (key == nullptr || *key == '\0') {
            this->sendErr("NVS Write, Key is not defined");
            return nvs_ret_t::NVS_WRITE_FAIL;
        }

        // Enforces correct function use. 
        if (dType != data_t::OTHER) {
            this->sendErr("NVS Write, must be of type other");
            return nvs_ret_t::NVS_WRITE_FAIL;
        } else {
            return this->write(key, &data, sizeof(data));
        }
    }

    // Main read function that is called by the more detailed function depending on 
    // data type. This will inboke the readFromNVS, and copy the NVS data to the 
    // passed carrier, assuming is is the right type. Returns NVS_READ_OK and 
    // NVS_READ_FAIL.
    template<typename NVSR>
    nvs_ret_t read(const char* key, NVSR* carrier, size_t size, bool isChar = false) {
        printf("READ\n");
        if (key == nullptr || *key == '\0') {
            this->sendErr("NVS Read, improper key passed to NVS");
            return nvs_ret_t::NVS_READ_FAIL;
        }

        if (carrier == nullptr) {
            this->sendErr("NVS Read, improper carrier passed to NVS"); 
            return nvs_ret_t::NVS_READ_FAIL;
        }

         // All opening and closing of the NVS exist in the read and write methods. This 
        // is to allow the code to run in its entirety and to simplify everything.
        if (this->safeStart() == nvs_ret_t::NVS_OPEN) {
            if (this->readFromNVS(key, carrier, size) == nvs_ret_t::NVS_READ_OK) {
                this->safeClose();
                return nvs_ret_t::NVS_READ_OK;
            } else {
                this->safeClose();
                return nvs_ret_t::NVS_READ_FAIL;
            }

        } else {
            this->safeClose();
            return nvs_ret_t::NVS_READ_FAIL;
        }
    }

    // Pass array by pointer, along with size of carrier. This allows the 
    // read function to fill that array with the correct data requested. 
    // Ensure to include the space for a null terminator if requesting a 
    // char array. For size, use sizeof(carrier);
    template<typename NVSRA>
    nvs_ret_t readArray(const char* key, data_t dType, NVSRA *carrier, size_t size) {
        printf("READ ARRAY\n");
        if (size <= 0) {
            this->sendErr("NVS Read, size must be > 0");
            return nvs_ret_t::NVS_READ_FAIL;
        }

        if (key == nullptr || *key == '\0') {
            this->sendErr("NVS Read, improper key passed to NVS"); 
            return nvs_ret_t::NVS_READ_FAIL;
        }

        if (carrier == nullptr) {
            this->sendErr("NVS Read, carrier must be a valid array");
            return nvs_ret_t::NVS_READ_FAIL;
        }

        // Flags if char array. This is because the size of the char array stored in
        // NVS will be variable but end with a null terminator. When written, the length
        // of the char array is known, when reading, it isnt. This will allow logic to 
        // extract the char array length only, at the checkSum level.
        if (dType == data_t::CHAR) {
            return this->read(key, carrier, size, true);
        } else {
            return this->read(key, carrier, size);
        }
    }

    // Used for all know data type sizes. Carrier is passed by reference 
    // to calculate the size, and it is sent to the read method to fill 
    // the carrier with requested data. 
    template<typename NVSRN>
    nvs_ret_t readNum(const char* key, data_t dType, NVSRN &carrier) {
        printf("READ NUM\n");
        if (key == nullptr || *key == '\0') {
            this->sendErr("NVS Read, improper key passed to NVS"); 
            return nvs_ret_t::NVS_READ_FAIL;
        }

        // Extra enforcement to ensure the correct type of number
        if (dType == data_t::OTHER) {
            this->sendErr("NVS Read, data_t must be number");
            return nvs_ret_t::NVS_READ_FAIL;
        } else {
            if (sizeof(carrier) == dataSize[static_cast<int>(dType)]) {
                return this->read(key, &carrier, sizeof(carrier));
            } else {
                this->sendErr("NVS Read, size of carrier does not match data_t");
                return nvs_ret_t::NVS_READ_FAIL;
            }
        }
    }

    // Mostly used for structs, but can be use for all non array types.
    // carrier is passed by refernce to calculate the size, and then 
    // sent to the read method to fill the carrier with requested data.
    template<typename NVSRO>
    nvs_ret_t readOther(const char* key, data_t dType, NVSRO &carrier){
        printf("READ OTHER\n");
        if (key == nullptr || *key == '\0') {
            this->sendErr("NVS Read, improper key passed to NVS"); 
            return nvs_ret_t::NVS_READ_FAIL;
        }

        if (dType != data_t::OTHER) {
            this->sendErr("NVS Read, must be of type OTHER");
            return nvs_ret_t::NVS_READ_FAIL;
        } else {
            return this->read(key, &carrier, sizeof(carrier));
        } 
    }
};

}


#endif // NVS_HPP