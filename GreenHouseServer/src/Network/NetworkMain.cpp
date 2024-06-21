#include "Network/Network.h"
#include <WiFi.h>

// This source file provides the general defintions of the NetMain superclass
// and its sub-classes. 

namespace Comms {

// STATIC VARIABLE DEFINITIONS
WebServer NetMain::server(80);
WIFI NetMain::prevServerType{WIFI::NO_WIFI};
bool NetMain::isServerRunning{false};
bool NetMain::MDNSrunning{false};
char NetMain::ST_SSID[32]{};
char NetMain::ST_PASS[64]{};
char NetMain::phone[15]{};
bool NetMain::mainServerSetup{false}; // Allows a single setup

// NetMain 
NetMain::NetMain(Messaging::MsgLogHandler &msglogerr) :
    msglogerr(msglogerr) {

    // Initialize the arrays with null chars
    memset(this->error, 0, sizeof(this->error));
}

NetMain::~NetMain(){}

// WAP
WirelessAP::WirelessAP(
    const char* AP_SSID, const char* AP_PASS,
    IPAddress local_IP, IPAddress gateway,
    IPAddress subnet, Messaging::MsgLogHandler &msglogerr) :

    NetMain(msglogerr),
    local_IP{local_IP},
    gateway{gateway},
    subnet{subnet}

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
Station::Station(Messaging::MsgLogHandler &msglogerr) : 
    NetMain(msglogerr), connectedToSTA{false} {}

// Provides the OLED with all of the station details, and seems redundant.
// It is not redundant because it will allow the client to see if there is 
// an error or something was changed or corrupted.
STAdetails Station::getSTADetails() {
    STAdetails details{"", "", ""};
    strcpy(details.SSID, NetMain::ST_SSID);
    IPAddress ip{WiFi.localIP()};
    sprintf(details.IPADDR, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    sprintf(details.signalStrength, "%d dBm", WiFi.RSSI());
    return details;
}   

// General
// Checks the position of the 3-way toggle to put the ESP into WAP exclusive,
// WAP, or Station mode.
WIFI wifiModeSwitch() {
    uint8_t WAP = digitalRead(static_cast<int>(NETPIN::WAPswitch));
    uint8_t STA = digitalRead(static_cast<int>(NETPIN::STAswitch));  

    if (!WAP && STA) { // WAP Exclusive
        return WIFI::WAP_ONLY;
    } else if (WAP && STA) { // Middle position, WAP Program mode for STA
        return WIFI::WAP_SETUP;
    } else if (!STA && WAP) { // Station mode reading from NVS
        return WIFI::STA_ONLY;
    } else {
        return WIFI::NO_WIFI; 
    }
}
}