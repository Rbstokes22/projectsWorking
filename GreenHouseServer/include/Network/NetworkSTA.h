#ifndef NETWORKSTA_H
#define NETWORKSTA_H

#include "Network/NetworkMain.h"

namespace Comms {

// struct netCredsInfo { // FUNCTION POINTER???????
//     const char* key;
//     const char* 
// };

class Station : public NetMain {
    private:
    bool connectedToSTA; // used for OTA updates.

    public:

    // Network/NetworkSTA.cpp
    Station(Messaging::MsgLogHandler &msglogerr);
    STAdetails getSTADetails(); 
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