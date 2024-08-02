#include "NVS/NVS.hpp"
#include "nvs_flash.h"
#include "nvs.h"
#include "UI/MsgLogHandler.hpp"

// PUT CHECKSUM IN ITS OWN CLASS TO AVOID DUPLICATION MAYBE

namespace NVS {

// STATIC SETUP
const size_t NVSctrl::maxEntry{512};

// CORRESPONDS DIRECTLY WITH ENUM CLASS DATATYPE POSITION
const uint8_t dataSize[]{ 
    sizeof(int8_t), sizeof(int16_t), sizeof(int32_t), sizeof(int64_t),
    sizeof(uint8_t), sizeof(uint16_t), sizeof(uint32_t), sizeof(uint64_t),
    sizeof(float), sizeof(double), sizeof(char)
};

// Main handler of all error display throughout the system. 
// Due to several potential errors, the majority will be to serial.
// Can change to include OLED if desired by passing true as arg 2.
void NVSctrl::sendErr(const char* msg, errDisp type) {
    switch (type) {
        case errDisp::OLED:
        this->msglogerr.handle(
        Messaging::Levels::ERROR,
        msg,
        Messaging::Method::OLED
        ); break;

        case errDisp::SRL:
        this->msglogerr.handle(
        Messaging::Levels::ERROR,
        msg,
        Messaging::Method::SRL
        ); break;

        case errDisp::ALL:
        this->msglogerr.handle(
        Messaging::Levels::ERROR,
        msg,
        Messaging::Method::SRL,
        Messaging::Method::OLED
        );
    }
}

// Used in the checksum methods. Since their arguments are identical,
// it is modular to run the same check on variables. Return types are 
// NVS_OK and NVS_FAIL.
nvs_ret_t NVSctrl::basicErrCheck(const char* key, const void* data, size_t size) {
    nvs_ret_t err{nvs_ret_t::NVS_OK};

    if (data == nullptr || key == nullptr) {
        this->sendErr("NVS key or data set to nullptr", errDisp::SRL);
        err = nvs_ret_t::NVS_FAIL;
    }

    if (*key == '\0') {
        this->sendErr("NVS key does not exist", errDisp::SRL);
        err = nvs_ret_t::NVS_FAIL;
    }

    if (size <= 0) {
        this->sendErr("NVS size must be greater than 0", errDisp::SRL);
        err = nvs_ret_t::NVS_FAIL;
    }

    return err; 
}

// This is developed to handle esp errors. This primarily returns
// either NVS_OK or NVS_FAIL correlating to the ESP_OK or not OK.
// NVS_NEW_ENTRY is a return for the key not yet existing. This 
// will prevent any blocking of the code since it is exclusively
// checked for.
nvs_ret_t NVSctrl::errHandlingNVS() {
     if (this->err == ESP_OK) {
        return nvs_ret_t::NVS_OK;
    } else if (this->err == ESP_ERR_NVS_NOT_FOUND) {
        this->sendErr("NVS Key has not been created", errDisp::SRL); 
        return nvs_ret_t::NVS_NEW_ENTRY;
    } else {
        this->sendErr(esp_err_to_name(this->err), errDisp::SRL);
        return nvs_ret_t::NVS_FAIL;
    }
}

// Checks to see if the NVS has been successfully opened. If not,
// it will attempt to open. It will return either NVS_OPEN or 
// NVS_CLOSED.
nvs_ret_t NVSctrl::safeStart() { // returns open or closed.
    if (this->NVSopen != nvs_ret_t::NVS_OPEN) {

        this->err = nvs_open(this->nameSpace, NVS_READWRITE, &this->handle);
        if (this->errHandlingNVS() == nvs_ret_t::NVS_OK) {
            this->NVSopen = nvs_ret_t::NVS_OPEN;
        } else {
            this->NVSopen = nvs_ret_t::NVS_CLOSED;
        }
        return this->NVSopen; // either NVS_OK or NVS_FAIL
    } else {
        return nvs_ret_t::NVS_OPEN;
    }
}

// Checks to see if the NVS is opened. If true, it will be closed.
// This will return NVS_CLOSED.
nvs_ret_t NVSctrl::safeClose() {
    if (this->NVSopen == nvs_ret_t::NVS_OPEN) {
        nvs_close(this->handle);
        this->NVSopen = nvs_ret_t::NVS_CLOSED;
        return nvs_ret_t::NVS_CLOSED;
    } else {
        return nvs_ret_t::NVS_CLOSED;
    }
}

NVSctrl::NVSctrl(Messaging::MsgLogHandler &msglogerr, const char nameSpace[12]) :

msglogerr(msglogerr), nameSpace(nameSpace), isCheckSumSafe{true},
NVSopen(nvs_ret_t::NVS_CLOSED) {}

// Erases all NVS data.
nvs_ret_t NVSctrl::eraseAll() {
    this->err = nvs_flash_erase();
    return this->errHandlingNVS();
}

}