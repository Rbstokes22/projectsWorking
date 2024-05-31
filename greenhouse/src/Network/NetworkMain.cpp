#include "Network.h"
#include <ESP8266WiFi.h>

Net::Net(
    const char* AP_SSID, 
    const char* AP_Pass, 
    IPAddress local_IP,
    IPAddress gateway,
    IPAddress subnet,
    uint16_t port) : 

    AP_SSID(AP_SSID),
    AP_Pass(AP_Pass),
    local_IP(local_IP),
    gateway(gateway),
    subnet(subnet),
    server(BearSSL::ESP8266WebServerSecure(port)),
    port(port),
    prevServerType(NO_WIFI),
    MDNSrunning(false),
    isServerRunning(false)
    {
        // Set memory to all null for char arrays
        memset(this->ST_SSID, 0, sizeof(this->ST_SSID));
        memset(this->ST_PASS, 0, sizeof(this->ST_PASS));
        memset(this->phoneNum, 0, sizeof(this->phoneNum));
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

// Checks 3-way switch position and starts the appropriate network
uint8_t netSwitch() {
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