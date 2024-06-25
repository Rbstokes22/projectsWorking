#ifndef NETWORKSTA_H
#define NETWORKSTA_H

#include "Network/NetworkMain.h"
#include "Network/Creds.h"

enum class Netvals {
    netSTAcredNum = 3
};

namespace Comms {

struct netCredsInfo { 
    const char* key;
    const char* (*source)();
    char* destination;
    uint8_t size;
};

class Station : public NetMain {
    private:
    bool connectedToSTA; // used for OTA updates.
    netCredsInfo credsinfo[static_cast<int>(Netvals::netSTAcredNum)];

    public:

    // Network/NetworkSTA.cpp
    Station(Messaging::MsgLogHandler &msglogerr);
    STAdetails getSTADetails(); 
    void setCredsNVS(FlashWrite::Credentials &Creds);
    void STAconnect(FlashWrite::Credentials &Creds);
    void attemptReconnect(FlashWrite::Credentials &Creds, uint8_t freqSec = 10);

    // Network/Routes.cpp
    void setRoutes(FlashWrite::Credentials &Creds) override;

    // Network/Server/Server/ServerStart.cpp
    WIFI STA(FlashWrite::Credentials &Creds);
    bool isSTAconnected(); // used to start OTA updates
};

}

#endif // NETWORKSTA_H