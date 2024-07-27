#include "Network/NetCreds.hpp"
#include "NVS/NVS.hpp"
#include "UI/MsgLogHandler.hpp"
#include "string.h"

namespace NVS {

Creds::Creds(char nameSpace[12], Messaging::MsgLogHandler &msglogerr) :

    msglogerr(msglogerr), nvs(msglogerr, nameSpace) {

        memset(this->credData, 0, sizeof(this->credData));
    }
    
void Creds::write(const char* key, const char* buffer, size_t size) {
    nvs_ret_t stat = this->nvs.writeArray(key, data_t::CHAR, buffer, size);

    if (stat != nvs_ret_t::NVS_WRITE_OK) {
        msglogerr.handle(
                Messaging::Levels::ERROR,
                "NVS Creds were not written",
                Messaging::Method::SRL
            );
    }
}

const char* Creds::read(const char* key) {
    memset(this->credData, 0, sizeof(this->credData));
 
    auto getCred = [this](const char* key) {
        nvs_ret_t stat = this->nvs.readArray(
            key, data_t::CHAR, 
            this->credData, 
            sizeof(this->credData)); 
        
        if (stat != nvs_ret_t::NVS_READ_OK) {
            msglogerr.handle(
                Messaging::Levels::ERROR,
                "NVS Creds were not read",
                Messaging::Method::SRL
            );
        }
    };

    for (const auto &_key : Comms::keys) {
        if (strcmp(key, _key) == 0) {
            getCred(key);
            break;
        }
    }

    return this->credData;
}

}

