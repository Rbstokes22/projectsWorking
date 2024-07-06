#include "NVS/NVS.h"

namespace NVS {

// Writes to NVS by taking data, casting it to a byte aray, and writing
// it as bytes into the NVS. This is cut and dry for all types and the
// size will be precise.
bool NVSctrl::writeToNVS(const char* key, const void* data, size_t size, bool isChar) {
    if(!this->basicErrCheck(key, data, size)) return false;

    // Creates a temp object to be filled by the byte data in NVS
    // sending temp as a carrier.
    uint8_t temp[128]{0}; // should never exceed anything close to this, mod if needed

    if (!this->readFromNVS(key, temp, size, isChar)) {
        this->sendErr("NVS Read, read error"); return false;
    }

    // Casts the passed data to a pointer to its array of byte data.
    const uint8_t* bytes = static_cast<const uint8_t*>(data);

    // Compares memory from each byte array, if equal, it does not
    // overwrite data to NVS. If unequal, returns the bytes written
    // compared with the size requested.
    if (memcmp(bytes, temp, size) == 0) {
        this->prefs.end();
        return true; // indicating no write required, data exists.
    } else {
        if (this->prefs.putBytes(key, bytes, size) == size) {
            if (this->setChecksum(key, bytes, size)) {
                return true;
            } else {
                return false; // error in setChecksum method.
            }

        } else {
            this->sendErr("NVS Data not written");
            return false;
        }
    }
}

// Sends refined data to be written to NVS. This method can be used directly,
// with the other methods simplifying the process. In the event of a unique type
// such as an array of structs, you can write directly here by passing the 
// data by pointer and including the size.
bool NVSctrl::write(const char* key, const void* data, size_t size, bool isChar) {

    // ERROR CHECKS
    if (key == nullptr || *key == '\0') {
        this->sendErr("NVS Write, Key is not defined");
        return false;
    }

    if (data == nullptr) {
        this->sendErr("NVS Write, data cannot be a nullptr");
        return false;
    } 

    if (size <= 0) {
        this->sendErr("NVS Write, size must be > 0");
        return false;
    }

    if (!this->prefs.begin(this->nameSpace)) {
        this->sendErr("NVS Write, did not begin"); 
        return false;
    }

    // end called in write checksum, or called during any error
    return this->writeToNVS(key, data, size, isChar);
}

// This is exclusive to arrays of known types. This does not include arrays of structs,
// or anything like that. Those must be written directly using the write method.
// Automatically account for a char array null terminator when using CHAR, as well as 
// other sizes based on their type (i.e. INT16_T = 2); When using char arrays, use 
// strlen for the length argument, for others, put the exact size.
template<typename NVSWA>
bool NVSctrl::writeArray(const char* key, const NVSWA* data, DType dType, size_t length) {
    size_t arraySize{0};
    bool isChar{false};
    
    if (length <= 0) {
        this->sendErr("NVS Write, array length must be > 0", false);
        return false;
    }

    size_t expSize = dataSize[static_cast<int>(dType)];

    if (key == nullptr || *key == '\0') {
        this->sendErr("NVS Write, Key is not defined", false);
        return false;
    }
    
    if (data == nullptr) {
        this->sendErr("NVS Write, data is not defined", false);
        return false;
    }

    if (sizeof(*data) != expSize) {
        this->sendErr("NVS Write, size of data does not match DType", false);
        return false;
    }

    switch(dType) {
        
        // Flags if char array. This is because the size of the char array stored in
        // NVS will be variable but end with a null terminator. When written, the length
        // of the char array is known, when reading, it isnt. This will allow logic to 
        // extract the char array length only, at the checkSum level.
        case DType::CHAR:
        arraySize = length + 1; // accounts for null terminator
        isChar = true;
        break;

        case DType::OTHER:
        this->sendErr("NVS Write, only arrays of known datatypes permitted", false);
        return false; 

        default:
        arraySize = (length * expSize);
    }

    return this->write(key, data, arraySize, isChar);
}

// The accepts only the OTHER keyword, which can be any type. This is meant
// for objects like structs. This can be used for all non-array objects and 
// is less strict then the writeNum method.
template<typename NVSWO>
bool NVSctrl::writeOther(const char* key, const NVSWO &data, DType dType) {
    if (key == nullptr || *key == '\0') {
        this->sendErr("NVS Write, Key is not defined", false);
        return false;
    }

    // Enforces correct function use. 
    if (dType != DType::OTHER) {
        this->sendErr("NVS Write, must be of type other", false);
        return false;
    } else {
        return this->write(key, &data, sizeof(data));
    }
}

// This accepts only types with known sizes such as CHAR, INT32_T, etc...
// Passes the appropriate data and sizes to write method.
template<typename NVSWN>
bool NVSctrl::writeNum(const char* key, const NVSWN &data, DType dType) {
    if (key == nullptr || *key == '\0') {
        this->sendErr("NVS Write, Key is not defined", false);
        return false;
    }

    // No function purpose beside consistency and and enforcing correct 
    // function use.
    if (dType == DType::OTHER) {
        this->sendErr("NVS Write, must be a number", false);
        return false;
    } else {
        if (sizeof(data) == dataSize[static_cast<int>(dType)]) {
            return this->write(key, &data, sizeof(data));
        } else {
            this->sendErr("NVS Write, size of data does not match DType", false);
            return false;
        }
    }
}
    
}