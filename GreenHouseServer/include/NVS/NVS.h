#ifndef NVS_H
#define NVS_H

#include <Preferences.h>
#include "UI/MsgLogHandler.h"

namespace NVS {

enum class DType {
    INT8_T, INT16_T, INT32_T, INT64_T,
    UINT8_T, UINT16_T , UINT32_T, UINT64_T,
    FLOAT, DOUBLE, CHAR, OTHER
};

extern const uint8_t dataSize[];

class NVSctrl {
    private:
    Preferences prefs;
    Messaging::MsgLogHandler &msglogerr;
    const char* nameSpace;
    void sendErr(const char* msg, bool endPref = true);
    bool basicErrCheck(const char* key, const void* data, size_t size);
    bool writeToNVS(const char* key, const void* data, size_t size, bool isChar = false);
    bool readFromNVS(const char* key, void* carrier, size_t size, bool isChar = false);
    uint32_t computeChecksum(const uint8_t* data, size_t size);
    bool setChecksum(const char* key, const uint8_t* data, size_t size);
    bool getChecksum(const char* key, const uint8_t* data, size_t size);
    bool isCheckSumSafe;

    public:
    NVSctrl(Messaging::MsgLogHandler &msglogerr, const char* nameSpace);
    bool write(const char* key, const void* data, size_t size, bool isChar = false);

    template<typename NVSWA>
    bool writeArray(const char* key, const NVSWA* data, DType dType, size_t length);

    template<typename NVSWO>
    bool writeOther(const char* key, const NVSWO &data, DType dType);

    template<typename NVSWN>
    bool writeNum(const char* key, const NVSWN &data, DType dType);

    template<typename NVSR>
    bool read(const char* key, NVSR* carrier, size_t size, bool isChar = false);

    template<typename NVSRN>
    bool readNum(const char* key, DType dType, NVSRN &carrier); // USE #include for implementations

    template<typename NVSRO>
    bool readOther(const char* key, DType dType, NVSRO &carrier);

    template<typename NVSRA>
    bool readArray(const char* key, DType dType, NVSRA *carrier, size_t size);
};

}

#endif // NVS_H