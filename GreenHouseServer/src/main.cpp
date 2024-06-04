// IDEAS: Twilio updates, changes, PWA to purchase as well as offer converters 
// for controlling code. 

#define VERSION 1.0
#define MODEL "GH-01"

#include <Arduino.h>
#include "Display.h"
#include "Timing.h"
#include "Network.h"
#include "OTAupdates.h"
#include "Creds.h"


// If changes to the servername are ever made, update the display printWAP 
// const char SSID to match
char SERVER_NAME[20] = "PJ Greenhouse"; 
char SERVER_PASS_DEFAULT[10] = "12345678";
#define IPADDR "192.168.1.1" // Will also set below in IPAddress

Display OLED(128, 64); // creates OLED class
Net* Network; // dynamically allocates in order to support password issues
Credentials EEPROMcreds(275);
Timer checkWifiModeSwitch(1000); // 3 second intervals to check network switch
Timer checkSensors(10000); // Check every 60 seconds (NEED TO BUILD)
Timer clearError(0); // Used to clear OLED screen after errors
OTAupdates otaUpdates(OLED);

size_t startupHeap = 0;

// Used to setup on WAP mode's password upon start.
void netConstructor(bool isDefault, const char* errorMsg, bool sendMsg = true) {
  char eepromError[50];
  switch (isDefault) {
    case true:
    Network = new Net(
    SERVER_NAME, SERVER_PASS_DEFAULT, // WAP SSID & Pass
    IPAddress(192, 168, 1, 1), // WAP localIP
    IPAddress(192, 168, 1, 1), // WAP gateway
    IPAddress(255, 255, 255, 0), // WAP subnet
    80);
    if (sendMsg == true) {
      strcpy(eepromError, errorMsg);
      OLED.displayError(eepromError);
    }
    break;

    case false: // being called
    Network = new Net(
    SERVER_NAME, EEPROMcreds.getWAPpass(), // WAP SSID & Pass
    IPAddress(192, 168, 1, 1), // WAP localIP
    IPAddress(192, 168, 1, 1), // WAP gateway
    IPAddress(255, 255, 255, 0), // WAP subnet
    80); 
  }
}

void setWAPtype(char* WAPtype, uint8_t wifiMode, bool isWapDef) {
  const char* mode = (wifiMode == WAP_ONLY) ? "WAP" : "WAP_SETUP";
  const char* suffix = (isWapDef) ? " (DEF)" : "";
  sprintf(WAPtype, "%s%s", mode, suffix);
}

void displayWAPstatus(Display &OLED, bool conStat, const char* WAPtype, bool updatingStatus, const char* heapHealth) {
  char conStatus[4];
  (conStat) ? strcpy(conStatus, "yes") : strcpy(conStatus, "no");
  OLED.printWAP(SERVER_NAME, IPADDR, conStatus, WAPtype, updatingStatus, heapHealth);
}

void displaySTAstatus(Display &OLED, bool conStat, STAdetails details, bool updatingStatus, const char* heapHealth) {
  char conStatus[4];
  (conStat) ? strcpy(conStatus, "yes") : strcpy(conStatus, "no");
  OLED.printSTA(&details, conStatus, updatingStatus, heapHealth);
}

void getHeapHealth(char* heapHealth) {
  size_t freeHeap = (ESP.getFreeHeap() * 100 ) / startupHeap; // percentage
  sprintf(heapHealth, "%d%%", freeHeap);
}

void handleWifiMode(Net* Network, Display &OLED) {
  char WAPtype[20];
  char heapHealth[10];
  bool isWapDef = (strcmp(Network->getWAPpass(), SERVER_PASS_DEFAULT) == 0);
  uint8_t wifiMode = wifiModeSwitch(); // checks toggle position
  bool updatingStatus = otaUpdates.isUpdating(); 
  bool conStat = false;

  setWAPtype(WAPtype, wifiMode, isWapDef);
  getHeapHealth(heapHealth);

  switch(wifiMode) {
    case WAP_ONLY:
    conStat = Network->WAP(OLED, EEPROMcreds);
    displayWAPstatus(OLED, conStat, WAPtype, updatingStatus, heapHealth); break;

    case WAP_SETUP:
    conStat = Network->WAPSetup(OLED, EEPROMcreds);
    displayWAPstatus(OLED, conStat, WAPtype, updatingStatus, heapHealth); break;

    case STA_ONLY:
    conStat = Network->STA(OLED, EEPROMcreds);
    STAdetails details = Network->getSTADetails();
    displaySTAstatus(OLED, conStat, details, updatingStatus, heapHealth);
  }
}

void setup() {
  Serial.begin(115200); // delete when finished
  startupHeap = ESP.getFreeHeap(); 

  pinMode(WAPswitch, INPUT_PULLUP);  // Wireless Access Point
  pinMode(STAswitch, INPUT_PULLUP); // Station
  pinMode(defaultWAPSwitch, INPUT_PULLUP); // default password override

  OLED.init(); // starts the OLED and displays 

// This will initialize or ensure that the eeprom has been initialized.
  uint8_t eepromSetup = EEPROMcreds.initialSetup(250, 2, 252, 22, 200); 


  // Clean this up TOMORROW
  switch(digitalRead(defaultWAPSwitch)) {
    case 0:
    Serial.println("DEFAULT WAP STARTED");
    netConstructor(true, ""); break;

    case 1:
    switch (eepromSetup) {
      case EEPROM_UP:
      EEPROMcreds.eepromRead(WAP_PASS_EEPROM);
      if (EEPROMcreds.getWAPpass()[0] != '\0') {
        Serial.println("EEPROM UP AND CONTAINS DATA");
        netConstructor(false, ""); 

      } else {
        Serial.println("EEPROM UP NO DATA");
        netConstructor(true, "Default password, setup new password");
      }
      break;

      case EEPROM_INITIALIZED:
      Serial.println("EEPROM INITIALIZED"); 
      netConstructor(true, "Ready to setup WAP Password", true);
      delay(3000); ESP.restart();
      break;

      case EEPROM_INIT_FAILED:
      Serial.println("EEPROM INIT FAILED");
      netConstructor(true, "Issue with memory initialization", true);
    }
  }
}

void loop() { 

  otaUpdates.manageOTA(Network);
  
  switch(checkWifiModeSwitch.isReady()) {
    case true:
    handleWifiMode(Network, OLED);
  }

  // This serves to clear errors displayed on the OLED. Typically the OLED
  // displays the network data and free memory. In the event of an error, that 
  // will be blocked, and if the override status == true, this will set a reminder
  // to clear it in n amount of milli seconds to allow normal funtioning again.
  if (OLED.getOverrideStat() == true) {
    if(clearError.setReminder(5000) == true) {
      OLED.setOverrideStat(false);
    } 
  }

  Network->handleServer();
}



