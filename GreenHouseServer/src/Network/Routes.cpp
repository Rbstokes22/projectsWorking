#include "Network/NetworkMain.h"
#include "Network/NetworkSTA.h"
#include "Network/NetworkWAP.h"

namespace Comms {

// MAIN SERVER ROUTES THAT APPLY TO ALL. The NetMain routes will be 
// called by each subclass, it will only be initialized once though
// using isMainServerSetup flag.
void NetMain::setRoutes(FlashWrite::Credentials &Creds) {
    if (NetMain::isMainServerSetup == false) {
        NetMain::server.on("/", [this](){this->handleIndex();});
        NetMain::server.onNotFound([this](){this->handleNotFound();});
        NetMain::isMainServerSetup = true;
    }
}

// SERVER ROUTES EXCLUSIVE TO WIRELESS
void WirelessAP::setRoutes(FlashWrite::Credentials &Creds) {
    NetMain::server.on("/WAPsubmit", [this, &Creds](){
        WirelessAP::handleWAPsubmit(Creds);
        });
    NetMain::setRoutes(Creds);
}

// SERVER ROUTES EXCLUSIVE TO STATION (NONE AT THE MOMENT)
void Station::setRoutes(FlashWrite::Credentials &Creds) {
    NetMain::setRoutes(Creds);
}

}