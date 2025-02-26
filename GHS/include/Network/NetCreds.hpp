#ifndef NETCREDS_HPP
#define NETCREDS_HPP

#include "NVS2/NVS.hpp"
#include "Config/config.hpp"
#include "UI/MsgLogHandler.hpp"

namespace NVS {

struct CredParams {
    const char nameSpace[12]; // Max 12 per NVS for "CS" addition, 11 with null.
};

// SMS requirements to communicate alerts to the client that
// are tripped by the peripheral sensors. Inclues phone and API key.
struct SMSreq {
    char phone[static_cast<int>(Comms::IDXSIZE::PHONE)]; // 10-dig phone.
    char APIkey[static_cast<int>(Comms::IDXSIZE::APIKEY)]; // 8-dig API key. 
};

class Creds { // Singleton
    private:
    static const char* tag; // Tag used in logging.
    NVSctrl nvs; // nvs object
    char credData[static_cast<int>(Comms::IDXSIZE::PASS)]; // largest array size
    SMSreq smsreq; // SMS request structure.
    CredParams &params; // Required parameters.
    static char log[LOG_MAX_ENTRY];
    Creds(CredParams &params);
    Creds(const Creds&) = delete; // prevent copying
    Creds &operator=(const Creds&) = delete; // prevent assignment

    public:
    static Creds* get(CredParams* parameter = nullptr);
    nvs_ret_t write(const char* key, const char* buffer, size_t length);
    const char* read(const char* key);
    SMSreq* getSMSReq(bool bypassCheck = false);
};

}

#endif // NETCREDS_HPP