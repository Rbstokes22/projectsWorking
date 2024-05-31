#ifndef NETWORK_H
#define NETWORK_H

#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClientSecure.h>
#include "IDisplay.h"

// For secure webserver you can use openssl to make .der files which is a binary
// that the esp accepts. Create and store these in a data directory in the 
// platformio file directory. Configure the platformio.ini file to use the 
// appropriate file system wither SPIFFS or LittleFS.

// Switches
#define WAPswitch 16 // Wireless Access Point
#define STAswitch 17 // Station

// Keywords
#define WAP_ONLY 0
#define WAP_SETUP 1
#define STA_ONLY 2
#define NO_WIFI 255
#define WIFI_STARTING 4
#define WIFI_RUNNING 5

struct STAdetails {
    char SSID[32];
    char IPADDR[16];
    char signalStrength[16];
};

class Net {
    private:
    const char* AP_SSID;
    const char* AP_Pass;
    char ST_SSID[32];
    char ST_PASS[64];
    char phoneNum[15];
    IPAddress local_IP;
    IPAddress gateway;
    IPAddress subnet;
    WebServer server;
    const char* serverCert;
    const char* serverPrivateKey;
    uint16_t port;
    uint8_t prevServerType;
    bool MDNSrunning;
    char error[40];
    bool isServerRunning;
    bool connectedToSTA;

    public:
    Net(
        const char* AP_SSID, 
        const char* AP_Pass,
        IPAddress local_IP,
        IPAddress gateway,
        IPAddress subnet,
        uint16_t port
        );

    void loadCertificates();
    bool WAP(IDisplay &OLED);
    bool WAPSetup(IDisplay &OLED); // Setup LAN setting from WAP
    uint8_t STA(IDisplay &OLED);
    void startServer(IDisplay &OLED);
    STAdetails getSTADetails(); // returns the struct STAdetails
    void handleServer();
    bool isSTAconnected(); // used to start OTA updates
    
};

uint8_t wifiModeSwitch(); // Checks the 3-way switch positon for the correct mode
extern const char WAPsetup[] PROGMEM;

#endif // NETWORK_H