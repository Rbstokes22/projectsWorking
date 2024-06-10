#include "Network.h"
#include <ESPmDNS.h>
#include <WiFi.h>

namespace Comms {

// WIRELESS ACCESS POINT EXCLUSIVE. This will host the same exact page as the station
// which is the main control for the device. It will just do so with the controlling
// device connects to the device via wifi. Will not have any connection internet.

bool Net::WAP(UI::IDisplay &OLED, FlashWrite::Credentials &Creds) { 
    // check if already in AP mode, setup if not
    if (WiFi.getMode() != WIFI_AP || prevServerType != WAP_ONLY) {
        prevServerType = WAP_ONLY;
        WiFi.mode(WIFI_AP);
        WiFi.softAPConfig(local_IP, gateway, subnet);
        WiFi.softAP(AP_SSID, AP_Pass, 1, 0, 4); // Allows 4 connections
        if (!this->isServerRunning) startServer(OLED, Creds);
        if (MDNSrunning) {
            MDNS.end(); MDNSrunning = false;
        }
        return false; // connecting
    } else {
        return true; // running
    }
}

// WIRELESS ACCESS POINT TO SETUP LAN CONNECTION. This is designed to broadcast via
// wifi, the setup page where the user can enter their SSID, password, and phone 
// for a twilio account altert systems. Once this is set up, the client can go 
// to station mode.

bool Net::WAPSetup(UI::IDisplay &OLED, FlashWrite::Credentials &Creds) {
    // check if already in AP mode, setup if not
    if (WiFi.getMode() != WIFI_AP || prevServerType != WAP_SETUP) {
        prevServerType = WAP_SETUP;
        WiFi.mode(WIFI_AP);
        WiFi.softAPConfig(local_IP, gateway, subnet);
        WiFi.softAP(AP_SSID, AP_Pass, 1, 0, 1); // Allows 1 connection for security
        if (!this->isServerRunning) startServer(OLED, Creds);
        if (MDNSrunning) {
            MDNS.end(); MDNSrunning = false;
        }
        return false; // connecting
    } else {
        return true; // running
    }
}

// STATION IS THE IDEAL MODE. This is deisgned to allow the user to view the controller
// webpage from any device connected to the LAN. This will also have access to the internet
// for SMS updates and alerts, as well as to check for upgraded firmware. 

uint8_t Net::STA(UI::IDisplay &OLED, FlashWrite::Credentials &Creds) {
    
    if (WiFi.getMode() != WIFI_STA || prevServerType != STA_ONLY) {
        WiFi.mode(WIFI_STA);
        this->prevServerType = STA_ONLY;

        // Checks if this has already been created by the WAP setup page. This will
        // happen upon attempting a write to the NVS of the SSID, PASS, and phone.
        // This serves as a catch for an NVS failure. This allows the user to 
        // sign in at the WAP setup page, which stores the variables, and then will
        // connect. The NVS serves to provide this data upon startup and is the 
        // primary. A reminder though, without NVS, the LAN login creds are volatile.

        if (this->ST_SSID[0] == '\0' || this->ST_PASS[0] == '\0') {

            Creds.read("ssid"); Creds.read("pass"); Creds.read("phone");

            // I copy the ssid, pass, and phone to the actual network variables. This
            // work with the above logic with the WAP setup page and serves as a
            // redundancy and eases some logic.
            strncpy(this->ST_SSID, Creds.getSSID(), sizeof(this->ST_SSID) - 1);
            this->ST_SSID[sizeof(this->ST_SSID) - 1] = '\0'; // Ensure null terminator exists

            strncpy(this->ST_PASS, Creds.getPASS(), sizeof(this->ST_PASS) - 1);
            this->ST_PASS[sizeof(this->ST_PASS) - 1] = '\0';

            strncpy(this->phone, Creds.getPhone(), sizeof(this->phone) - 1);
            this->phone[sizeof(this->phone) - 1] = '\0';
        } 
        
        // Done need delay to wait for web, this is a non blocking begin.
        WiFi.begin(this->ST_SSID, this->ST_PASS);
        if (!this->isServerRunning) startServer(OLED, Creds);

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