// IDEAS: Twilio updates, changes, PWA to purchase as well as offer converters 
// for controlling code. 

#define VERSION 1.0
#define MODEL GH1

#include <Arduino.h>
#include "Display.h"
#include "Timing.h"
#include "Network.h"
#include "OTAupdates.h"

// If changes to the servername are ever made, update the display printWAP 
// const char SSID to match
char SERVER_NAME[20] = "PJ Greenhouse"; 
char SERVER_PASS_DEFAULT[10] = "12345678";
#define IPADDR "192.168.1.1" // Will also set below in IPAddress

Display OLED(128, 64); // creates OLED class
Net* Network; // dynamically allocates in order to support password issues

Timer checkWifiModeSwitch(1000); // 3 second intervals to check network switch
Timer checkSensors(10000); // Check every 60 seconds (NEED TO BUILD)
Timer clearError(0);

OTAupdates otaUpdates(OLED);

size_t startupHeap = 0;

void setup() {
  Serial.begin(115200); // delete when finished
  startupHeap = ESP.getFreeHeap(); 

  pinMode(WAPswitch, INPUT_PULLUP);  // Wireless Access Point
  pinMode(STAswitch, INPUT_PULLUP); // Station
  pinMode(defaultWAP, INPUT_PULLUP); // default password override

  OLED.init(); // starts the OLED and displays 

  switch(digitalRead(defaultWAP)) {
    case 0:
    Network = new Net(
    SERVER_NAME, SERVER_PASS_DEFAULT, // WAP SSID & Pass
    IPAddress(192, 168, 1, 1), // WAP localIP
    IPAddress(192, 168, 1, 1), // WAP gateway
    IPAddress(255, 255, 255, 0), // WAP subnet
    80
    ); Network->setIsDefaultWAPpass(true); break;

    case 1:
    Network = new Net(
    SERVER_NAME, "11111111", // WAP SSID & Pass
    IPAddress(192, 168, 1, 1), // WAP localIP
    IPAddress(192, 168, 1, 1), // WAP gateway
    IPAddress(255, 255, 255, 0), // WAP subnet
    80
    ); Network->setIsDefaultWAPpass(false);
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
      switch(Network->WAP(OLED)) {
        case true:
        OLED.printWAP(SERVER_NAME, IPADDR, "Yes", WAPtype, updatingStatus, heapHealth);
        break;

        case false:
        OLED.printWAP(SERVER_NAME, IPADDR, "No", WAPtype, updatingStatus, heapHealth);
      }
      break;

      case WAP_SETUP:
      switch(Network->WAPSetup(OLED)) {
        case true:
        OLED.printWAP(SERVER_NAME, IPADDR, "Yes", WAPtype, updatingStatus, heapHealth);
        break;

        case false:
        OLED.printWAP(SERVER_NAME, IPADDR, "No", WAPtype, updatingStatus, heapHealth);
      }
      break;

      case STA_ONLY:
      STAdetails details = Network->getSTADetails();

      switch(Network->STA(OLED)) {
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



