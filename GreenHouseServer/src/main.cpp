// IDEAS: Twilio updates, changes, PWA to purchase as well as offer converters 
// for controlling code. 

#define FIRMWARE_VERSION 1.0
#define MODEL "GH-01"

#include <Arduino.h>
#include <Wire.h>
#include "Display.h"
#include "Timing.h"
#include "Network.h"
#include "NetworkManager.h"
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
Comms::Net Network(
    SERVER_NAME, SERVER_PASS_DEFAULT, // WAP SSID & Pass
    IPAddress(192, 168, 1, 1), // WAP localIP
    IPAddress(192, 168, 1, 1), // WAP gateway
    IPAddress(255, 255, 255, 0), // WAP subnet
    80
    );

FlashWrite::Credentials Creds("Network", OLED);

// Timer objects pass in the interval of reset
Clock::Timer checkWifiModeSwitch(1000); 
Clock::Timer checkSensors(1000); 
Clock::Timer clearError(0); // Used to clear OLED screen after errors
Threads::SensorThread sensorThread(checkSensors);
UpdateSystem::OTAupdates otaUpdates(OLED, sensorThread);

size_t networkManager::startupHeap = 0;

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

  networkManager::initializeNet(
    digitalRead(defaultWAPSwitch), 
    Creds, 
    Network, 
    OLED);
}

void loop() { 
  
  otaUpdates.manageOTA(&Network);

  switch(checkWifiModeSwitch.isReady()) {
    case true:;

    networkManager::handleWifiMode(
      Network, 
      OLED,
      otaUpdates,
      Creds,
      SERVER_PASS_DEFAULT,
      SERVER_NAME,
      IPADDR
      );
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

  Network.handleServer();
}



