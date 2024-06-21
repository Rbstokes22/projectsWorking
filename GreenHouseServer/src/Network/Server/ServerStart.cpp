#include "Network/Network.h"

namespace Comms {

// Once the routes are setup, this will begin the server.
void NetMain::startServer() {
    NetMain::server.begin(); 
    NetMain::isServerRunning = true;
    this->msglogerr.handle(Levels::INFO,"Server Started", Method::SRL);
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