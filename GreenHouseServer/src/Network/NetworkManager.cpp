#include "Network/NetworkManager.h"

namespace networkManager {

// (SETUP) This is called in the setup function of main and will start by reading
// the default switch, which is pressed drop the voltage and inits the
// WAP into default mode with the default password. If switch == HIGH,
// the WAP password will attempt to be read from the NVS. If a password
// exists, it will be used, if not, the WAP will be in default mode.
// If the NVS has corrupt data or data that the client doesn't rememeber,
// they can start in default mode and change the password. Once creds are 
// established, the server routes are setup.
void initializeWAP(
    bool defaultSwitch, FlashWrite::Credentials &Creds, 
    Comms::WirelessAP &wirelessAP, Comms::Station &station,
    Messaging::MsgLogHandler &msglogerr) {    
    
    switch (defaultSwitch) {
        case false:
        msglogerr.handle(Levels::INFO, "Default Mode",Method::OLED); 
        break;

        case true:
        Creds.read("WAPpass"); 

        // Changes default password if one is saved in the NVS.
        if (FlashWrite::Credentials::getWAPpass()[0] != '\0') {
            wirelessAP.setWAPpass(FlashWrite::Credentials::getWAPpass());

        } else {
            msglogerr.handle(
            Levels::INFO, "Default Mode, set WAP password", Method::OLED);
        }
    }

    // Once routes are setup, the server will begin when called by 
    // WAP or STA connect methods.
    wirelessAP.setRoutes(Creds);
    station.setRoutes(Creds);
}

// Sets the OLED display to the WAP type, and if it is the default 
// password, displays that indicator next to the name.
void setWAPtype(char* WAPtype, WIFI wifiMode, bool isWapDef) {
    const char* mode{(wifiMode == WIFI::WAP_ONLY) ? "WAP" : "WAP SETUP"};
    const char* suffix{(isWapDef) ? " (DEF)" : ""};
    sprintf(WAPtype, "%s%s", mode, suffix);
}

// Sends the current data to the OLED for the display to the client
// about the current settings and status. If connected, the 
// conStatus will be changed to yes, if not, itll be no.
void displayWAPstatus(
    UI::Display &OLED, const char* serverName,
    const char* ipaddr, bool conStat, const char* WAPtype, 
    const char* heapHealth, uint8_t clientsConnected) {

    char conStatus[4]{}; // Connected status, yes or no.
    (conStat) ? strcpy(conStatus, "yes") : strcpy(conStatus, "no");
    OLED.printWAP(
        serverName, ipaddr, conStatus, 
        WAPtype, heapHealth, clientsConnected);
    }

// If WL_CONNECTED, this will be passed a true constat, like the WAP
// status. This will allow the client to see if they are connected
// to the LAN amongst the other detail like the SSID, IP, Signal
// Strength, etc...
void displaySTAstatus(
    UI::Display &OLED, bool conStat, 
    Comms::STAdetails &details, const char* heapHealth) {

    char conStatus[4]{}; // connected status
    (conStat) ? strcpy(conStatus, "yes") : strcpy(conStatus, "no");
    OLED.printSTA(details, conStatus, heapHealth);
    }

// Uses the current Free Heap and divides it by the startup Heap.
// It is then multiplied by 100 to give its percentage.
void getHeapHealth(char* heapHealth) {
    size_t freeHeap{(ESP.getFreeHeap() * 100 ) / startupHeap}; // percentage
    sprintf(heapHealth, "%d%%", freeHeap);
}

// (LOOP) This is called from the loop function at a set interval. 
void handleWifiMode(
    Comms::WirelessAP &wirelessAP, Comms::Station &station,
    UI::Display &OLED, UpdateSystem::OTAupdates &otaUpdates,
    FlashWrite::Credentials &Creds, const char* serverPassDefault,
    const char* serverName, const char* ipaddr) {

    char WAPtype[20]{}; // Displays Default or not Default
    char heapHealth[10]{}; // Displays the remaining heap memory

    // Compares the current WAP password to the default password. If equal,
    // OLED will display Default next to the WAP type. Once the password
    // is reset, it will not display Default.
    bool isWapDef{(strcmp(wirelessAP.getWAPpass(), serverPassDefault) == 0)};

    WIFI wifiMode{Comms::wifiModeSwitch()}; // Toggle position.
    bool conStat{false}; // Connected, true or false.
    uint8_t clientsConnected = WiFi.softAPgetStationNum(); 

    setWAPtype(WAPtype, wifiMode, isWapDef); // shows type and if default.
    getHeapHealth(heapHealth); // Gets percentage of free heap.

    // All of the calls to the correct wifi mode (WAP, STA, WAPSetup), will return 
    // a bool if connected. It will then send a call to display the current status
    // of that mode. STA_ONLY is unique because it sends more details since it is 
    // dependent on the LAN connection, so it includes additional data in the 
    // details.
    switch(wifiMode) {
        case WIFI::WAP_ONLY:
        conStat = wirelessAP.WAP(Creds);
        displayWAPstatus(
            OLED, serverName, ipaddr, conStat, 
            WAPtype, heapHealth, clientsConnected); break;

        case WIFI::WAP_SETUP:
        conStat = wirelessAP.WAPSetup(Creds);
        displayWAPstatus(
            OLED, serverName, ipaddr, conStat, 
            WAPtype, heapHealth, clientsConnected); break;

        case WIFI::STA_ONLY:
        // bool value, just care if it is running or not running.
        conStat = (station.STA(Creds) == WIFI::WIFI_RUNNING);
        Comms::STAdetails details{station.getSTADetails()};
        displaySTAstatus(OLED, conStat, details, heapHealth);
    }
}
}