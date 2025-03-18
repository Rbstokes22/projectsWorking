#include "NVS2/NVS.hpp"
#include "nvs.h"
#include "nvs_flash.h"
#include "string.h"
#include "UI/MsgLogHandler.hpp"

namespace NVS {

NVS_Config::NVS_Config(const char* nameSpace) : 

    handle(NVS_NULL), nameSpace(nameSpace) {}

// Requires message, message level, and if repeating log analysis should be 
// ignored. Messaging default to ERROR, ignoreRepeat default to false.
void NVSctrl::sendErr(const char* msg, Messaging::Levels lvl,
    bool ignoreRepeat) {

    Messaging::MsgLogHandler::get()->handle(lvl, msg, NVS_LOG_METHOD, 
        ignoreRepeat);
}

// Requires the namespace. Max namespace is 15 chars, leaving room
// for the null terminator, you can have up to 14 actual chars.
NVSctrl::NVSctrl(const char* nameSpace) : tag("(NVSctrl)"), conf(nameSpace) {

    snprintf(this->log, sizeof(this->log), "%s Ob Created", this->tag);
    this->sendErr(this->log, Messaging::Levels::INFO, true);
}

// Requires no params. Inititializes the NVS if it hasnt been initialized.
// Returns NVS_INIT_OK if initialized, NVS_MAX_INIT_ATTEMPTS if unable to
// initialize after set amount of attempts, or NVS_INIT_FAIL if unable to init.
nvs_ret_t NVSctrl::init() {
    // If already init, return init ok.
    if (this->isInit) return nvs_ret_t::NVS_INIT_OK; 

    // In order to prevent any looping, once max attempts are reached,
    // a gate is closed.
    static uint8_t initAttempts = 0;

    // If max attempts are equal, logs with WARNING, init max attempts 
    // reached. Then incrememnets to set up future block to avoid overlogging.
    if (initAttempts == NVS_MAX_INIT_ATT) {

        snprintf(this->log, sizeof(this->log), "%s Reached max init att at %d", 
            this->tag, initAttempts);

        this->sendErr(this->log, Messaging::Levels::WARNING);
        initAttempts++; // Allows the return block to exec in the elseif block.

        return nvs_ret_t::MAX_INIT_ATTEMPTS;

    } else if (initAttempts > NVS_MAX_INIT_ATT) return nvs_ret_t::NVS_INIT_FAIL;

    esp_err_t err = nvs_flash_init(); // init attempt after gate.
    initAttempts++; // Increment the counter

    switch (err) {

        case ESP_ERR_NVS_NO_FREE_PAGES: 

        snprintf(this->log, sizeof(this->log), 
            "%s erasing partition, no free pages", this->tag);

        this->sendErr(this->log, Messaging::Levels::INFO);

        // Erase the NVS partition to try to re-init after.
        if (this->eraseAll() == nvs_ret_t::NVS_PARTITION_ERASED) {

            // Despite log in eraseAll(), log re-initializing as well.
            snprintf(this->log, sizeof(this->log), "%s re-initializing", 
                this->tag);

            this->sendErr(this->log, Messaging::Levels::INFO);
            this->init(); // Re-attempt after valid erase.
        } 

        break;

        case ESP_OK: // Initialized.
        this->isInit = true; // Change init to true.
        snprintf(this->log, sizeof(this->log), "%s Init", this->tag);
        this->sendErr(this->log, Messaging::Levels::INFO);

        return nvs_ret_t::NVS_INIT_OK;

        default: // If anything else
        snprintf(this->log, sizeof(this->log), "%s not init", this->tag);
        this->sendErr(this->log);
        break;
    }

    return nvs_ret_t::NVS_INIT_FAIL; // Always fail, unless init = true.
}

// Requires no params. Erases the NVS partition. If successful, returns
// NVS_PARTITION_ERASE, if not, returns NVS_PARTITION_NOT_ERASED. Does not 
// require initialization before running.
nvs_ret_t NVSctrl::eraseAll() {
    esp_err_t erase = nvs_flash_erase();

    if (erase == ESP_OK) {
        snprintf(this->log, sizeof(this->log), "%s partition erased", 
            this->tag);
        
        this->sendErr(this->log, Messaging::Levels::INFO);
        return nvs_ret_t::NVS_PARTITION_ERASED;
    }

    snprintf(this->log, sizeof(this->log), "%s partition not erased", 
        this->tag);
    
    this->sendErr(this->log);
    return nvs_ret_t::NVS_PARTITION_NOT_ERASED;
}





}