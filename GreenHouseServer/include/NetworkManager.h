#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <Arduino.h>
#include "Creds.h"
#include "Network.h"
#include "Display.h"
#include "OTAupdates.h"

namespace networkManager {

extern size_t startupHeap;

void netConstructor(
    bool isDefault, 
    Comms::Net &Network,
    UI::Display &OLED,
    FlashWrite::Credentials &Creds,
    const char* errorMsg, 
    bool sendMsg = true
    );

void initializeNet(
    bool defaultSwitch,
    FlashWrite::Credentials &Creds, 
    Comms::Net &Network, 
    UI::Display &OLED
    );

void setWAPtype(char* WAPtype, uint8_t wifiMode, bool isWapDef);

void displayWAPstatus(
    UI::Display &OLED, 
    const char* serverName,
    const char* ipaddr,
    bool conStat, 
    const char* WAPtype, 
    bool updatingStatus, 
    const char* heapHealth
);

void displaySTAstatus(
    UI::Display &OLED, 
    bool conStat, 
    Comms::STAdetails details, 
    bool updatingStatus, 
    const char* heapHealth
    );

void getHeapHealth(char* heapHealth);

void handleWifiMode(
    Comms::Net &Network, 
    UI::Display &OLED,
    UpdateSystem::OTAupdates &otaUpdates,
    FlashWrite::Credentials &Creds,
    const char* serverPassDefault,
    const char* serverName,
    const char* ipaddr
    );


}

#endif // NETWORKMANAGER_H