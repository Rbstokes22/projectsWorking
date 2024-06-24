#include "Network/NetworkManager.h"

namespace networkManager {

void initializeWAP(
    bool defaultSwitch, FlashWrite::Credentials &Creds, 
    Comms::WirelessAP &wirelessAP, Comms::Station &station,
    Messaging::MsgLogHandler &msglogerr
    ) {    
    
    char msg[50]{};
    switch (defaultSwitch) {
        case false:
        msglogerr.handle(
            Levels::INFO,
            "Default Mode",
            Method::OLED); break;

        case true:
        Creds.read("WAPpass"); 

        // Changes default password if one is saved in the NVS.
        if (Creds.getWAPpass()[0] != '\0') {
            wirelessAP.setWAPpass(Creds.getWAPpass());

        } else {
            msglogerr.handle(
            Levels::INFO,
            "Default Mode, set WAP password",
            Method::OLED);
        }
    }

    // After initialization, sets up server routes. Once routes are setup, the server will
    // begin when called by WAP or STA connect methods.
    wirelessAP.setRoutes(Creds);
    station.setRoutes(Creds);
}

void setWAPtype(char* WAPtype, WIFI wifiMode, bool isWapDef) {
    const char* mode{(wifiMode == WIFI::WAP_ONLY) ? "WAP" : "WAP SETUP"};
    const char* suffix{(isWapDef) ? " (DEF)" : ""};
    sprintf(WAPtype, "%s%s", mode, suffix);
}

void displayWAPstatus(
    UI::Display &OLED, const char* serverName,
    const char* ipaddr, bool conStat, const char* WAPtype, 
    const char* heapHealth, uint8_t clientsConnected
    ) {

    char conStatus[4]{}; // connected status
    (conStat) ? strcpy(conStatus, "yes") : strcpy(conStatus, "no");
    OLED.printWAP(
        serverName, ipaddr, conStatus, 
        WAPtype, heapHealth, clientsConnected
        );
    }

void displaySTAstatus(
    UI::Display &OLED, bool conStat, 
    Comms::STAdetails &details, const char* heapHealth
    ) {

    char conStatus[4]{}; // connected status
    (conStat) ? strcpy(conStatus, "yes") : strcpy(conStatus, "no");
    OLED.printSTA(details, conStatus, heapHealth);
    }

void getHeapHealth(char* heapHealth) {
    size_t freeHeap{(ESP.getFreeHeap() * 100 ) / startupHeap}; // percentage
    sprintf(heapHealth, "%d%%", freeHeap);
}

// Displays the current state of the wifi connection or broadcast. This is also
// the function responsible for running the WAP, WAP Setup, or STA mode based on
// the position of the toggle switch.
void handleWifiMode(
    Comms::WirelessAP &wirelessAP, Comms::Station &station,
    UI::Display &OLED, UpdateSystem::OTAupdates &otaUpdates,
    FlashWrite::Credentials &Creds, const char* serverPassDefault,
    const char* serverName, const char* ipaddr
    ) {

    char WAPtype[20]{};
    char heapHealth[10]{};

    // compares to the current set WAP password, if it == the default, then 
    // the OLED will display the (DEF) follwoing the WAP type. This will disappear
    // when the AP_Pass is reset. It allows dynamic chaning.
    bool isWapDef{(strcmp(wirelessAP.getWAPpass(), serverPassDefault) == 0)};

    WIFI wifiMode{Comms::wifiModeSwitch()}; // checks toggle position
    bool conStat{false}; // connected status to the WAP or STA
    uint8_t clientsConnected = WiFi.softAPgetStationNum();

    setWAPtype(WAPtype, wifiMode, isWapDef); // shows type and if default
    getHeapHealth(heapHealth);

    switch(wifiMode) {
        case WIFI::WAP_ONLY:
        conStat = wirelessAP.WAP(Creds);
        displayWAPstatus(
            OLED, serverName, ipaddr, conStat, 
            WAPtype, heapHealth, clientsConnected
            ); break;

        case WIFI::WAP_SETUP:
        conStat = wirelessAP.WAPSetup(Creds);
        displayWAPstatus(
            OLED, serverName, ipaddr, conStat, 
            WAPtype, heapHealth, clientsConnected
            ); break;

        case WIFI::STA_ONLY:
        // bool checks if it is running
        conStat = (station.STA(Creds) == WIFI::WIFI_RUNNING);
        Comms::STAdetails details{station.getSTADetails()};
        displaySTAstatus(OLED, conStat, details, heapHealth);
    }
}
}