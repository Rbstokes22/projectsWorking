#ifndef OTAUPDATES_H
#define OTAUPDATES_H

// FUTURE CODE: Enable https client in order to download the firmware 
// via web. This will be controllable via web interface to get the update
// and then like a daily check for updates.

#include "Display.h"
#include "Network.h"
#include "Peripherals.h"

class OTAupdates {
    private:

    // volatile is used for variables that can be modified by different 
    // threads or what not. It ensures that the value is checked at use 
    // instead of relying on cached data.
    volatile bool OTAisUpdating;
    Display &OLED; // reference to OLED from main
    Threads &sensorThread;
    char buffer[32];
    bool hasStarted;

    public:
    OTAupdates(Display &OLED, Threads &sensorThread);
    void start();
    void handle();
    bool isUpdating() const;
    bool getHasStarted();
    void setHasStarted(bool value);
    void manageOTA(Net* Network);
};

// For an OTAupdate, you need to include <ESP8266mDNS.h> and run 
// MDNS.begin("hostname") right after WiFi.begin(ssid, pass) is connected. In this 
// sketch it is done in the network page.

#endif // OTAUPDATES_H