// IDEAS: Twilio updates, changes, PWA to purchase as well as offer converters 
// for controlling code. 

#define VERSION 1.0
#define MODEL "GH-01"

#include <Arduino.h>
#include "Display.h"
#include "Timing.h"
#include "Network.h"
#include "OTAupdates.h"
#include "eeprom.h"

// If changes to the servername are ever made, update the display printWAP 
// const char SSID to match
char SERVER_NAME[20] = "PJ Greenhouse"; 
char SERVER_PASS_DEFAULT[10] = "12345678";
#define IPADDR "192.168.1.1" // Will also set below in IPAddress

Display OLED(128, 64); // creates OLED class
Net* Network; // dynamically allocates in order to support password issues
STAsettings STAeeprom;

Timer checkWifiModeSwitch(1000); // 3 second intervals to check network switch
Timer checkSensors(10000); // Check every 60 seconds (NEED TO BUILD)
Timer clearError(0);

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
    Network->setIsDefaultWAPpass(true);
    if (sendMsg == true) {
      strcpy(eepromError, errorMsg);
      OLED.displayError(eepromError);
    }

    case false:
    Network = new Net(
      SERVER_NAME, STAeeprom.getWAPpass(), // WAP SSID & Pass
      IPAddress(192, 168, 1, 1), // WAP localIP
      IPAddress(192, 168, 1, 1), // WAP gateway
      IPAddress(255, 255, 255, 0), // WAP subnet
      80); 
      Network->setIsDefaultWAPpass(false);
  }
}

void setup() {
  Serial.begin(115200); // delete when finished
  startupHeap = ESP.getFreeHeap(); 

  pinMode(WAPswitch, INPUT_PULLUP);  // Wireless Access Point
  pinMode(STAswitch, INPUT_PULLUP); // Station
  pinMode(defaultWAP, INPUT_PULLUP); // default password override

  OLED.init(); // starts the OLED and displays 

// This will initialize or ensure that the eeprom has been initialized.
  uint8_t eepromSetup = STAeeprom.initialSetup(261, 2, 263, 22, 200); 
  
  // Startup switch determines this mode. It is designed to read from the EEPROM.
  // Upon a first start, the EEPROM will initialize and reset nullifying all 
  // data block entries. It will then restart looking for a password for the WAP.
  // If it is a '\0', it will start with the default password, and this is fine 
  // if the client desires, or they can change their password. In the event they
  // every forget their saved password, and want to restart, there is a switch 
  // that they can hold upon startup. This will automatically start with a default
  // password. Other than that, once the user saves a password successfully in the
  // EEPROM, the WAP will begin with that password upon every read. If that data 
  // is somehow corrupt, troubleshooting will be needed. Its doubtful with the 
  // 100k - 1M write cycles an EEPROM can take, but there is always some probability
  // for failure. In the event it doesnt start, the client can always use the default
  // until fixed or replaced.
  switch(digitalRead(defaultWAP)) {
    case 0:
    Serial.println("DEFAULT WAP STARTED");
    netConstructor(true, ""); break;

    case 1:
    switch (eepromSetup) {
      case EEPROM_UP:
      STAeeprom.eepromRead(WAP_PASS_EEPROM);
      if (STAeeprom.getWAPpass()[0] != '\0') {
        Serial.println("EEPROM UP AND CONTAINS DATA");
        netConstructor(false, ""); break;

      } else {
        Serial.println("EEPROM UP NO DATA");
        netConstructor(true, "Default password, setup new password");
        break;
      }

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

  // Safety that only allows the ota updates to begin when connected
  // to the station. Once started it doesn't stop until the end of the
  // program, but will not handle any updates unless station connected.
  if (Network->isSTAconnected() && !otaUpdates.getHasStarted()) {
    otaUpdates.start();
    otaUpdates.setHasStarted(true); 
  } else if(Network->isSTAconnected() && otaUpdates.getHasStarted()) {
    otaUpdates.handle(); // checks for firmware 
  }

  switch(checkWifiModeSwitch.isReady()) {
    case true:

    // If the default WAP password button is held, or dropped to ground upon 
    // initialization, this will display to the OLED that it is in whichever
    // WAP mode, however the default password applies. This is for those 
    // who need to do a reset because they forgot their password for their 
    // WAP.
    char WAPtype[20];
    bool isWapDef = Network->getIsDefaultWAPpass(); // default WAP password
    uint8_t wifiMode = wifiModeSwitch();

    if (wifiMode == WAP_ONLY) {
      if (isWapDef == true) {
        strcpy(WAPtype, "WAP (DEF)");
      } else {
        strcpy(WAPtype, "WAP");
      }
    } else if (wifiMode == WAP_SETUP) {
      if (isWapDef == true) {
        strcpy(WAPtype, "WAP SETUP (DEF)");
      } else {
        strcpy(WAPtype, "WAP SETUP");
      }
    }

    // overrides typical wifi OLED display when updating OTA
    bool updatingStatus = otaUpdates.isUpdating(); 

    // Handles heap free memory
    size_t freeHeap = (ESP.getFreeHeap() * 100 ) / startupHeap; // percentage
    char heapHealth[10];
    sprintf(heapHealth, "%d%%", freeHeap);

    switch(wifiMode) { // 0 = WAP, 1 = WAP setup, 2 = STA
      case WAP_ONLY:
      switch(Network->WAP(OLED, STAeeprom)) {
        case true:
        OLED.printWAP(SERVER_NAME, IPADDR, "Yes", WAPtype, updatingStatus, heapHealth);
        break;

        case false:
        OLED.printWAP(SERVER_NAME, IPADDR, "No", WAPtype, updatingStatus, heapHealth);
      }
      break;

      case WAP_SETUP:
      switch(Network->WAPSetup(OLED, STAeeprom)) {
        case true:
        OLED.printWAP(SERVER_NAME, IPADDR, "Yes", WAPtype, updatingStatus, heapHealth);
        break;

        case false:
        OLED.printWAP(SERVER_NAME, IPADDR, "No", WAPtype, updatingStatus, heapHealth);
      }
      break;

      case STA_ONLY:
      STAdetails details = Network->getSTADetails();

      switch(Network->STA(OLED, STAeeprom)) {
        case WIFI_STARTING:
        OLED.printSTA(&details, "No", updatingStatus, heapHealth); 
        break;

        case WIFI_RUNNING:
        OLED.printSTA(&details, "Yes", updatingStatus, heapHealth);
      }
    }
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



