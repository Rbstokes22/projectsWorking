#include "Network.h"
#include <WiFi.h>

// This source file provides the general defintions of the NetMain superclass
// and its sub-classes. 

namespace Comms {

// STATIC VARIABLE DEFINITIONS
WebServer NetMain::server(80);
uint8_t NetMain::prevServerType = NO_WIFI;
bool NetMain::isServerRunning = false;
bool NetMain::MDNSrunning = false;
char NetMain::ST_SSID[32];
char NetMain::ST_PASS[64];
char NetMain::phone[15];
bool NetMain::mainServerSetup = false;

// NetMain 
NetMain::NetMain() {
    // Initialize the arrays with null chars
    memset(this->error, 0, sizeof(this->error));
    memset(NetMain::ST_SSID, 0, sizeof(NetMain::ST_SSID));
    memset(NetMain::ST_PASS, 0, sizeof(NetMain::ST_PASS));
    memset(NetMain::phone, 0, sizeof(NetMain::phone));
}

NetMain::~NetMain(){}

// WAP
WirelessAP::WirelessAP(
    const char* AP_SSID, const char* AP_PASS,
    IPAddress local_IP, IPAddress gateway,
    IPAddress subnet) :
    local_IP(local_IP),
    gateway(gateway),
    subnet(subnet)
    {
    // Copy the SSID and PASS arguments of this function to the class
    // variables.
    strncpy(this->AP_SSID, AP_SSID, sizeof(this->AP_SSID) - 1);
    this->AP_SSID[sizeof(this->AP_SSID) - 1] = '\0';

    strncpy(this->AP_PASS, AP_PASS, sizeof(this->AP_PASS) - 1);
    this->AP_PASS[sizeof(this->AP_PASS) - 1] = '\0';
}

const char* WirelessAP::getWAPpass() {
    return this->AP_PASS;
}

void WirelessAP::setWAPpass(const char* pass) {
    strncpy(this->AP_PASS, pass, sizeof(this->AP_PASS) - 1);
    this->AP_PASS[sizeof(this->AP_PASS) - 1] = '\0';
}

// Station
Station::Station() : connectedToSTA(false) {}

// Provides the OLED with all of the station details, and seems redundant.
// It is not redundant because it will allow the client to see if there is 
// an error or something was changed or corrupted.
STAdetails Station::getSTADetails() {
    STAdetails details;
    strcpy(details.SSID, NetMain::ST_SSID);
    IPAddress ip = WiFi.localIP();
    sprintf(details.IPADDR, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    sprintf(details.signalStrength, "%d dBm", WiFi.RSSI());
    return details;
}   

// General
// Checks the position of the 3-way toggle to put the ESP into WAP exclusive,
// WAP, or Station mode.
uint8_t wifiModeSwitch() {
    uint8_t WAP = digitalRead(WAPswitch);
    uint8_t STA = digitalRead(STAswitch);  

    if (!WAP && STA) { // WAP Exclusive
        return WAP_ONLY;
    } else if (WAP && STA) { // Middle position, WAP Program mode for STA
        return WAP_SETUP;
    } else if (!STA && WAP) { // Station mode reading from NVS
        return STA_ONLY;
    } else {
        return NO_WIFI; 
    }
}
}