#ifndef OTAUPDATES_H
#define OTAUPDATES_H

// FUTURE CODE: Enable https client in order to download the firmware 
// via web. This will be controllable via web interface to get the update
// and then like a daily check for updates.

#include "Display.h"
#include "Network/Network.h"
#include "Threads.h"

// Used for system updates.
namespace UpdateSystem {

class OTAupdates {
    private:
    UI::Display &OLED; // reference to OLED from main
    Threads::SensorThread &sensorThread;
    char buffer[32];
    bool hasStarted;

    public:
    OTAupdates(
        UI::Display &OLED, 
        Threads::SensorThread &sensorThread       
        );
    void start();
    void handle();
    bool getHasStarted();
    void setHasStarted(bool value);
    void manageOTA(Comms::Station &station);
};
}

// For an OTAupdate, you need to include <ESP8266mDNS.h> and run 
// MDNS.begin("hostname") right after WiFi.begin(ssid, pass) is connected. In this 
// sketch it is done in the network page.

#endif // OTAUPDATES_H