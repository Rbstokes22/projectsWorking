// IDEAS: Twilio updates, changes, PWA to purchase as well as offer converters 
// for controlling code. 

#define FIRMWARE_VERSION 1.0
#define MODEL "GH-01"

#include <Arduino.h>
#include <Wire.h>
#include "Display.h"
#include "Timing.h"
#include "Network.h"
#include "OTAupdates.h"
#include "Creds.h"
#include "Peripherals.h"

// If changes to the servername are ever made, update the display printWAP 
// const char SSID to match
char SERVER_NAME[20] = "PJ Greenhouse"; 
char SERVER_PASS_DEFAULT[10] = "12345678";
#define IPADDR "192.168.1.1" // Will also set below in IPAddress

// ALL OBJECTS
UI::Display OLED(128, 64); 
Comms::Net* Network = nullptr; // dynamically allocates in order to support password issues
FlashWrite::Credentials Creds("Network", OLED);
Clock::Timer checkWifiModeSwitch(1000); // 3 second intervals to check network switch
Clock::Timer checkSensors(1000); // Check every 60 seconds
Clock::Timer clearError(0); // Used to clear OLED screen after errors
Threads::SensorThread sensorThread(checkSensors);
UpdateSystem::OTAupdates otaUpdates(OLED, sensorThread);

namespace networkManager { // Used to initialize and manage network
size_t startupHeap = 0;

// Used to setup on WAP mode's password upon start.
void netConstructor(bool isDefault, const char* errorMsg, bool sendMsg = true) {
  char eepromError[50];
  switch (isDefault) {
    case true:
    Network = new Comms::Net(
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
    Network = new Comms::Net(
    SERVER_NAME, Creds.getWAPpass(), // WAP SSID & Pass
    IPAddress(192, 168, 1, 1), // WAP localIP
    IPAddress(192, 168, 1, 1), // WAP gateway
    IPAddress(255, 255, 255, 0), // WAP subnet
    80); 
  }
}

// Checks for button pressing during startup to go into default WAP password mode.
// If not pushed, checks EEPROM for an actual WAP password, if exists, starts normally.
// If it does not exist and the value is '\0', default WAP password will be in place 
// until changed.
void initializeNet(bool defaultSwitch) {

  // To erase eeprom, change one of the expVals
  // uint8_t eepromSetup = Creds.initialSetup(2, 22);
  switch (defaultSwitch) {
    case false:
    netConstructor(true, "Default Mode"); break;

    // INIT THE WAP PASS HERE Using CREDS
    case true:
    Creds.read("WAPpass"); // sets the WAPpass array
    if (Creds.getWAPpass()[0] != '\0') {
      netConstructor(false, "");
    } else {
      netConstructor(true, "Set WAP Pass");
    }
  }
}

void setWAPtype(char* WAPtype, uint8_t wifiMode, bool isWapDef) {
  const char* mode = (wifiMode == WAP_ONLY) ? "WAP" : "WAP SETUP";
  const char* suffix = (isWapDef) ? " (DEF)" : "";
  sprintf(WAPtype, "%s%s", mode, suffix);
}

void displayWAPstatus(UI::Display &OLED, bool conStat, const char* WAPtype, bool updatingStatus, const char* heapHealth) {
  char conStatus[4]; // connected status
  (conStat) ? strcpy(conStatus, "yes") : strcpy(conStatus, "no");
  OLED.printWAP(SERVER_NAME, IPADDR, conStatus, WAPtype, updatingStatus, heapHealth);
}

void displaySTAstatus(UI::Display &OLED, bool conStat, Comms::STAdetails details, bool updatingStatus, const char* heapHealth) {
  char conStatus[4]; // connected status
  (conStat) ? strcpy(conStatus, "yes") : strcpy(conStatus, "no");
  OLED.printSTA(&details, conStatus, updatingStatus, heapHealth);
}

void getHeapHealth(char* heapHealth) {
  size_t freeHeap = (ESP.getFreeHeap() * 100 ) / startupHeap; // percentage
  sprintf(heapHealth, "%d%%", freeHeap);
}

void handleWifiMode(Comms::Net* Network, UI::Display &OLED) {
  char WAPtype[20];
  char heapHealth[10];

  // compares to the current set WAP password, if it == the default, then 
  // the OLED will display the (DEF) follwoing the WAP type. This will disappear
  // when the AP_Pass is reset. It allows dynamic chaning.
  bool isWapDef = (strcmp(Network->getWAPpass(), SERVER_PASS_DEFAULT) == 0);
  uint8_t wifiMode = Comms::wifiModeSwitch(); // checks toggle position
  bool updatingStatus = otaUpdates.isUpdating(); 
  bool conStat = false; // connected status to the WAP or STA

  setWAPtype(WAPtype, wifiMode, isWapDef);
  getHeapHealth(heapHealth);

  switch(wifiMode) {
    case WAP_ONLY:
    conStat = Network->WAP(OLED, Creds);
    displayWAPstatus(OLED, conStat, WAPtype, updatingStatus, heapHealth); break;

    case WAP_SETUP:
    conStat = Network->WAPSetup(OLED, Creds);
    displayWAPstatus(OLED, conStat, WAPtype, updatingStatus, heapHealth); break;

    case STA_ONLY:
    conStat = Network->STA(OLED, Creds);
    Comms::STAdetails details = Network->getSTADetails();
    displaySTAstatus(OLED, conStat, details, updatingStatus, heapHealth);
  }
}
}

void setup() {
  networkManager::startupHeap = ESP.getFreeHeap(); 
  pinMode(WAPswitch, INPUT_PULLUP);  // Wireless Access Point
  pinMode(STAswitch, INPUT_PULLUP); // Station
  pinMode(defaultWAPSwitch, INPUT_PULLUP); // default password override
  pinMode(Relay_1_PIN, OUTPUT);
  pinMode(Soil_1_PIN, INPUT);
  pinMode(Photoresistor_PIN, INPUT);

  Wire.begin(); // Setup the i2c bus. 
  Serial.begin(115200); // delete when finished

  sensorThread.setupThread();

  dht.begin(); // MAKE CLASS TO MAKE THIS EASIER//////////////////////
  as7341.begin();

  
  OLED.init(); // starts the OLED and displays 
  networkManager::initializeNet(digitalRead(defaultWAPSwitch));
}

void loop() { 
  
  otaUpdates.manageOTA(Network);

  switch(checkWifiModeSwitch.isReady()) {
    case true:
    networkManager::handleWifiMode(Network, OLED);
  }

  // This serves to clear errors displayed on the OLED. Typically the OLED
  // displays the network data and free memory. In the event of an error, that 
  // will be blocked, and if the override status == true, this will set a reminder
  // to clear it in n amount of milliseconds to allow normal funtioning again.
  if (OLED.getOverrideStat() == true) {
    if(clearError.setReminder(5000) == true) {
      OLED.setOverrideStat(false);
    } 
  }

  Network->handleServer();
}



