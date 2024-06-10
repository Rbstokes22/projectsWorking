#include "Network.h"
#include <WiFi.h>

namespace Comms {

Net::Net(
    const char* AP_SSID, 
    const char* AP_Pass, 
    IPAddress local_IP,
    IPAddress gateway,
    IPAddress subnet,
    uint16_t port) : 

    local_IP(local_IP),
    gateway(gateway),
    subnet(subnet),
    server(port),
    port(port),
    prevServerType(NO_WIFI),
    MDNSrunning(false),
    isServerRunning(false),
    connectedToSTA(false)
    {
        // Set memory to all null for char arrays and copy the starting 
        // network ssid and pass. This helps for when the eeprom goes out 
        // of scope and copies the value instead of the pointer garbage.
        strncpy(this->AP_SSID, AP_SSID, sizeof(this->AP_SSID) - 1);
        this->AP_SSID[sizeof(this->AP_SSID) - 1] = '\0';

        strncpy(this->AP_Pass, AP_Pass, sizeof(this->AP_Pass) - 1);
        this->AP_Pass[sizeof(this->AP_Pass) - 1] = '\0';
        
        memset(this->ST_SSID, 0, sizeof(this->ST_SSID));
        memset(this->ST_PASS, 0, sizeof(this->ST_PASS));
        memset(this->phone, 0, sizeof(this->phone));
        memset(this->error, 0, sizeof(this->error));
    } 

STAdetails Net::getSTADetails() {
    STAdetails details;
    strcpy(details.SSID, this->ST_SSID);
    IPAddress ip = WiFi.localIP();
    sprintf(details.IPADDR, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    sprintf(details.signalStrength, "%d dBm", WiFi.RSSI());
    return details;
}   

const char* Net::getWAPpass() {
    return this->AP_Pass;
}

void Net::setWAPpass(const char* pass) {
    strncpy(this->AP_Pass, pass, sizeof(this->AP_Pass) - 1);
    this->AP_Pass[sizeof(this->AP_Pass) - 1] = '\0';
}

uint8_t wifiModeSwitch() {
    // 0 = WAP exclusive, 1 = WAP Program, 2 = Station
    uint8_t WAP = digitalRead(WAPswitch);
    uint8_t STA = digitalRead(STAswitch);  

    if (!WAP && STA) { // WAP Exclusive
        return WAP_ONLY;
    } else if (WAP && STA) { // Middle position, WAP Program mode for STA
        return WAP_SETUP;
    } else if (!STA && WAP) { // Station mode reading from EEPROM
        return STA_ONLY;
    } else {
        return NO_WIFI; // standard error throughout this program
    }
}
}