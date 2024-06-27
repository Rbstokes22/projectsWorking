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
    static const char* keys[static_cast<int>(Comms::IDXSIZE::NETCREDKEYQTY)];
    static const uint16_t checksumConst;

    public:
    Credentials(const char* nameSpace, Messaging::MsgLogHandler &msglogerr);
    bool write(const char* key, const char* buffer);
    void read(const char* key);
    uint8_t computeChecksum();
    void setChecksum();
    bool getChecksum();
    static const char* getSSID();
    static const char* getPASS();
    static const char* getPhone();
    static const char* getWAPpass();
};
}

#endif // CREDS_H