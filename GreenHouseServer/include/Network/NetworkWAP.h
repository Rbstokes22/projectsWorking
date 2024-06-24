#ifndef NETWORKWAP_H
#define NETWORKWAP_H

#include "Network/NetworkMain.h"

namespace Comms {

class WirelessAP : public NetMain {
    private:

    // These variables are exclusive to the WAP, thus are not shared by all
    // subclasses to NetMain.
    char AP_SSID[15];
    char AP_PASS[64];
    IPAddress local_IP;
    IPAddress gateway;
    IPAddress subnet;

    public:

    // Network/NetworkWAP.cpp
    WirelessAP(const char* AP_SSID, const char* AP_PASS,
        IPAddress local_IP, IPAddress gateway, IPAddress subnet,
        Messaging::MsgLogHandler &msglogerr);

    const char* getWAPpass();
    void setWAPpass(const char* pass);
    void WAPconnect(uint8_t maxClients);
    bool WAP(FlashWrite::Credentials &Creds);
    bool WAPSetup(FlashWrite::Credentials &Creds); // Setup LAN setting from WAP
    
    // Network/Routes.cpp
    void setRoutes(FlashWrite::Credentials &Creds) override;

    // Network/Server/ServerWapSubmit.cpp
    void handleWAPsubmit(FlashWrite::Credentials &Creds);
        
    void commitAndRespond(
        const char* type, FlashWrite::Credentials &Creds,char* buffer);

    void handleJson(FlashWrite::Credentials &Creds);
};

// Network/webPages.cpp
extern const char WAPSetupPage[] PROGMEM;

}

#endif // NETWORKWAP_H