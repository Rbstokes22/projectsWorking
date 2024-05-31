#include "Network.h"
#include <ESP8266mDNS.h>
#include "eeprom.h"
#include <ESP8266WiFi.h>
#include <BearSSLHelpers.h>
#include <CertStoreBearSSL.h>
#include <FS.h>
#include <LittleFS.h>

void Net::loadCertificates() {
    
        Serial.println("SETTING UP NETWORK");
        // Start the file system
        if(!LittleFS.begin()) {
            strcpy(this->error, "failed LittleFS mount");
            Serial.println("LITTLE FS ERROR"); return;
            // OLED.displayError(this->error);
        } 

        // Load the file certificate
        File cert = LittleFS.open("/certificate.der", "r");
        if(!cert) {
            strcpy(this->error, "Failed to open security cert");
            Serial.println("cert ERROR"); return;
            // OLED.displayError(this->error); return;
        }

        size_t certSize = cert.size();
        uint8_t* certBuffer = new uint8_t[certSize];
        if (cert.read(certBuffer, certSize) == certSize) {
            this->serverCert.append(certBuffer, certSize);
        }
        delete[] certBuffer; cert.close();

        // Load the private key
        File privateKey = LittleFS.open("/private_key.der", "r");
        if (!privateKey) {
            strcpy(this->error, "Failed to open private key");
            Serial.println("KEy ERROR"); return;
            // OLED.displayError(this->error); return;
        }

        size_t keySize = privateKey.size();
        uint8_t* keyBuffer = new uint8_t[keySize];
        if (privateKey.read(keyBuffer, keySize) == keySize) {
            this->serverPrivateKey.parse(keyBuffer, keySize);
        }
        delete[] keyBuffer; privateKey.close();

        this->server.getServer().setRSACert(&this->serverCert, &this->serverPrivateKey);
}

// WIRELESS ACCESS POINT EXCLUSIVE. This will host the same exact page as the station
// which is the main control for the device. It will just do so with the controlling
// device connects to the device via wifi. Will not have any connection internet.

bool Net::WAP(IDisplay &OLED) { 
    // check if already in AP mode, setup if not
    Serial.println("ON WAP MODE");
    if (WiFi.getMode() != WIFI_AP || prevServerType != WAP_ONLY) {
        prevServerType = WAP_ONLY;
        WiFi.mode(WIFI_AP);
        WiFi.softAPConfig(local_IP, gateway, subnet);
        WiFi.softAP(AP_SSID, AP_Pass);
        if (!this->isServerRunning) startServer(OLED);
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

bool Net::WAPSetup(IDisplay &OLED) {
    // check if already in AP mode, setup if not
    if (WiFi.getMode() != WIFI_AP || prevServerType != WAP_SETUP) {
        prevServerType = WAP_SETUP;
        WiFi.mode(WIFI_AP);
        WiFi.softAPConfig(local_IP, gateway, subnet);
        WiFi.softAP(AP_SSID, AP_Pass);
        if (!this->isServerRunning) startServer(OLED);
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

uint8_t Net::STA(IDisplay &OLED) {
    
    if (WiFi.getMode() != WIFI_STA || prevServerType != STA_ONLY) {
        // resetServer();
        WiFi.mode(WIFI_STA);
        this->prevServerType = STA_ONLY;

        // Checks if this has already been created by the WAP setup page. This will
        // happen upon attempting a write to the EEPROM of the SSID, PASS, and phone.
        // This serves as a catch for an EEPROM failure. This allows the user to 
        // sign in at the WAP setup page, which stores the variables, and then will
        // connect. The EEPROM serves to provide this data upon startup and is the 
        // primary. A reminder though, without EEPROM, the LAN login creds are volatile.

        if (this->ST_SSID[0] == '\0' || this->ST_PASS[0] == '\0') {
            STAsettings readEEPROM; // reads eeprom to get stored vals up creation
            readEEPROM.eepromRead(0); // reads epprom data and sets class variables

            // The readEEPROM returns char pointers, which will go out of scope and 
            // cause issues after initialization in this function. To prevent that and 
            // store the class variables, we use these functions, and then have no 
            // follow on issues. These need to be mutable.
            strncpy(this->ST_SSID, readEEPROM.getSSID(), sizeof(this->ST_SSID) - 1);
            this->ST_SSID[sizeof(this->ST_SSID) - 1] = '\0'; // Ensure null terminator exists

            strncpy(this->ST_PASS, readEEPROM.getPASS(), sizeof(this->ST_PASS) - 1);
            this->ST_PASS[sizeof(this->ST_PASS) - 1] = '\0';

            strncpy(this->phoneNum, readEEPROM.getPhone(), sizeof(this->phoneNum) - 1);
            this->phoneNum[sizeof(this->phoneNum) - 1] = '\0';
        } 
        
        WiFi.begin(this->ST_SSID, this->ST_PASS);
        if (!this->isServerRunning) startServer(OLED);

        if (WiFi.status() == WL_CONNECTED) {
            if (!MDNSrunning) {
                MDNS.begin("esp8266"); MDNSrunning = true;
            }
            return WIFI_RUNNING;
        } else {
            return WIFI_STARTING;
        }
    } else {
        if (WiFi.status() == WL_CONNECTED) {
            if (!MDNSrunning) {
                MDNS.begin("esp8266"); MDNSrunning = true;
            }
            return WIFI_RUNNING;
        } else {
            return WIFI_STARTING;
        }
    }
}