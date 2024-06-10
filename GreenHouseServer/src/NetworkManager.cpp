// #include "NetworkManager.h"

// // Checks 3-way switch position and starts the appropriate network
// uint8_t networkManager::wifiModeSwitch() {
//     Serial.println("CHECKED WIFI MODE SWITCH");
//     // 0 = WAP exclusive, 1 = WAP Program, 2 = Station
//     uint8_t WAP = digitalRead(WAPswitch);
//     uint8_t STA = digitalRead(STAswitch);  

//     if (!WAP && STA) { // WAP Exclusive
//         return WAP_ONLY;
//     } else if (WAP && STA) { // Middle position, WAP Program mode for STA
//         return WAP_SETUP;
//     } else if (!STA && WAP) { // Station mode reading from EEPROM
//         return STA_ONLY;
//     } else {
//         return NO_WIFI; // standard error throughout this program
//     }
// }

// void networkManager::netConstructor(
//     bool isDefault, 
//     Net* Network, 
//     Credentials &Creds, 
//     Display &OLED,
//     const char* serverName,
//     const char* serverPassDefault,
//     const char* errorMsg, 
//     bool sendMsg) {

//   char error[50];
//   switch (isDefault) {
//     case true:
//     Serial.println("Starting default");
//     Network = new Net(
//     serverName, serverPassDefault, // WAP SSID & Pass
//     IPAddress(192, 168, 1, 1), // WAP localIP
//     IPAddress(192, 168, 1, 1), // WAP gateway
//     IPAddress(255, 255, 255, 0), // WAP subnet
//     80);
//     if (sendMsg == true) {
//       strcpy(error, errorMsg);
//       OLED.displayError(error);
//     }
//     break;

//     case false: // being called
//     Network = new Net(
//     serverName, Creds.getWAPpass(), // WAP SSID & Pass
//     IPAddress(192, 168, 1, 1), // WAP localIP
//     IPAddress(192, 168, 1, 1), // WAP gateway
//     IPAddress(255, 255, 255, 0), // WAP subnet
//     80); 
//   }
// }

// // Checks for button pressing during startup to go into default WAP password mode.
// // If not pushed, checks EEPROM for an actual WAP password, if exists, starts normally.
// // If it does not exist and the value is '\0', default WAP password will be in place 
// // until changed.
// void networkManager::initializeNet(
//     bool defaultSwitch, 
//     Credentials &Creds, 
//     Net* Network, 
//     Display &OLED,
//     const char* serverName,
//     const char* serverPassDefault
//     ) {
//         Serial.println("init Net");
//   switch (defaultSwitch) {
//     case false:
//     netConstructor(true, Network, Creds, OLED, serverName, serverPassDefault, "Default Mode"); break;

//     // INIT THE WAP PASS HERE Using CREDS
//     case true:
//     Creds.read("WAPpass"); // sets the WAPpass array
//     if (Creds.getWAPpass()[0] != '\0') {
//       netConstructor(false, Network, Creds, OLED, serverName, serverPassDefault, "");
//     } else {
//       netConstructor(true, Network, Creds, OLED, serverName, serverPassDefault, "Set WAP Pass");
//     }
//   }
// }

// void networkManager::setWAPtype(char* WAPtype, uint8_t wifiMode, bool isWapDef) {
//     Serial.println("setWAPtype");
//   const char* mode = (wifiMode == WAP_ONLY) ? "WAP" : "WAP SETUP";
//   const char* suffix = (isWapDef) ? " (DEF)" : "";
//   sprintf(WAPtype, "%s%s", mode, suffix);
// }

// void networkManager::displayWAPstatus(
//     Display &OLED, 
//     bool conStat, 
//     const char* WAPtype, 
//     bool updatingStatus, 
//     const char* heapHealth,
//     const char* serverName,
//     const char* ipaddr
//     ) {
//         Serial.println("disp wap stat");
//   char conStatus[4]; // connected status
//   (conStat) ? strcpy(conStatus, "yes") : strcpy(conStatus, "no");
//   OLED.printWAP(serverName, ipaddr, conStatus, WAPtype, updatingStatus, heapHealth);
// }

// void networkManager::displaySTAstatus(
//     Display &OLED, 
//     bool conStat, 
//     STAdetails details, 
//     bool updatingStatus, 
//     const char* heapHealth) {Serial.println("disp sta status");
//   char conStatus[4]; // connected status
//   (conStat) ? strcpy(conStatus, "yes") : strcpy(conStatus, "no");
//   OLED.printSTA(&details, conStatus, updatingStatus, heapHealth);
// }

// void networkManager::getHeapHealth(char* heapHealth) {
//     Serial.println("got heap health");
//   size_t freeHeap = (ESP.getFreeHeap() * 100 ) / startupHeap; // percentage
//   sprintf(heapHealth, "%d%%", freeHeap);
// }

// void networkManager::handleWifiMode(
//     Net* Network, 
//     Display &OLED,
//     Credentials &Creds,
//     OTAupdates &otaUpdates,
//     const char* serverPassDefault,
//     const char* serverName,
//     const char* ipaddr
//     ) {
//         Serial.println("handled wifi mode");
//   char WAPtype[20];
//   char heapHealth[10];

//   // compares to the current set WAP password, if it == the default, then 
//   // the OLED will display the (DEF) follwoing the WAP type. This will disappear
//   // when the AP_Pass is reset. It allows dynamic chaning.
//   bool isWapDef = (strcmp(Network->getWAPpass(), serverPassDefault) == 0);
//   uint8_t wifiMode = networkManager::wifiModeSwitch(); // checks toggle position
//   bool updatingStatus = otaUpdates.isUpdating(); 
//   bool conStat = false; // connected status to the WAP or STA

//   setWAPtype(WAPtype, wifiMode, isWapDef);
//   getHeapHealth(heapHealth);

//   switch(wifiMode) {
//     case WAP_ONLY:
//     conStat = Network->WAP(OLED, Creds);
//     displayWAPstatus(OLED, conStat, WAPtype, updatingStatus, heapHealth, serverName, ipaddr); break;

//     case WAP_SETUP:
//     conStat = Network->WAPSetup(OLED, Creds);
//     displayWAPstatus(OLED, conStat, WAPtype, updatingStatus, heapHealth, serverName, ipaddr); break;

//     case STA_ONLY:
//     conStat = Network->STA(OLED, Creds);
//     STAdetails details = Network->getSTADetails();
//     displaySTAstatus(OLED, conStat, details, updatingStatus, heapHealth);
//   }
// }