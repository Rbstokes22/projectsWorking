#ifndef CREDS_H
#define CREDS_H

#define SSID_MAX 32 
#define PASS_MAX 64 
#define PHONE_MAX 15
#define WAP_PASS_MAX 64

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
    CredsInfo credInfo[4];
    char ssid[SSID_MAX];
    char pass[PASS_MAX];
    char phone[PHONE_MAX];
    char WAPpass[WAP_PASS_MAX];
    const char* nameSpace;
    static const char* keys[4];
    static const uint16_t checksumConst;

    public:
    Credentials(const char* nameSpace, Messaging::MsgLogHandler &msglogerr);
    bool write(const char* key, const char* buffer);
    void read(const char* key);
    uint8_t computeChecksum();
    void setChecksum();
    bool getChecksum();
    const char* getSSID() const;
    const char* getPASS() const;
    const char* getPhone() const;
    const char* getWAPpass() const;
};
}

#endif // CREDS_H