#ifndef CREDS_H
#define CREDS_H

#include <Arduino.h>
#include <Preferences.h>
#include "Network/NetConfig.h"
#include "UI/MsgLogHandler.h"

namespace NVS {

// Used for the read() method. Iterates this data to get the 
// correct network credentials for the station connection.
struct CredInfo { 
    const char* key;
    char* destination;
    size_t size;
};

class Credentials {
    private:
    Preferences prefs;
    Messaging::MsgLogHandler &msglogerr;
    CredInfo credInfo[static_cast<int>(Comms::IDXSIZE::NETCREDKEYQTY)];
    static char ssid[static_cast<int>(Comms::IDXSIZE::SSID)];
    static char pass[static_cast<int>(Comms::IDXSIZE::PASS)];
    static char phone[static_cast<int>(Comms::IDXSIZE::PHONE)];
    static char WAPpass[static_cast<int>(Comms::IDXSIZE::PASS)];
    const char* nameSpace;
    bool checkSumSafe;

    public:
    Credentials(const char* nameSpace, Messaging::MsgLogHandler &msglogerr);
    bool write(const char* key, const char* buffer);
    void read(const char* key);
    uint32_t computeChecksum(const char* buffer);
    void setChecksum(const char* key, const char* buffer);
    bool getChecksum(const char* key, const char* buffer);
    static const char* getSSID();
    static const char* getPASS();
    static const char* getPhone();
    static const char* getWAPpass();
};
}

#endif // CREDS_H