#ifndef NETCREDS_HPP
#define NETCREDS_HPP

#include "NVS2/NVS.hpp"
#include "Config/config.hpp"

namespace NVS {

struct CredParams {
    const char nameSpace[12];
};

// SMS requirements to communicate alerts to the client that
// are tripped by the peripheral sensors.
struct SMSreq {
    char phone[static_cast<int>(Comms::IDXSIZE::PHONE)];
    char APIkey[static_cast<int>(Comms::IDXSIZE::APIKEY)];
};

class Creds {
    private:
    NVSctrl nvs;
    char credData[static_cast<int>(Comms::IDXSIZE::PASS)]; // largest array size
    SMSreq smsreq;
    CredParams &params; 
    Creds(CredParams &params);
    Creds(const Creds&) = delete; // prevent copying
    Creds &operator=(const Creds&) = delete; // prevent assignment

    public:
    static Creds* get(CredParams* parameter = nullptr);
    nvs_ret_t write(const char* key, const char* buffer, size_t length);
    const char* read(const char* key);
    SMSreq* getSMSReq(bool sendRaw = false);
};

}

#endif // NETCREDS_HPP