#ifndef NETWORKMAIN_H
#define NETWORKMAIN_H

#include <WiFi.h>
#include <WebServer.h>
#include "UI/MsgLogHandler.h"
#include "Network/Creds.h"
#include <pgmspace.h>
#include <ESPmDNS.h>

// This is an unsecure web network meant to run on the LAN. The risk is not worth
// the overhead of working this to be an https server. In normal operation the 
// client puts the switch to the middle position to enter the LAN information to 
// allow station connection. The attacker would also have to be present at that 
// exact moment, connected to the WAP network which is limited to 1 connection,
// be sniffing the packets during that 30 second window of passing the information,
// and their reward would be your password to your network. The probability is 
// quite low. 

// BOTH NetworkSTA.h and NetworkWAP.h include NetworkMain.h. For your include 
// statements, unless exclusive to NetworkMain.h, you can use either of the two.

// Switches
enum class NETPIN : uint8_t {
    WAPswitch = 16, // Wireless Access Point
    STAswitch = 17, // Station
    defaultWAPSwitch = 4 // button to start WAP with default password
};

enum class WIFI : uint8_t { // WIFI variables to check connection
    WAP_ONLY, WAP_SETUP, STA_ONLY, NO_WIFI, WIFI_STARTING, WIFI_RUNNING
};

// Communications namespace applies to the Network and servers.
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
    static WIFI prevServerType;
    static bool MDNSrunning;
    static bool isServerRunning;

    // Unlike AP data, these are set in WAP and used in STA so they are 
    // shared between subclasses.
    static char ST_SSID[32];
    static char ST_PASS[64];
    static char phone[15]; // will be used for sms service.

    // Allows the main server to be setup exactly once, by either subclass
    // that first calls it.
    static bool isMainServerSetup;

    char error[168]; // 168 Capacity at OLED (21 * 8) size 1
    Messaging::MsgLogHandler &msglogerr;
    
    public:

    // Network/NetworkMain.cpp
    NetMain(Messaging::MsgLogHandler &msglogerr);
    virtual ~NetMain();
    void appendErr(const char* msg); // appends error for net connection errors

    // Network/Routes.cpp
    virtual void setRoutes(FlashWrite::Credentials &Creds) = 0;

    // Network/Server/ServerStart.cpp
    void startServer();
    void handleNotFound();
    void handleServer();

    // Network/Server/ServerMain.cpp
    void handleIndex();  
};

// Network/NetworkMain.cpp
WIFI wifiModeSwitch(); // 3-way toggle switch position

}

#endif // NETWORKMAIN_H