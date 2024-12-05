#ifndef NVS_HPP
#define NVS_HPP

#include "nvs.h"
#include "nvs_flash.h"

namespace NVS {

// Namespace and keys have the same mac chars per datasheet.
#define MAX_NAMESPACE 15 // Max chars per datasheet
#define MAX_NVS_ENTRY 512 // Max value storage
#define NVS_MAX_INIT_ATT 5 // Attempts at initializing.

enum class nvs_ret_t {
    NVS_INIT_OK, MAX_INIT_ATTEMPTS, NVS_INIT_FAIL,
    NVS_PARTITION_ERASED, NVS_PARTITION_NOT_ERASED,
    NVS_NEW_ENTRY, NVS_KEY_TOO_LONG,


    NVS_OK, NVS_FAIL, CHECKSUM_OK, CHECKSUM_FAIL,
    NVS_OPEN, NVS_CLOSED, 
    NVS_READ_OK, NVS_READ_FAIL, NVS_READ_BAD_PARAMS,
    NVS_WRITE_OK, NVS_WRITE_FAIL, NVS_WRITE_BAD_PARAMS
};

struct NVS_Config {
    nvs_handle_t handle;
    const char* nameSpace;
    bool NVSopen;
    NVS_Config(const char* nameSpace);
};

class NVSctrl {
    private:
    static bool NVSinit;
    NVS_Config conf;
    nvs_ret_t writeToNVS(const char* key, uint8_t* data, size_t bytes);
    nvs_ret_t readFromNVS(
        const char* key, 
        uint8_t* carrier, 
        size_t bytes, 
        bool fromWrite = false // used for new key handling
        );
    nvs_ret_t checkCRC(const char* key, uint8_t* data, size_t bytes);

    public:
    NVSctrl(const char nameSpace[MAX_NAMESPACE]); 
    static void defaultPrint(const char* TAG, esp_err_t err);
    static nvs_ret_t init();
    NVS_Config* getConfig();
    nvs_ret_t eraseAll();
    nvs_ret_t write(const char* key, void* data, size_t bytes);
    nvs_ret_t read(const char* key, void* carrier, size_t bytes);
    uint32_t crc32(const uint8_t* data, size_t bytes, bool &dataSafe);



};

class NVS_SAFE_OPEN {
    private:
    NVSctrl* nvs;
    const char* TAG;

    public:
    NVS_SAFE_OPEN(NVSctrl* nvs);
    ~NVS_SAFE_OPEN();

};

}

#endif // NVS_HPP