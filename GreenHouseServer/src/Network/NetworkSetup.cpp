#include "Network/Network.h"
#include <ESPmDNS.h>
#include <WiFi.h>

namespace Comms {

void NetMain::appendErr(const char* msg) {
    size_t remaining = sizeof(this->error) - strlen(this->error) - 1;

    strncat(this->error, msg, remaining);

    if (remaining == 0) {
        this->msglogerr.handle(
            Levels::WARNING,
            "Errors exceeding buffer length",
            Method::SRL
        );
    }
}

// Disconnects clients and wifi, and then reconfigures the system. This
// will occur on password change via WAPSetup server, or when the toggle
// switches position.
void WirelessAP::WAPconnect(uint8_t maxClients) {
    memset(this->error, 0, sizeof(this->error));
    uint8_t errCt{0};

    // Handle Disconnect
    if (!WiFi.mode(WIFI_OFF)) {
            appendErr("Failed to Disconnect WiFi; "); errCt++;
        } 

    // Handle Connection
    if (!WiFi.mode(WIFI_AP)) {
        appendErr("Wifi AP mode unsettable; "); errCt++;
    }

    if (!WiFi.softAPConfig(
        this->local_IP, this->gateway, this->subnet)) {
        appendErr("Unable to config AP; "); errCt++;
    } 

    if (!WiFi.softAP(
        this->AP_SSID, this->AP_PASS, 1, 0, maxClients)) { 
        appendErr("Unable to start AP; "); errCt++;
    }

    if (errCt > 0) {
        this->msglogerr.handle(
            Levels::ERROR, this->error, Method::OLED, Method::SRL
        );
    }  

    if (!NetMain::isServerRunning) startServer();

    if (NetMain::MDNSrunning) {
        MDNS.end(); NetMain::MDNSrunning = false;
    }
}

void Station::STAconnect(FlashWrite::Credentials &Creds) {
    memset(this->error, 0, sizeof(this->error));
    uint8_t errCt{0};

    // Handle Disconnect
    if (!WiFi.mode(WIFI_OFF)) {
            appendErr("Failed to Disconnect AP; "); errCt++;
        }

    // Checks if SSID/PASS have already been created on the WAP Setup page, which
    // will serve as a redundancy to allow station access if the NVS fails.
    // If these dont exist, typical for a fresh start, the NVS will be read
    // to provide credentials. If nothing exists, there will be no SSID or 
    // PASS to connect to station.
    if (NetMain::ST_SSID[0] == '\0' || NetMain::ST_PASS[0] == '\0') {

        Creds.read("ssid"); Creds.read("pass"); Creds.read("phone");

        // Copy the read data to the class variables to make them a primary
        // means of credential entry.
        strncpy(NetMain::ST_SSID, Creds.getSSID(), sizeof(NetMain::ST_SSID) - 1);
        NetMain::ST_SSID[sizeof(NetMain::ST_SSID) - 1] = '\0'; 

        strncpy(NetMain::ST_PASS, Creds.getPASS(), sizeof(NetMain::ST_PASS) - 1);
        NetMain::ST_PASS[sizeof(NetMain::ST_PASS) - 1] = '\0';

        strncpy(NetMain::phone, Creds.getPhone(), sizeof(NetMain::phone) - 1);
        NetMain::phone[sizeof(NetMain::phone) - 1] = '\0';
    } 

    if (!WiFi.mode(WIFI_STA)) {
        appendErr("WiFi STA mode unsettable; "); errCt++;
    }

    if (errCt > 0) {
        this->msglogerr.handle(
            Levels::ERROR, this->error, Method::OLED, Method::SRL);
    }  

    // Does not call begin if there is already an attempt or active connection
    if (WiFi.status() != WL_CONNECTED && WiFi.status() != WL_CONNECT_FAILED) {
        WiFi.begin(NetMain::ST_SSID, NetMain::ST_PASS);
    }
    
    if (!NetMain::isServerRunning) this->startServer();
}

void Station::attemptReconnect(
    FlashWrite::Credentials &Creds, uint8_t freqSec) { // default 10

    static uint64_t elapsedTime = millis();

    if ((millis() - elapsedTime) > freqSec * 1000) {
        STAconnect(Creds); elapsedTime = millis();
    }
}

// MDNS is used for OTA updates when the system is in station mode.

// WIRELESS ACCESS POINT. This is designed to broadcast via wifi, the main
// sensor control page. This will be a closed system that allows 4 clients.
// This will not allow any outside firmware updates or any alerts since it 
// is not connected to the internet.
bool WirelessAP::WAP(FlashWrite::Credentials &Creds) { 
    // check if already in AP mode, setup if not
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
bool WirelessAP::WAPSetup(FlashWrite::Credentials &Creds) {
    // check if already in AP mode, setup if not
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

// STATION IS THE IDEAL MODE. This is deisgned to allow the user to view the controller
// webpage from any device connected to the LAN. This will also have access to the internet
// for SMS updates and alerts, as well as to check for updated firmware. 
WIFI Station::STA(FlashWrite::Credentials &Creds) {
    
    if (WiFi.getMode() != WIFI_STA || NetMain::prevServerType != WIFI::STA_ONLY) {
        STAconnect(Creds);

        // This must follow connection for purposes of handling previous 
        // server disconnections
        NetMain::prevServerType = WIFI::STA_ONLY;

        if (WiFi.status() == WL_CONNECTED) {
            if (!MDNSrunning) {
                MDNS.begin("esp32"); MDNSrunning = true;
            }
            this->connectedToSTA = true;
            return WIFI::WIFI_RUNNING;
        } else {
            this->connectedToSTA = false;
            return WIFI::WIFI_STARTING;
        }

    } else {
        // This is essential to the non-blocking WiFi begin. After being
        // called, this else block will cycle and catch the connection once
        // connected. This ensure exactly one setup.
        if (WiFi.status() == WL_CONNECTED) {
            if (!MDNSrunning) {
                MDNS.begin("esp32"); MDNSrunning = true;
            }
            this->connectedToSTA = true; // used for OTA updates
            return WIFI::WIFI_RUNNING;
        } else {
            attemptReconnect(Creds, 10);
            this->connectedToSTA = false;
            return WIFI::WIFI_STARTING;
        }
    }
}

}