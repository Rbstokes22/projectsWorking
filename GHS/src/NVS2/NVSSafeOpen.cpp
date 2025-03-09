#include "NVS2/NVS.hpp"
#include "nvs.h"
#include "assert.h"
#include "UI/MsgLogHandler.hpp"

namespace NVS {

char NVS_SAFE_OPEN::log[LOG_MAX_ENTRY]{0}; // Static var to avoid re-creation

// Requires the pointer to the nvs control instance, and the reference to
// it configuration. This will safely opne the NVS and safely destruct it once
// it goes out of scope or the opening encounters an error. 
NVS_SAFE_OPEN::NVS_SAFE_OPEN(NVSctrl* ctrl, NVS_Config &conf) : 

    conf(conf), tag("NVSSafeOpen") {

    // Check for nullptr, block if so.
    if (ctrl == nullptr) return;

    // Initializes if not already initialized. If the nvs is 
    // unable to initialize, does not attept to open. No logging, logging is
    // handled in the NVSctrl class for initialization.
    if (ctrl->init() != nvs_ret_t::NVS_INIT_OK) return;

    if (conf.handle != NVS_NULL) return; // If open return, if not, open

    esp_err_t open = nvs_open(conf.nameSpace, NVS_READWRITE, &conf.handle);

    switch (open) {

        case ESP_OK: // Is open
        snprintf(NVS_SAFE_OPEN::log, sizeof(NVS_SAFE_OPEN::log),
            "%s namespace [%s] is open", this->tag, conf.nameSpace);

        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::INFO,
            NVS_SAFE_OPEN::log, Messaging::Method::SRL_LOG);

        return;

        default: // Is not open.
        snprintf(NVS_SAFE_OPEN::log, sizeof(NVS_SAFE_OPEN::log),
            "%s namespace [%s] is not open", this->tag, conf.nameSpace);

        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::ERROR,
            NVS_SAFE_OPEN::log, Messaging::Method::SRL_LOG);
    }
}

// Destructor will close the NVS once called or it goes out of scope.
NVS_SAFE_OPEN::~NVS_SAFE_OPEN() {

    if (conf.handle != NVS_NULL) return; // Will not close unless open.

    nvs_close(conf.handle); // closes the NVS connection
    conf.handle = NVS_NULL; // Reset handle to null in closing.

    snprintf(NVS_SAFE_OPEN::log, sizeof(NVS_SAFE_OPEN::log),
        "%s namespace [%s] is closed", this->tag, conf.nameSpace);

    Messaging::MsgLogHandler::get()->handle(Messaging::Levels::INFO,
        NVS_SAFE_OPEN::log, Messaging::Method::SRL_LOG);
}

}