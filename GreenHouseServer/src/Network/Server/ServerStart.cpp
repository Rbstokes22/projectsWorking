#include "Network.h"

void Net::startServer(IDisplay &OLED, Credentials &Creds) {
    server.on("/", [this](){handleIndex();});
    server.on("/WAPsubmit", [this, &OLED, &Creds](){handleWAPsubmit(OLED, Creds);});
    server.onNotFound([this](){handleNotFound();});
    server.begin(); 
    this->isServerRunning = true;
}

void Net::handleNotFound() {
    server.send(404, "text/html", "Page Not Found");
}

void Net::handleServer() {
    if (this->isServerRunning) { // begins after server.begin()
        this->server.handleClient();
    }
}

// Used to start the OTA updates
bool Net::isSTAconnected() {
    return this->connectedToSTA;
}