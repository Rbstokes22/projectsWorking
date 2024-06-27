#ifndef NETWORKSTA_H
#define NETWORKSTA_H

#include "Network/NetworkMain.h"
#include "Network/Creds.h"

namespace Comms {

// Used in iterable array to receive NVS storage during the connection
// to the station.
struct CredInfo { 
    const char* key;
    const char* (*source)();
    char* destination;
    uint8_t size;
};

class Station : public NetMain {
    private:
    bool connectedToSTA; // used for OTA updates.
    CredInfo credinfo[static_cast<int>(IDXSIZE::KEYQTYSTA)]; // THIS COULD BE AN ISSUE, FIX ENUM

    public:

    // Network/NetworkSTA.cpp
    Station(Messaging::MsgLogHandler &msglogerr);
    STAdetails getSTADetails(); 
    void setCredsNVS(NVS::Credentials &Creds);
    void STAconnect(NVS::Credentials &Creds);
    void attemptReconnect(NVS::Credentials &Creds, uint8_t freqSec = 10);

    // Network/Routes.cpp
    void setRoutes(NVS::Credentials &Creds) override;

    // Network/Server/Server/ServerStart.cpp
    WIFI STA(NVS::Credentials &Creds);
    bool isSTAconnected(); // used to start OTA updates
};

}

#endif // NETWORKSTA_H