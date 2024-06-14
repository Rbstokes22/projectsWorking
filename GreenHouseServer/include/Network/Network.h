#ifndef NETWORK_H
#define NETWORK_H

#include <WiFi.h>
#include <WebServer.h>
#include "IDisplay.h"
#include "Creds.h"
#include <pgmspace.h>

// This is an unsecure web network meant to run on the LAN. The risk is not worth
// the overhead of working this to be an https server. In normal operation the 
// client puts the switch to the middle position to enter the LAN information to 
// allow station connection. The attacker would also have to be present at that 
// exact moment, connected to the WAP network which is limited to 1 connection,
// be sniffing the packets during that 30 second window of passing the information,
// and their reward would be your password to your network. The probability is 
// quite low. 

// Switches
enum NetworkPins {
    WAPswitch = 16, // Wireless Access Point
    STAswitch = 17, // Station
    defaultWAPSwitch = 4 // button to start WAP with default password
};

enum WIFI { // Wifi keywords
    WAP_ONLY,
    WAP_SETUP,
    STA_ONLY,
    NO_WIFI,
    WIFI_STARTING,
    WIFI_RUNNING
};

// Communications namespace applies to the Network and servers in this 
// header and source files.
namespace Comms {

// Provides the details of the station connection to OLED.
struct STAdetails {
    char SSID[32];
    char IPADDR[16];
    char signalStrength[16];
};

// Most variables are declared as static since they must be shared between
// all class instances, especially when switching between WAP and STA.
class NetMain { // Abstract class
    protected:
    static WebServer server;
    static uint8_t prevServerType;
    static bool MDNSrunning;
    static bool isServerRunning;

    // Unlike AP data, these are set in WAP and used in STA so they are 
    // shared between subclasses.
    static char ST_SSID[32];
    static char ST_PASS[64];
    static char phone[15]; // will be used for sms service.

    // Allows the main server to be setup exactly once, by either subclass
    // that first calls it.
    static bool mainServerSetup;

    char error[40]; // used for error/msg reporting to client
    
    public:

    // Network/NetworkMain.cpp
    NetMain();
    virtual ~NetMain();

    // Network/Routes.cpp
    virtual void setRoutes(UI::IDisplay &OLED, FlashWrite::Credentials &Creds) = 0;

    // Network/Server/ServerStart.cpp
    
    void startServer();
    void handleNotFound();
    void handleServer();

    // Network/Server/ServerMain.cpp
    void handleIndex();  
};

class WirelessAP : public NetMain {
    private:

    // These variables are exclusive to the WAP, thus are not shared by all
    // subclasses to NetMain.
    char AP_SSID[15];
    char AP_PASS[64];
    IPAddress local_IP;
    IPAddress gateway;
    IPAddress subnet;

    public:

    // Network/NetworkMain.cpp
    WirelessAP(const char* AP_SSID, const char* AP_PASS,
        IPAddress local_IP, IPAddress gateway, IPAddress subnet     
        );

    const char* getWAPpass();
    void setWAPpass(const char* pass);
    
    // Network/Routes.cpp
    void setRoutes(UI::IDisplay &OLED, FlashWrite::Credentials &Creds) override;

    // Network/Server/ServerWapSubmit.cpp
    void handleWAPsubmit(UI::IDisplay &OLED, FlashWrite::Credentials &Creds);
    void commitAndRespond(
        const char* type, FlashWrite::Credentials &Creds,char* buffer
        );

    void handleJson(UI::IDisplay &OLED, FlashWrite::Credentials &Creds);

    // Netowrk/NetworkSetup.cpp
    bool WAP(UI::IDisplay &OLED, FlashWrite::Credentials &Creds);
    bool WAPSetup(UI::IDisplay &OLED, FlashWrite::Credentials &Creds); // Setup LAN setting from WAP
};

class Station : public NetMain {
    private:
    bool connectedToSTA; // used for OTA updates.

    public:

    // Network/NetworkMain.cpp
    Station();

    // Network/Routes.cpp
    void setRoutes(UI::IDisplay &OLED, FlashWrite::Credentials &Creds) override;

    // Network/Server/Server/ServerStart.cpp
    STAdetails getSTADetails(); 
    uint8_t STA(UI::IDisplay &OLED, FlashWrite::Credentials &Creds);
    bool isSTAconnected(); // used to start OTA updates
};

// Network/NetworkMain.cpp
uint8_t wifiModeSwitch(); // 3-way toggle switch position

// Network/webPages.cpp
extern const char WAPsetup[] PROGMEM;
}

#endif // NETWORK_H

