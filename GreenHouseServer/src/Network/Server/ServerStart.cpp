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

// Once the routes are setup, this will begin the server.
void NetMain::startServer() {
    NetMain::server.begin(); 
    NetMain::isServerRunning = true;
    Serial.println("Server started");
}

void NetMain::handleNotFound() {
    NetMain::server.send(404, "text/html", "Page Not Found");
}

// Only handled when the server has begun.
void NetMain::handleServer() {
    if (NetMain::isServerRunning) { // begins after server.begin()
        NetMain::server.handleClient();
    }
}

// Used to start the OTA updates.
bool Station::isSTAconnected() {
    return this->connectedToSTA;
}
}