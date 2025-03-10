#include "Network/NetCreds.hpp"
#include "NVS2/NVS.hpp"
#include "Config/config.hpp"
#include "UI/MsgLogHandler.hpp"
#include "string.h"

namespace NVS {

// tag declared static because it is used in that static get function.
const char* Creds::tag = "(Creds)";
char Creds::log[LOG_MAX_ENTRY] = {0}; // Static def of log.

// namespace must be under 12 chars long. Manages credentials and works as 
// an interface between the program and the NVS.
Creds::Creds(CredParams &params) :

    nvs(params.nameSpace), params(params) {

        memset(this->credData, 0, sizeof(this->credData));
        memset(this->smsreq.phone, 0, sizeof(this->smsreq.phone));
        memset(this->smsreq.APIkey, 0, sizeof(this->smsreq.APIkey));
        // this->nvs.eraseAll(); // UNCOMMENT AS NEEDED FOR TESTING.

        snprintf(Creds::log, sizeof(Creds::log), "%s Ob created", Creds::tag);
        Creds::sendErr(Creds::log, Messaging::Levels::INFO, true);
    }

// Requires message, message level, and if repeating log analysis should be 
// ignored. Messaging default to ERROR, ignoreRepeat default to false.
void Creds::sendErr(const char* msg, Messaging::Levels lvl, bool ignoreRepeat) {
    Messaging::MsgLogHandler::get()->handle(lvl, msg,
        Messaging::Method::SRL_LOG, ignoreRepeat);
}

// Requires CredParams pointer, which is default set to nullptr, and must be 
// passed to initialize the object. Once complete, a call with no parameters 
// will return a pointer to the created object.
Creds* Creds::get(CredParams* parameter) {
    static bool isInit{false};

    // Empty calls will pass nullptr by default, which will return a nullptr
    // if it hasnt been init. Once init, this will be bypassed.
    if (parameter == nullptr && !isInit) {

        snprintf(Creds::log, sizeof(Creds::log), "%s Creds has not been init",
            Creds::tag);

        // nullptr will cause a crash if used.
        Creds::sendErr(Creds::log, Messaging::Levels::CRITICAL);
        return nullptr; // Blocks instance from being created.

    } else if (parameter != nullptr && !isInit) {
        // This will occur upon init with the proper parameter. This
        // can only be init once.
        
        snprintf(Creds::log, sizeof(Creds::log), 
            "%s Creds init with namespace %s", Creds::tag, 
            parameter->nameSpace);

        Creds::sendErr(Creds::log, Messaging::Levels::INFO);
        isInit = true; // Opens gate after proper init
    }

    static Creds instance(*parameter); // Create instance and return.
    
    return &instance;
}

// Requires key, pointer to buffer, and size of buffer. Ensure to use strlen
// + 1 for bytes, leaving room for the null terminator and to not mess up the
// checksum value. Returns NVS_WRITE_OK or NVS_WRITE_FAIL.
nvs_ret_t Creds::write(const char* key, const char* buffer, size_t bytes) {

    nvs_ret_t stat = this->nvs.write(key, (uint8_t*)buffer, bytes);

    if (stat != nvs_ret_t::NVS_WRITE_OK) {
        snprintf(Creds::log, sizeof(Creds::log), "%s writeErr", Creds::tag);
        Creds::sendErr(Creds::log);
    }

    return stat;
}

// Requires the key. If key exists in the key array in the configuration header
// it will return a const char* that will be available to copy.
const char* Creds::read(const char* key) {
   
    // lambda to in key iteration. Reads the entry matching the key and 
    // copies it over to credData array. If error, creddata will = 0.
    auto getCred = [this](const char* key) {

        nvs_ret_t stat = this->nvs.read(
            key, this->credData, sizeof(this->credData)
            );

        if (stat != nvs_ret_t::NVS_READ_OK) {
            snprintf(Creds::log, sizeof(Creds::log), "%s readErr", Creds::tag);
            Creds::sendErr(Creds::log);
            }
    };

    // Iterates each key in the netKeys comparing the passed key variable to
    // the allowed keys in the array. If matching, the credential is copied
    // and returned.
    for (const auto &_key : Comms::netKeys) {
        if (strcmp(key, _key) == 0) {
            getCred(key);
            break;
        }
    }

    return this->credData; // 0 if bad.
}

// Requires boolean to bypass checks. which is called by the WAP Setup Handler 
// to bypass validation to directly manipulate the object and avoid checks 
// which will return a nullptr if formatting is incorrect. Data will not be 
// written to the NVS unless formatting is correct, so all calls from outside 
// of the WAP Setup Handler, can pass false, and the formatting checks will 
// occur returning the object if successful, or a nullptr if error. Handle 
// nullptr on the requesting code.
SMSreq* Creds::getSMSReq(bool bypassCheck) {

    // Bypassed when populating the struct, called by WAP setup handler.
    if (bypassCheck) return &this->smsreq;

    // Shouldnt have to ensure proper format when reading since that
    // is controlled during the write phase.

    // First get the string size of the phone, should be 11. This is used to
    // ensure null termination.
    size_t strSize = sizeof(this->smsreq.phone); // phone size 10 + null

    // Then check is phone is empty. If so, attempt read from the NVS to 
    // to the char array in the sms requirement. If error, will populate
    // phone to 0.
    if (strlen(this->smsreq.phone) == 0) {
        strncpy(this->smsreq.phone, this->read("phone"), strSize - 1);
        this->smsreq.phone[strSize - 1] = '\0'; // null term.
    }

    // Next get the string size of the API key, should be 9. This is used to
    // ensure null termination.
    strSize = sizeof(this->smsreq.APIkey); // API key size.

    // Now check if the API key is empty. If so, attempt read from the NVS
    // to the char array in the sms requirement. If error, will populate
    // APIkey to 0.
    if (strlen(this->smsreq.APIkey) == 0) {
        strncpy(this->smsreq.APIkey, this->read("APIkey"), strSize - 1);
        this->smsreq.APIkey[strSize - 1] = '\0'; // null term.
    }

    // In this section we compare the the expected string length, of course
    // omitting the null terminator. If they do not meet the requirements,
    // a nullptr will be returned preventing the use of the data.
    strSize = static_cast<size_t>(Comms::IDXSIZE::PHONE) - 1; // 10.

    // Check the actual strlen and compare it against the requirement, If err,
    // return nullptr and log entry ERROR. Will not disable system.
    if (strlen(this->smsreq.phone) != strSize) {

        snprintf(Creds::log, sizeof(Creds::log), 
            "%s SMS phone size does not meet requirements", Creds::tag);

        Creds::sendErr(Creds::log);

        return nullptr;
    }
    
    // Check again for the API key strlen.
    strSize = static_cast<size_t>(Comms::IDXSIZE::APIKEY) - 1; // should be 8.

    // Compare that to the expected requirement. If err, return nullptr and
    // log entry ERROR. Will not diable system.
    if (strlen(this->smsreq.APIkey) != strSize) {
        snprintf(Creds::log, sizeof(Creds::log), 
            "%s SMS APIkey size does not meet requirements", Creds::tag);

        Creds::sendErr(Creds::log);

        return nullptr;
    }

    return &this->smsreq; // If successful, return good data.
}

}

