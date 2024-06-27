#include "Network/NetworkWAP.h"

namespace Comms {

WirelessAP::WirelessAP(
    const char* AP_SSID, const char* AP_PASS,
    IPAddress local_IP, IPAddress gateway,
    IPAddress subnet, Messaging::MsgLogHandler &msglogerr) :

    NetMain(msglogerr), local_IP{local_IP}, gateway{gateway},
    subnet{subnet} {

    // set the arguments to the class variables. This will always send
    // the default password upon creation. It will then be changed to the
    // NVS password upon init, if one exists. 
    strncpy(this->AP_SSID, AP_SSID, sizeof(this->AP_SSID) - 1);
    this->AP_SSID[sizeof(this->AP_SSID) - 1] = '\0';

    strncpy(this->AP_PASS, AP_PASS, sizeof(this->AP_PASS) - 1);
    this->AP_PASS[sizeof(this->AP_PASS) - 1] = '\0';

    // Uses the struct CredInfo from NVS.
    this->credinfo[0] = {keys[static_cast<int>(KI::ssid)], 
        NetMain::ST_SSID, sizeof(NetMain::ST_SSID)
        };
    this->credinfo[1] = {keys[static_cast<int>(KI::pass)], 
        NetMain::ST_PASS, sizeof(NetMain::ST_PASS)
        };
    this->credinfo[2] = {keys[static_cast<int>(KI::phone)], 
        NetMain::phone, sizeof(NetMain::phone)
        };
    this->credinfo[3] = {keys[static_cast<int>(KI::WAPpass)], 
        this->AP_PASS, sizeof(this->AP_PASS)
        };
}

const char* WirelessAP::getWAPpass() {
    return this->AP_PASS;
}

// This is setup in the network manager upon init. If a WAP pass exists in
// NVS, it will be set there. Upon a new password creation in WAP setup 
// page, it will be directly set and not use this function.
void WirelessAP::setWAPpass(const char* pass) {
    strncpy(this->AP_PASS, pass, sizeof(this->AP_PASS) - 1);
    this->AP_PASS[sizeof(this->AP_PASS) - 1] = '\0';
}

// Disconnects clients and wifi, and then reconfigures the system. This
// will occur on password change via WAPSetup server, or when the toggle
// switches position.
void WirelessAP::WAPconnect(uint8_t maxClients) {

    // Handle Disconnect
    if (!WiFi.mode(WIFI_OFF)) this->sendErr("Failed to Disconnect WiFi"); 
  

    // Handle Connection
    if (!WiFi.mode(WIFI_AP)) this->sendErr("Wifi AP mode unsettable"); 
    
    if (!WiFi.softAPConfig(
        this->local_IP, this->gateway, this->subnet)) {
        this->sendErr("Unable to config AP"); 
    }

    if (!WiFi.softAP(
        this->AP_SSID, this->AP_PASS, 1, 0, maxClients)) { 
        this->sendErr("Unable to start AP"); 
    }

    if (!NetMain::isServerRunning) startServer();

    if (NetMain::MDNSrunning) {
        MDNS.end(); NetMain::MDNSrunning = false;
    }
}

// WIRELESS ACCESS POINT. This is designed to broadcast via wifi, the main
// sensor control page. This will be a closed system that allows 4 clients.
// This will not allow any outside firmware updates or any alerts since it 
// is not connected to the internet.
bool WirelessAP::WAP(NVS::Credentials &Creds) { 
    
    if (WiFi.getMode() != WIFI_AP || NetMain::prevServerType != WIFI::WAP_ONLY) {
        this->WAPconnect(4); // 4 clients max

        // This must follow connection for purposes of handling previous 
        // server disconnections
        NetMain::prevServerType = WIFI::WAP_ONLY; 

        return false; // connecting
    } else {
        return true; // running
    }
}

// WIRELESS ACCESS POINT TO SETUP LAN CONNECTION. This is designed to broadcast via
// wifi, the setup page where the user can enter their SSID, password, phone, and
// WAP password. This is limited to 1 client for security purposes considering 
// this in an unsecured network. 
bool WirelessAP::WAPSetup(NVS::Credentials &Creds) {
  
    if (WiFi.getMode() != WIFI_AP || NetMain::prevServerType != WIFI::WAP_SETUP) {
        WAPconnect(1); // 1 client for security reasons

        // This must follow connection for purposes of handling previous 
        // server disconnections
        NetMain::prevServerType = WIFI::WAP_SETUP;

        return false; // connecting
    } else {
        return true; // running
    }
}

}