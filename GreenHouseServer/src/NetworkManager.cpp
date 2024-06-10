#include "NetworkManager.h"


namespace networkManager {

void netConstructor(
    bool isDefault, 
    Comms::Net &Network,
    UI::Display &OLED,
    FlashWrite::Credentials &Creds,
    const char* errorMsg, 
    bool sendMsg
    ) {
    char error[50];
    switch (isDefault) {
        case true:

            if (sendMsg == true) {
            strcpy(error, errorMsg);
            OLED.displayError(error);
            }
            break;

        case false:
        Network.setWAPpass(Creds.getWAPpass());
    }
}

void initializeNet(
    bool defaultSwitch, 
    FlashWrite::Credentials &Creds, 
    Comms::Net &Network, 
    UI::Display &OLED
) {
    switch (defaultSwitch) {
        case false:
        netConstructor(
            true, 
            Network,
            OLED,
            Creds,
            "Default Mode"
            ); break;

        // INIT THE WAP PASS HERE Using CREDS
        case true:
        Creds.read("WAPpass"); // sets the WAPpass array
        if (Creds.getWAPpass()[0] != '\0') {
        netConstructor(false, Network, OLED, Creds, "");
        } else {
        netConstructor(true, Network, OLED, Creds, "Set WAP Pass");
        }
    }
}

void setWAPtype(char* WAPtype, uint8_t wifiMode, bool isWapDef) {
    const char* mode = (wifiMode == WAP_ONLY) ? "WAP" : "WAP SETUP";
    const char* suffix = (isWapDef) ? " (DEF)" : "";
    sprintf(WAPtype, "%s%s", mode, suffix);
}

void displayWAPstatus(
    UI::Display &OLED, 
    const char* serverName,
    const char* ipaddr,
    bool conStat, 
    const char* WAPtype, 
    bool updatingStatus, 
    const char* heapHealth) {

    char conStatus[4]; // connected status
    (conStat) ? strcpy(conStatus, "yes") : strcpy(conStatus, "no");
    OLED.printWAP(serverName, ipaddr, conStatus, WAPtype, updatingStatus, heapHealth);
    }

void displaySTAstatus(
    UI::Display &OLED, 
    bool conStat, 
    Comms::STAdetails details, 
    bool updatingStatus, 
    const char* heapHealth) {
    
    char conStatus[4]; // connected status
    (conStat) ? strcpy(conStatus, "yes") : strcpy(conStatus, "no");
    OLED.printSTA(&details, conStatus, updatingStatus, heapHealth);
    }

void getHeapHealth(char* heapHealth) {
    size_t freeHeap = (ESP.getFreeHeap() * 100 ) / startupHeap; // percentage
    sprintf(heapHealth, "%d%%", freeHeap);
}

void handleWifiMode(
    Comms::Net &Network, 
    UI::Display &OLED,
    UpdateSystem::OTAupdates &otaUpdates,
    FlashWrite::Credentials &Creds,
    const char* serverPassDefault,
    const char* serverName,
    const char* ipaddr
    ) {
    char WAPtype[20];
    char heapHealth[10];

    // compares to the current set WAP password, if it == the default, then 
    // the OLED will display the (DEF) follwoing the WAP type. This will disappear
    // when the AP_Pass is reset. It allows dynamic chaning.
    bool isWapDef = (strcmp(Network.getWAPpass(), serverPassDefault) == 0);
    uint8_t wifiMode = Comms::wifiModeSwitch(); // checks toggle position
    bool updatingStatus = otaUpdates.isUpdating(); 
    bool conStat = false; // connected status to the WAP or STA

    setWAPtype(WAPtype, wifiMode, isWapDef);
    getHeapHealth(heapHealth);

    switch(wifiMode) {
        case WAP_ONLY:
        conStat = Network.WAP(OLED, Creds);
        displayWAPstatus(OLED, serverName, ipaddr, conStat, WAPtype, updatingStatus, heapHealth); break;

        case WAP_SETUP:
        conStat = Network.WAPSetup(OLED, Creds);
        displayWAPstatus(OLED, serverName, ipaddr, conStat, WAPtype, updatingStatus, heapHealth); break;

        case STA_ONLY:
        conStat = Network.STA(OLED, Creds);
        Comms::STAdetails details = Network.getSTADetails();
        displaySTAstatus(OLED, conStat, details, updatingStatus, heapHealth);
    }
}





}