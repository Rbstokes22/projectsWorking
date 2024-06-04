#ifndef NETWORK_H
#define NETWORK_H

#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClientSecure.h>
#include "IDisplay.h"
#include "Creds.h"

// This is an unsecure web network meant to run on the LAN. The risk is not worth
// the overhead of working this to be an https server. In normal operation the 
// client puts the switch to the middle position to enter the LAN information to 
// allow station connection. The attacker would also have to be present at that 
// exact moment, connected to the WAP network which is limited to 1 connection,
// be sniffing the packets during that 30 second window of passing the information,
// and their reward would be your password to your network. The probability is 
// quite low. 

// Switches
#define WAPswitch 16 // Wireless Access Point
#define STAswitch 17 // Station
#define defaultWAPSwitch 4 // button to start WAP with default password

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
    char AP_SSID[15];
    char AP_Pass[64];
    char ST_SSID[32];
    char ST_PASS[64];
    char phoneNum[15];
    IPAddress local_IP;
    IPAddress gateway;
    IPAddress subnet;
    WebServer server;
    uint16_t port;
    uint8_t prevServerType;
    bool MDNSrunning;
    char error[40];
    bool isServerRunning;
    bool connectedToSTA;

    public:
    // Network/NetworkMain.cpp
    Net(
        const char* AP_SSID, 
        const char* AP_Pass,
        IPAddress local_IP,
        IPAddress gateway,
        IPAddress subnet,
        uint16_t port
        );

    STAdetails getSTADetails(); // returns the struct STAdetails
    const char* getWAPpass();

    // Network/NetworkSetup.cpp
    bool WAP(IDisplay &OLED, Credentials &EEPROMcreds);
    bool WAPSetup(IDisplay &OLED, Credentials &EEPROMcreds); // Setup LAN setting from WAP
    uint8_t STA(IDisplay &OLED, Credentials &EEPROMcreds);

    // Network/Server/ServerStart.cpp
    void startServer(IDisplay &OLED, Credentials &EEPROMcreds);
    void handleNotFound();
    void handleServer();
    bool isSTAconnected(); // used to start OTA updates

    // Network/Server/ServerMain.cpp
    void handleIndex();

    // Network/Server/ServerWapSubmit.cpp
    void handleWAPsubmit(IDisplay &OLED, Credentials &EEPROMcreds);
    void eepromWriteRespond(
        const char* type, 
        Credentials &EEPROMcreds,
        char* buffer);
    void handleJson(IDisplay &OLED, Credentials &EEPROMcreds);
};

// Network/NetworkMain.cpp
uint8_t wifiModeSwitch(); // Checks the 3-way switch positon for the correct mode

// Network/webPages.cpp
extern const char WAPsetup[] PROGMEM;

#endif // NETWORK_H