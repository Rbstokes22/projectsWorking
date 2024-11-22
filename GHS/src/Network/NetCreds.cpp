#include "Network/NetCreds.hpp"
#include "NVS/NVS.hpp"
#include "Config/config.hpp"
#include "UI/MsgLogHandler.hpp"
#include "string.h"

namespace NVS {

// namespace must be under 12 chars long.
Creds::Creds(CredParams &params) :

    nvs(params.msglogerr, params.nameSpace), params(params) {

        memset(this->credData, 0, sizeof(this->credData));
        memset(this->smsreq.phone, 0, sizeof(this->smsreq.phone));
        memset(this->smsreq.APIkey, 0, sizeof(this->smsreq.APIkey));
        // this->nvs.eraseAll(); // UNCOMMENT AS NEEDED FOR TESTING.
    }

// Requires CredParams pointer, which is default set to nullptr,
// and must be passed to initialize the object. Once complete,
// a call with no parameters will return a pointer to the object.
Creds* Creds::get(CredParams* parameter) {
    static bool isInit{false};

    if (parameter == nullptr && !isInit) {
        printf("Creds has not been init\n");
        return nullptr; // Blocks instance from being created.
    } else if (parameter != nullptr) {
        printf("Creds has been init with namespace %s\n", parameter->nameSpace);
        isInit = true; // Opens gate after proper init
    }

    static Creds instance(*parameter);
    
    return &instance;
}

// writes the char array to the NVS. Takes const char* key and buffer, as 
// well as size_t length. Ensure to use strlen for length, as not to mess up 
// the checksum. Returns NVS_WRITE_OK or NVS_WRITE_FAIL.
nvs_ret_t Creds::write(const char* key, const char* buffer, size_t length) {
    nvs_ret_t stat = this->nvs.writeArray(key, data_t::CHAR, buffer, length);

    if (stat != nvs_ret_t::NVS_WRITE_OK) {
        params.msglogerr.handle(
                Messaging::Levels::ERROR,
                "NVS Creds were not written",
                Messaging::Method::SRL
            );
    }

    return stat;
}

// Reads the NVS data char array. Takes const char* key as argument,
// and if it exists in the key array, it will return a const char* 
// that will be available to strcpy or memcpy.
const char* Creds::read(const char* key) {
    memset(this->credData, 0, sizeof(this->credData));
 
    auto getCred = [this](const char* key) {
        nvs_ret_t stat = this->nvs.readArray(
            key, data_t::CHAR, 
            this->credData, 
            sizeof(this->credData)); 
        
        if (stat != nvs_ret_t::NVS_READ_OK) {
            params.msglogerr.handle(
                Messaging::Levels::ERROR,
                "NVS Creds were not read",
                Messaging::Method::SRL
            );
        }
    };

    for (const auto &_key : Comms::netKeys) {
        if (strcmp(key, _key) == 0) {
            getCred(key);
            break;
        }
    }

    return this->credData;
}

SMSreq* Creds::getSMSReq() {
    return &this->smsreq;
}

}

