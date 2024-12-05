#include "NVS2/NVS.hpp"
#include "nvs.h"
#include "nvs_flash.h"
#include "string.h"

namespace NVS {

bool NVSctrl::NVSinit = false; // inits to false.

NVS_Config::NVS_Config(const char* nameSpace) : 

    nameSpace(nameSpace), NVSopen(false) {}

// Requires the namespace. Max namespace is 15 chars, leaving room
// for the null terminator, you can have up to 14 chars.
NVSctrl::NVSctrl(const char nameSpace[MAX_NAMESPACE]) :
    conf(nameSpace) {}

// Requires TAG from calling function, and the error. Designed to handle
// print statements thoughout the program, that require no follow-on
// action and only a print.
void NVSctrl::defaultPrint(const char* TAG, esp_err_t err) {
    // this is a public method since it will be used outside
    // of the class with NVS_SAFE_OPEN.
    switch (err) {
        case ESP_OK:
        printf("%s: OK\n", TAG);
        break;

        case ESP_FAIL:
        printf("%s: Fail, potentially corrupt\n", TAG);
        break;

        case ESP_ERR_NVS_NOT_INITIALIZED:
        printf("%s: Storage driver not init\n", TAG);
        break;

        case ESP_ERR_NVS_PART_NOT_FOUND:
        printf("%s: Partition not found\n", TAG);
        break;

        case ESP_ERR_NVS_NOT_FOUND:
        printf("%s: ID namespace non-existant\n", TAG);
        break;

        case ESP_ERR_NVS_INVALID_NAME:
        printf("%s: Namespace invalid\n", TAG);
        break;

        case ESP_ERR_NVS_INVALID_HANDLE:
        printf("%s: Handle has been closed or is null\n", TAG);
        break;

        case ESP_ERR_NVS_INVALID_LENGTH:
        printf("%s: Length is not sufficient to store data\n", TAG);
        break;

        case ESP_ERR_NO_MEM:
        printf("%s: Memory allocation error\n", TAG);
        break;

        case ESP_ERR_NVS_NOT_ENOUGH_SPACE:
        printf("%s: No space for new entry\n", TAG);
        break;

        case ESP_ERR_NOT_ALLOWED:
        printf("%s: Action not allowed\n", TAG);
        break;

        case ESP_ERR_INVALID_ARG:
        printf("%s: Handle is equal to null\n", TAG);
        break;

        case ESP_ERR_NVS_NO_FREE_PAGES: 
        printf("%s: NVS contains no empty pages\n", TAG);
        break;

        default:
        printf("%s: Unknown case\n", TAG);
    }
}

// Initializes the NVS. Returns NVS_INIT_OK if successful,
// NVS_MAX_INIT_ATTEMPTS if unable to init due to nvs erase
// errors, or NVS_INIT_FAIL if failed for another reason.
nvs_ret_t NVSctrl::init() {
    // If already init, return init ok.
    if (NVSctrl::NVSinit) return nvs_ret_t::NVS_INIT_OK; 

    const char* TAG = "NVS Init";

    // In order to prevent any looping, once max attempts are reached,
    // a gate is closed.
    static uint8_t initAttempts = 0;

    if (initAttempts > NVS_MAX_INIT_ATT) {
        printf("%s: Reached max init attempts at %d\n", TAG, NVS_MAX_INIT_ATT);
        return nvs_ret_t::MAX_INIT_ATTEMPTS;
    }

    esp_err_t err = nvs_flash_init(); // first init attempt
    initAttempts++; // Incremenet the counter

    switch (err) {
        case ESP_ERR_NVS_NO_FREE_PAGES: 
        printf("%s: Erasing partition, no free pages\n", TAG);
        err = nvs_flash_erase(); // Erase NVS partition.

        if (err == ESP_OK) {
            NVSctrl::init(); // Re-attempt after valid erase.
        } else {
            printf("%s: No label for NVS in partitions\n", TAG);
        }
    
        break;

        case ESP_OK:
        NVSctrl::NVSinit = true; // change to true.
        printf("%s: Initialized\n", TAG);
        return nvs_ret_t::NVS_INIT_OK;

        default:
        NVSctrl::defaultPrint(TAG, err);
    }

    return nvs_ret_t::NVS_INIT_FAIL;
}

// Returns the configuration struct that is part of this class.
NVS_Config* NVSctrl::getConfig() {
    return &this->conf;
}

// Erases the NVS partition. If successful, returns NVS_PARTITION_ERASED, 
// if not, returns NVS_PARTITION_NOT_ERASED. Does not require initialization
// before running.
nvs_ret_t NVSctrl::eraseAll() {
    esp_err_t erase = nvs_flash_erase();
    const char* TAG = "NVS Erase";

    if (erase == ESP_OK) {
        printf("%s: Erased partition\n", TAG);
        return nvs_ret_t::NVS_PARTITION_ERASED;
    }

    printf("%s: Failed to erase partition\n", TAG);
    return nvs_ret_t::NVS_PARTITION_NOT_ERASED;
}





}