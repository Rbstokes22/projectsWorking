#include "NVS/NVS.h"

namespace NVS {

// When reading from the NVS, an explicit size is called, with the exception of
// char arrays. The flag isChar will allow the byte data to be reinterpreted
// as const char* data, to get a string length. The null terminator is always set
// for every char array written to NVS, due to the +1 write. 
bool NVSctrl::readFromNVS(const char* key, void* carrier, size_t size, bool isChar) {
    if(!this->basicErrCheck(key, carrier, size)) return false;

    uint8_t* bytes = static_cast<uint8_t*>(carrier);
    memset(bytes, 0, size); // Clear to 0 to ensure null term.
    size_t bytesRead = this->prefs.getBytes(key, bytes, size);

    // if Char, gets the strlen and changes the size to = it + 1 to 
    // acct for null terminator.
    if (isChar) { 
        size_t strLgth = strlen(reinterpret_cast<const char*>(bytes));
        size = strLgth + 1;
    } 

    if (this->getChecksum(key, bytes, size)) {
        return bytesRead > 0 ? true : false; 
    } else {
        return false; // error sent in getChecksum method.
    }
}

// Main read function that is called by the more detailed function, similar to
// the write methods. 
template<typename NVSR>
bool NVSctrl::read(const char* key, NVSR* carrier, size_t size, bool isChar) {
    
    if (!this->prefs.begin(this->nameSpace)) {
        this->sendErr("NVS Read, did not begin"); 
        return false;
    }

    if (key == nullptr || *key == '\0') {
        this->sendErr("NVS Read, improper key passed to NVS"); 
        return false;
    }

    if (carrier == nullptr) {
        this->sendErr("NVS Read, improper carrier passed to NVS"); 
        return false;
    }

    // end called in read checksum().
    return this->readFromNVS(key, carrier, size);
}

// Used for all know data type sizes. Carrier is passed by reference 
// to calculate the size, and it is sent to the read method to fill 
// the carrier with requested data. 
template<typename NVSRN>
bool NVSctrl::readNum(const char* key, data_t dType, NVSRN &carrier) {
    if (key == nullptr || *key == '\0') {
        this->sendErr("NVS Read, improper key passed to NVS"); 
        return false;
    }

    // Extra enforcement to ensure the correct type of number
    if (dType == data_t::OTHER) {
        this->sendErr("NVS Read, data_t must be number");
        return false;
    } else {
        if (sizeof(carrier) == dataSize[static_cast<int>(dType)]) {
            return this->read(key, &carrier, sizeof(carrier));
        } else {
            this->sendErr("NVS Read, size of carrier does not match data_t");
            return false;
        }
    }
}

// Mostly used for structs, but can be use for all non array types.
// carrier is passed by refernce to calculate the size, and then 
// sent to the read method to fill the carrier with requested data.
template<typename NVSRO>
bool NVSctrl::readOther(const char* key, data_t dType, NVSRO &carrier){
    if (key == nullptr || *key == '\0') {
        this->sendErr("NVS Read, improper key passed to NVS"); 
        return false;
    }

    if (dType != data_t::OTHER) {
        this->sendErr("NVS Read, must be of type OTHER");
        return false;
    } else {
        return this->read(key, &carrier, sizeof(carrier));
    } 
}

// Passed array by pointer, along with size of carrier. This allows the 
// read function to fill that array with the correct data requested. 
// Ensure to include the space for a null terminator if requesting a 
// char array. For size, use sizeof(carrier);
template<typename NVSRA>
bool NVSctrl::readArray(const char* key, data_t dType, NVSRA *carrier, size_t size) {

    if (size <= 0) {
        this->sendErr("NVS Read, size must be > 0");
        return false;
    }

    if (key == nullptr || *key == '\0') {
        this->sendErr("NVS Read, improper key passed to NVS"); 
        return false;
    }

    if (carrier == nullptr) {
        this->sendErr("NVS Read, carrier must be a valid array");
        return false;
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

}