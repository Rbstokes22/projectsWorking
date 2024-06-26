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

enum class NetSize {
    SSID = 32,
    IPADDR = 16,
    SIGSTRENGTH = 16,
    PASS = 64, 
    PHONE = 15,
    KEYQTYWAP = 4, // uses for ssid, pass, phone, and WAPpass
    KEYQTYSTA = 3, // uses only ssid, pass, and phone
    PORT = 80
};

// Key Index used to in conjunction with the keys[], used for 
// indexing values.
enum class KI {ssid, pass, phone, WAPpass}; 

// Communications namespace applies to the Network and servers.
namespace Comms {

// Provides the details of the station connection to OLED.
struct STAdetails {
    char SSID[static_cast<int>(NetSize::SSID)];
    char IPADDR[static_cast<int>(NetSize::IPADDR)];
    char signalStrength[static_cast<int>(NetSize::SIGSTRENGTH)];
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
    static char ST_SSID[static_cast<int>(NetSize::SSID)];
    static char ST_PASS[static_cast<int>(NetSize::PASS)];
    static char phone[static_cast<int>(NetSize::PHONE)]; // will be used for sms service.

    // Allows the main server to be setup exactly once, by either subclass
    // that first calls it.
    static bool isMainServerSetup;

    static const char* keys[static_cast<int>(NetSize::KEYQTYWAP)];

    Messaging::MsgLogHandler &msglogerr;
    
    public:

    // Network/NetworkMain.cpp
    NetMain(Messaging::MsgLogHandler &msglogerr);
    virtual ~NetMain();
    void sendErr(const char* msg); // appends error for net connection errors

    // Network/Routes.cpp
    virtual void setRoutes(NVS::Credentials &Creds) = 0;

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