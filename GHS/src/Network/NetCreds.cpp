#include "Network/NetCreds.hpp"
#include "NVS2/NVS.hpp"
#include "Config/config.hpp"
#include "UI/MsgLogHandler.hpp"
#include "string.h"

namespace NVS {

// namespace must be under 12 chars long.
Creds::Creds(CredParams &params) :

    nvs(params.nameSpace), params(params) {

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
// well as size_t bytes. Ensure to use strlen for length, as not to mess up 
// the checksum. Returns NVS_WRITE_OK or NVS_WRITE_FAIL.
nvs_ret_t Creds::write(const char* key, const char* buffer, size_t bytes) {

    nvs_ret_t stat = this->nvs.write(key, (uint8_t*)buffer, bytes);

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
   
    auto getCred = [this](const char* key) {

        nvs_ret_t stat = this->nvs.read(
            key, this->credData, sizeof(this->credData)
            );

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

// Requires no parameters. Returns the SMS requirements pointer
// if the the phone and API key length meet the requirements.
// If not, will return nullptr, and those values will remain
// unpopulated.
SMSreq* Creds::getSMSReq(bool sendRaw) {
    // Since this function returns a non-nullptr iff data meets requirements,
    // this is called in the WAPSetupHandler so that it can populate this
    // data and bypass the checks.
    if (sendRaw) return &this->smsreq;

    // Shouldnt have to ensure proper format when reading since that
    // is controlled during the write phase.
    size_t strSize = sizeof(this->smsreq.phone); // phone size

    // Check is phone is empty. If so, read from the NVS to 
    // to the char array in the sms requirement.
    if (strlen(this->smsreq.phone) == 0) {
        strncpy(
            this->smsreq.phone, 
            this->read("phone"), 
            strSize - 1
            );

        this->smsreq.phone[strSize - 1] = '\0'; // null term.
    }

    strSize = sizeof(this->smsreq.APIkey); // API key size.

    // Check if the API key is empty. If so, read from the NVS
    // to the char array in the sms requirement.
    if (strlen(this->smsreq.APIkey) == 0) {
        strncpy(
            this->smsreq.APIkey, 
            this->read("APIkey"), 
            strSize - 1
            );

        this->smsreq.APIkey[strSize - 1] = '\0'; // null term.
    }

    // In this section we compare the the expected string length, of course
    // omitting the null terminator. If they do not meet the requirements,
    // a nullptr will be returned.
    strSize = static_cast<size_t>(Comms::IDXSIZE::PHONE) - 1; 

    if (strlen(this->smsreq.phone) != strSize) {
        // printf("Size: %zu vs strlen %d, Data: %s\n", strSize, strlen(this->smsreq.phone), this->smsreq.phone);
        printf("SMS Request: phone does not meet requirements\n");
        return nullptr;
    }
    
    strSize = static_cast<size_t>(Comms::IDXSIZE::APIKEY) - 1;

    if (strlen(this->smsreq.APIkey) != strSize) {
        printf("SMS Request: APIkey does not meet requirements\n");
        return nullptr;
    }

    return &this->smsreq;
}

}

