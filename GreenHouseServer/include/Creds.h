#ifndef CREDS_H
#define CREDS_H

#define SSID_MAX 32 
#define PASS_MAX 64 
#define PHONE_MAX 15
#define WAP_PASS_MAX 64

#include <Arduino.h>
#include <Preferences.h>
#include "IDisplay.h"

class Credentials {
    private:
    Preferences prefs;
    IDisplay &OLED;
    char ssid[SSID_MAX];
    char pass[PASS_MAX];
    char phone[PHONE_MAX];
    char WAPpass[WAP_PASS_MAX];
    const char* nameSpace;

    public:
    Credentials(const char* nameSpace, IDisplay &OLED);
    bool write(const char* type, const char* buffer);
    void read(const char* type);
    void setChecksum();
    bool getChecksum();
    const char* getSSID() const;
    const char* getPASS() const;
    const char* getPhone() const;
    const char* getWAPpass() const;
};

#endif // CREDS_H