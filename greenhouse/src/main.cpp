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
  443
  ); // WAP SSID and Pass

Timer checkNetSwitch(1000); // 3 second intervals to check network switch
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
  otaUpdates.start();
  i2c.begin();
  
  
  }

void loop() { 
  
  otaUpdates.handle(); // Checks if any firmware updates are coming through

  // This switch sets the network up depending on its value
  if (checkNetSwitch.isReady()) {
    uint8_t wifiMode = netSwitch(); // 0 = WAP, 1 = WAP setup, 2 = STA
    bool updatingStatus = otaUpdates.isUpdating(); // kills display if updating
    size_t freeHeap = (ESP.getFreeHeap() * 100 ) / startupHeap; // percentage
    char heapHealth[10];
    sprintf(heapHealth, "%d%%", freeHeap);

    if (wifiMode == 0) {
      if (Network.WAP(OLED)) { // Means that it is running
        OLED.printWAP(SERVER_NAME, IPADDR, "Yes", 0, updatingStatus, heapHealth);
      } else { // means that it is connecting
        OLED.printWAP(SERVER_NAME, IPADDR, "No", 0, updatingStatus, heapHealth);
      }
    } else if (wifiMode == 1) {
      if (Network.WAPSetup(OLED)) {
        OLED.printWAP(SERVER_NAME, IPADDR, "Yes", 1, updatingStatus, heapHealth);
      } else {
        OLED.printWAP(SERVER_NAME, IPADDR, "No", 1, updatingStatus, heapHealth);
      }
    } else if (wifiMode == 2) {
        uint8_t action = Network.STA(OLED);
        STAdetails details = Network.getSTADetails();

        switch(action) {
          case WIFI_STARTING:
          OLED.printSTA(&details, "No", updatingStatus, heapHealth); break;
          case WIFI_RUNNING:
          OLED.printSTA(&details, "Yes", updatingStatus, heapHealth); 
        }
    }
  }

  if (OLED.getOverrideStat() == true) {
    if(clearError.setReminder(5000) == true) {
      OLED.setOverrideStat(false);
    } 
  }

  Network.handleServer();

  
}


