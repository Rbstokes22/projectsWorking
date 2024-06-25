#ifndef CREDS_H
#define CREDS_H

enum class CredsNet {
    SSID_MAX = 32,
    PASS_MAX = 64,
    PHONE_MAX = 15,
    WAP_PASS_MAX = 64,
    networkCredKeyNum = 4
};

// #define SSID_MAX 32 // CHANGE TO ENUM
// #define PASS_MAX 64 
// #define PHONE_MAX 15
// #define WAP_PASS_MAX 64

#include <Arduino.h>
#include <Preferences.h>
#include "UI/MsgLogHandler.h"

namespace FlashWrite {

struct CredsInfo { // used for the read method
    const char* key;
    char* data;
    size_t size;
};

class Credentials {
    private:
    Preferences prefs;
    Messaging::MsgLogHandler &msglogerr;
    CredsInfo credInfo[static_cast<int>(CredsNet::networkCredKeyNum)];
    static char ssid[static_cast<int>(CredsNet::SSID_MAX)];
    static char pass[static_cast<int>(CredsNet::PASS_MAX)];
    static char phone[static_cast<int>(CredsNet::PHONE_MAX)];
    static char WAPpass[static_cast<int>(CredsNet::WAP_PASS_MAX)];
    const char* nameSpace;
    static const char* keys[static_cast<int>(CredsNet::networkCredKeyNum)];
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