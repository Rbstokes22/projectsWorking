#ifndef NETWORKWAP_H
#define NETWORKWAP_H

#include <ArduinoJson.h>
#include "Network/NetworkMain.h"

namespace Comms {

class WirelessAP : public NetMain {
    private:

    // These variables are exclusive to the WAP, thus are not shared by all
    // subclasses to NetMain.
    char AP_SSID[static_cast<int>(IDXSIZE::SSID)];
    char AP_PASS[static_cast<int>(IDXSIZE::PASS)];
    IPAddress local_IP;
    IPAddress gateway;
    IPAddress subnet;
    NVS::CredInfo credinfo[static_cast<int>(IDXSIZE::NETCREDKEYQTY)];

    public:

    // Network/NetworkWAP.cpp
    WirelessAP(const char* AP_SSID, const char* AP_PASS,
        IPAddress local_IP, IPAddress gateway, IPAddress subnet,
        Messaging::MsgLogHandler &msglogerr);

    const char* getWAPpass();
    void setWAPpass(const char* pass);
    void WAPconnect(uint8_t maxClients);
    bool WAP(NVS::Credentials &Creds);
    bool WAPSetup(NVS::Credentials &Creds); // Setup LAN setting from WAP
    
    // Network/Routes.cpp
    void setRoutes(NVS::Credentials &Creds) override;

    // Network/Server/ServerWapSubmit.cpp
    void handleWAPsubmit(NVS::Credentials &Creds);
    void commitAndRespond(
        const char* type, NVS::Credentials &Creds,char* buffer);
    void keyNotFound();
    void getJson(
        const char* key, char* jsonData, JsonDocument &jsonDoc, NVS::Credentials &Creds);
    void handleJson(NVS::Credentials &Creds);
};

// Network/webPages.cpp
extern const char WAPSetupPage[] PROGMEM;

}

#endif // NETWORKWAP_H