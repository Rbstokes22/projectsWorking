#include "Network.h"
#include <ESPmDNS.h>
#include <WiFi.h>

namespace Comms {

// OLED is passed to each function for implementation of any error reporting
// to the OLED display.

// MDNS is used for OTA updates when the system is in station mode.

// WIRELESS ACCESS POINT. This is designed to broadcast via wifi, the main
// sensor control page. This will be a closed system that allows 4 clients.
// This will not allow any outside firmware updates or any alerts since it 
// is not connected to the internet.
bool WirelessAP::WAP(UI::IDisplay &OLED, FlashWrite::Credentials &Creds) { 
    // check if already in AP mode, setup if not
    if (WiFi.getMode() != WIFI_AP || NetMain::prevServerType != WAP_ONLY) {
        NetMain::prevServerType = WAP_ONLY;
        WiFi.mode(WIFI_AP);
        WiFi.softAPConfig(this->local_IP, this->gateway, this->subnet);
        WiFi.softAP(this->AP_SSID, this->AP_PASS, 1, 0, 4); // Allows 4 connections
        if (!NetMain::isServerRunning) startServer();
        if (NetMain::MDNSrunning) {
            MDNS.end(); NetMain::MDNSrunning = false;
        }
        return false; // connecting
    } else {
        return true; // running
    }
}

// WIRELESS ACCESS POINT TO SETUP LAN CONNECTION. This is designed to broadcast via
// wifi, the setup page where the user can enter their SSID, password, phone, and
// WAP password. This is limited to 1 client for security purposes considering 
// this in an unsecured network. 
bool WirelessAP::WAPSetup(UI::IDisplay &OLED, FlashWrite::Credentials &Creds) {
    // check if already in AP mode, setup if not
    if (WiFi.getMode() != WIFI_AP || NetMain::prevServerType != WAP_SETUP) {
        NetMain::prevServerType = WAP_SETUP;
        WiFi.mode(WIFI_AP);
        WiFi.softAPConfig(this->local_IP, this->gateway, this->subnet);
        WiFi.softAP(this->AP_SSID, this->AP_PASS, 1, 0, 1); // Allows 1 connection for security
        if (!NetMain::isServerRunning) startServer();
        if (NetMain::MDNSrunning) {
            MDNS.end(); NetMain::MDNSrunning = false;
        }
        return false; // connecting
    } else {
        return true; // running
    }
}

// STATION IS THE IDEAL MODE. This is deisgned to allow the user to view the controller
// webpage from any device connected to the LAN. This will also have access to the internet
// for SMS updates and alerts, as well as to check for updated firmware. 
uint8_t Station::STA(UI::IDisplay &OLED, FlashWrite::Credentials &Creds) {
    
    if (WiFi.getMode() != WIFI_STA || NetMain::prevServerType != STA_ONLY) {
        WiFi.mode(WIFI_STA);
        NetMain::prevServerType = STA_ONLY;

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
        
        // Non-blocking, if it doesnt connect, the OLED will show that. This
        // is to avoid a blocking station connection preventing access to the 
        // wireless AP.
        WiFi.begin(NetMain::ST_SSID, NetMain::ST_PASS);
        if (!NetMain::isServerRunning) this->startServer();

        if (WiFi.status() == WL_CONNECTED) {
            if (!MDNSrunning) {
                MDNS.begin("esp32"); MDNSrunning = true;
            }
            this->connectedToSTA = true;
            return WIFI_RUNNING;
        } else {
            this->connectedToSTA = false;
            return WIFI_STARTING;
        }
    } else {
        // This is essential to the non-blocking WiFi begin. After being
        // called, this else block will cycle and catch the connection once
        // connected. This ensure exactly one setup.
        if (WiFi.status() == WL_CONNECTED) {
            if (!MDNSrunning) {
                MDNS.begin("esp32"); MDNSrunning = true;
            }
            this->connectedToSTA = true;
            return WIFI_RUNNING;
        } else {
            this->connectedToSTA = false;
            return WIFI_STARTING;
        }
    }
}

}