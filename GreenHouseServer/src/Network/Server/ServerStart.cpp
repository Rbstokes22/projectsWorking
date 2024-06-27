#include "Network/NetworkSTA.h"

namespace Comms {

// Once the routes are setup, this will begin the server.
void NetMain::startServer() {
    NetMain::server.begin(); 
    NetMain::isServerRunning = true;
    this->msglogerr.handle(
        Messaging::Levels::INFO,
        "Server Started", 
        Messaging::Method::SRL);
}

void NetMain::handleNotFound() {
    NetMain::server.send(404, "text/html", "Page Not Found");
}

// Only handled once the server has begun.
void NetMain::handleServer() {
    if (NetMain::isServerRunning) { // begins after server.begin()
        NetMain::server.handleClient();
    }
}

// Used to confirm the connection to STA for the OTA updates.
bool Station::isSTAconnected() {
    return this->connectedToSTA;
}

}