#include "Network.h"

namespace Comms {

// MAIN SERVER ROUTES THAT APPLY TO ALL
void NetMain::setRoutes(UI::IDisplay &OLED, FlashWrite::Credentials &Creds) {
    if (NetMain::mainServerSetup == false) {
        NetMain::server.on("/", [this](){this->handleIndex();});
        NetMain::server.onNotFound([this](){this->handleNotFound();});
        NetMain::mainServerSetup = true;
    }
}

// SERVER ROUTES EXCLUSIVE TO WIRELESS
void WirelessAP::setRoutes(UI::IDisplay &OLED, FlashWrite::Credentials &Creds) {
    NetMain::server.on("/WAPsubmit", [this, &OLED, &Creds](){
        WirelessAP::handleWAPsubmit(OLED, Creds);
        });
    NetMain::setRoutes(OLED, Creds);
}

// SERVER ROUTES EXCLUSIVE TO WIRELESS (NONE AT THE MOMENT)
void Station::setRoutes(UI::IDisplay &OLED, FlashWrite::Credentials &Creds) {
    NetMain::setRoutes(OLED, Creds);
}

}