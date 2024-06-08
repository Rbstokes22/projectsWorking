#ifndef OTAUPDATES_H
#define OTAUPDATES_H

#include "Display.h"

class OTAupdates {
    private:

    // volatile ensures that the program checks the value when using it as 
    // opposed to relaying on cached data.
    volatile bool OTAisUpdating;
    Display &OLED; // reference to OLED from main
    char buffer[32];
    bool hasStarted;

    public:
    OTAupdates(Display &OLED);
    void start();
    void handle();
    bool isUpdating() const;
    bool getHasStarted();
    void setHasStarted(bool value);
};

// For an OTAupdate, you need to include <ESP8266mDNS.h> and run 
// MDNS.begin("hostname") right after WiFi.begin(ssid, pass) is connected. In this 
// sketch it is done in the network page.

#endif // OTAUPDATES_H