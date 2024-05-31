// IDEAS: Twilio updates, changes, PWA to purchase as well as offer converters 
// for controlling code. OAP updates for firmware.

#define VERSION 1.0
#define MODEL GH1

#include <Arduino.h>
#include "Display.h"
#include "Timing.h"
#include "Network.h"
#include "OTAupdates.h"
#include "arduinoCom.h"

// If changes to the servername are ever made, update the display printWAP 
// const char SSID to match
#define SERVER_NAME "GreenHouse" 
#define SERVER_PASS "12345678"
#define IPADDR "192.168.1.1" // Will also set below in IPAddress

Display OLED(128, 64); // creates OLED class

// WPA2-PSK passwords require an 8 char minimum, this must happen for this
// to broadcast the appropriate SSID.
Net Network(
  SERVER_NAME, SERVER_PASS, // WAP SSID & Pass
  IPAddress(192, 168, 1, 1), // WAP localIP
  IPAddress(192, 168, 1, 1), // WAP gateway
  IPAddress(255, 255, 255, 0), // WAP subnet
  80
  ); // WAP SSID and Pass

Timer checkWifiModeSwitch(1000); // 3 second intervals to check network switch
Timer checkSensors(10000); // Check every 60 seconds (NEED TO BUILD)
Timer clearError(0);

OTAupdates otaUpdates(OLED);
I2Ccomm i2c(0x16); // 22, used to communicate with the arduino

size_t startupHeap = 0;

void setup() {
  Serial.begin(115200); // delete when finished
  startupHeap = ESP.getFreeHeap(); 
  Network.loadCertificates();

  pinMode(WAPswitch, INPUT_PULLUP);  // Wireless Access Point
  pinMode(STAswitch, INPUT_PULLUP); // Station

  OLED.init(); // starts the OLED and displays 
  i2c.begin();
}

void loop() { 

  // Safety that only allows the ota updates to begin when connected
  // to the station. Once started it doesn't stop until the end of the
  // program, but will not handle any updates unless station connected.
  if (Network.isSTAconnected() && !otaUpdates.getHasStarted()) {
    otaUpdates.start();
    otaUpdates.setHasStarted(true); 
  } else if(Network.isSTAconnected() && otaUpdates.getHasStarted()) {
    otaUpdates.handle(); // checks for firmware 
  }

  switch(checkWifiModeSwitch.isReady()) {
    case true:
    bool updatingStatus = otaUpdates.isUpdating(); // override display if updating
    size_t freeHeap = (ESP.getFreeHeap() * 100 ) / startupHeap; // percentage
    char heapHealth[10];
    sprintf(heapHealth, "%d%%", freeHeap);

    switch(wifiModeSwitch()) { // 0 = WAP, 1 = WAP setup, 2 = STA
      case WAP_ONLY:
      switch(Network.WAP(OLED)) {
        case true:
        OLED.printWAP(SERVER_NAME, IPADDR, "Yes", 0, updatingStatus, heapHealth);
        break;

        case false:
        OLED.printWAP(SERVER_NAME, IPADDR, "No", 0, updatingStatus, heapHealth);
      }
      break;

      case WAP_SETUP:
      switch(Network.WAPSetup(OLED)) {
        case true:
        OLED.printWAP(SERVER_NAME, IPADDR, "Yes", 1, updatingStatus, heapHealth);
        break;

        case false:
        OLED.printWAP(SERVER_NAME, IPADDR, "No", 1, updatingStatus, heapHealth);
      }
      break;

      case STA_ONLY:
      STAdetails details = Network.getSTADetails();

      switch(Network.STA(OLED)) {
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

  Network.handleServer();
}



