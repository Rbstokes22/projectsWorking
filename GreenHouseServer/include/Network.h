#ifndef NETWORK_H
#define NETWORK_H

#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClientSecure.h>
#include "IDisplay.h"
#include "eeprom.h"

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
#define defaultWAP 4 // button to start WAP with default password

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
    bool isDefaultWAPpass; // Depends if button is pushed in startup

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
    bool WAP(IDisplay &OLED, STAsettings &STAeeprom);
    bool WAPSetup(IDisplay &OLED, STAsettings &STAeeprom); // Setup LAN setting from WAP
    uint8_t STA(IDisplay &OLED, STAsettings &STAeeprom);
    void startServer(IDisplay &OLED, STAsettings &STAeeprom);
    STAdetails getSTADetails(); // returns the struct STAdetails
    void handleServer();
    bool isSTAconnected(); // used to start OTA updates
    void setIsDefaultWAPpass(bool value);
    bool getIsDefaultWAPpass(); // used to show OLED display of (DEF)
};

uint8_t wifiModeSwitch(); // Checks the 3-way switch positon for the correct mode
extern const char WAPsetup[] PROGMEM;

#endif // NETWORK_H