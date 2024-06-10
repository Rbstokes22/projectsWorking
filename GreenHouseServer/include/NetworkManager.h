// #ifndef NETWORKMANAGER_H
// #define NETWORKMANAGER_H

// #include <Arduino.h>
// #include "Display.h"
// #include "Network.h"
// #include "Creds.h"
// #include "OTAupdates.h"

// namespace networkManager {
//     extern size_t startupHeap;


//     uint8_t wifiModeSwitch(); // Checks the 3-way switch positon for the correct mode
//     void netConstructor(
//         bool isDefault, 
//         Net* Network, 
//         Credentials &Creds, 
//         Display &OLED, 
//         const char* serverName,
//         const char* serverPassDefault,
//         const char* errorMsg, 
//         bool sendMsg = true
//         );
//     void initializeNet(
//         bool defaultSwitch, 
//         Credentials &Creds, 
//         Net* Network, 
//         Display &OLED,
//         const char* serverName,
//         const char* serverPassDefault
//         );
//     void setWAPtype(char* WAPtype, uint8_t wifiMode, bool isWapDef);
//     void displayWAPstatus(
//         Display &OLED, 
//         bool conStat, 
//         const char* WAPtype, 
//         bool updatingStatus, 
//         const char* heapHealth,
//         const char* serverName,
//         const char* ipaddr
//         );
//     void displaySTAstatus(
//         Display &OLED, 
//         bool conStat, 
//         STAdetails details, 
//         bool updatingStatus, 
//         const char* heapHealth
//         );
//     void getHeapHealth(char* heapHealth);
//     void handleWifiMode(
//         Net* Network, 
//         Display &OLED,
//         Credentials &Creds,
//         OTAupdates &otaUpdates,
//         const char* serverPassDefault,
//         const char* serverName,
//         const char* ipaddr
//         );
// }

// #endif // NETWORKMANAGER_H