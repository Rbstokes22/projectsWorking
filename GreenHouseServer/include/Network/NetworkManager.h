#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <Arduino.h>
#include "Network/Creds.h"
#include "Network/NetworkWAP.h"
#include "Network/NetworkSTA.h"
#include "UI/MsgLogHandler.h"
#include "Common/OTAupdates.h"
#include "UI/Display.h"

namespace networkManager {

extern size_t startupHeap;

void initializeWAP(
    bool defaultSwitch, NVS::Credentials &Creds, 
    Comms::WirelessAP &wirelessAP, Comms::Station &station,
    Messaging::MsgLogHandler &msglogerr);

// Alerts the display if the WAP password is normal or default.
void setWAPtype(char* WAPtype, Comms::WIFI wifiMode, bool isWAPdefault);

void displayWAPstatus(
    UI::Display &OLED, const char* serverName,
    const char* ipaddr, bool conStat, const char* WAPtype, 
    const char* heapHealth);

void displaySTAstatus(
    UI::Display &OLED, bool conStat, 
    Comms::STAdetails &details, const char* heapHealth);

void getHeapHealth(char* heapHealth);

void handleWifiMode(
    Comms::WirelessAP &wirelessAP, Comms::Station &station,
    UI::Display &OLED, UpdateSystem::OTAupdates &otaUpdates,
    NVS::Credentials &Creds, const char* serverPassDefault,
    const char* serverName, const char* ipaddr);
}

#endif // NETWORKMANAGER_H