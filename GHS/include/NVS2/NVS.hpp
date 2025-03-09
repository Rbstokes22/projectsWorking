#ifndef NVS_HPP
#define NVS_HPP

#include "nvs.h"
#include "nvs_flash.h"
#include "cstdint"
#include "UI/MsgLogHandler.hpp"

namespace NVS {

// Namespace and keys have the same mac chars per datasheet.
#define MAX_NAMESPACE 15 // MAX CHARS per datasheet
#define MAX_NVS_ENTRY 512 // Max value storage
#define NVS_MAX_INIT_ATT 5 // Attempts at initializing.
#define NVS_NULL 0 // Used for handle

enum class nvs_ret_t {
    NVS_INIT_OK, MAX_INIT_ATTEMPTS, NVS_INIT_FAIL,
    NVS_PARTITION_ERASED, NVS_PARTITION_NOT_ERASED,
    NVS_NEW_ENTRY, NVS_KEYLENGTH_ERROR,
    CHECKSUM_OK, CHECKSUM_FAIL,
    NVS_READ_OK, NVS_READ_FAIL, NVS_READ_BAD_PARAMS,
    NVS_WRITE_OK, NVS_WRITE_FAIL, NVS_WRITE_BAD_PARAMS
};

struct NVS_Config {
    nvs_handle_t handle; // NVS handle
    const char* nameSpace; // NVS namespace
    NVS_Config(const char* nameSpace);
};

class NVSctrl {
    private:
    const char* tag;
    char log[LOG_MAX_ENTRY];
    bool isInit;
    NVS_Config conf;
    nvs_ret_t writeToNVS(const char* key, uint8_t* data, size_t bytes);
    nvs_ret_t readFromNVS(const char* key, uint8_t* carrier, size_t bytes, 
        bool fromWrite = false); // used for new key handling
    
    nvs_ret_t checkCRC(const char* key, uint8_t* data, size_t bytes);
    void sendErr(const char* msg, Messaging::Levels lvl = 
        Messaging::Levels::ERROR);

    public:
    NVSctrl(const char* nameSpace); 
    nvs_ret_t init();
    nvs_ret_t eraseAll();
    nvs_ret_t write(const char* key, void* data, size_t bytes);
    nvs_ret_t read(const char* key, void* carrier, size_t bytes);
    uint32_t crc32(const uint8_t* data, size_t bytes, bool &dataSafe);
};

// Uses RAII principle
class NVS_SAFE_OPEN {
    private:
    NVS_Config &conf;
    const char* tag;
    static char log[LOG_MAX_ENTRY]; // Avoid re creation everytime.

    public:
    NVS_SAFE_OPEN(NVSctrl* ctrl, NVS_Config &conf);
    ~NVS_SAFE_OPEN();

};

}

#endif // NVS_HPP