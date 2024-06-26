#ifndef CREDS_H
#define CREDS_H

#include <Arduino.h>
#include <Preferences.h>
#include "UI/MsgLogHandler.h"

enum class CredsSize { // max index values
    SSID_MAX = 32,
    PASS_MAX = 64,
    PHONE_MAX = 15,
    WAP_PASS_MAX = 64,
    networkCredKeyQty = 4 // uses for ssid, pass, phone, and WAPpass
};

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
    CredInfo credInfo[static_cast<int>(CredsSize::networkCredKeyQty)];
    static char ssid[static_cast<int>(CredsSize::SSID_MAX)];
    static char pass[static_cast<int>(CredsSize::PASS_MAX)];
    static char phone[static_cast<int>(CredsSize::PHONE_MAX)];
    static char WAPpass[static_cast<int>(CredsSize::WAP_PASS_MAX)];
    const char* nameSpace;
    static const char* keys[static_cast<int>(CredsSize::networkCredKeyQty)];
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