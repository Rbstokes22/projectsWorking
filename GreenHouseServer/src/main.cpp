// IDEAS: Twilio updates, changes, PWA to purchase as well as offer converters 
// for controlling code. Include a warnings class, like the relays, you have 
// predefined conditions, ie, Temp < 35F, send warning 1x, will not send anymore 
// temp warnings. (Work a natural reset for that)

#define FIRMWARE_VERSION 1.0
#define MODEL "GH-01"

#include <Arduino.h>
#include <Wire.h>
#include "Display.h"
#include "Timing.h"
#include "Network/Network.h"
#include "Network/NetworkManager.h"
#include "OTAupdates.h"
#include "Creds.h"
#include "Threads.h"
#include "Peripherals.h"

// If changes to the servername are ever made, update the display printWAP 
// const char SSID to match
char SERVER_NAME[20]{"PJ Greenhouse"}; 
char SERVER_PASS_DEFAULT[10]{"12345678"};
#define IPADDR "192.168.1.1" // Will also set below in IPAddress

// ALL OBJECTS
UI::Display OLED{128, 64};

// Created with default password, will set password during init if 
// one is saved in the NVS, or if one is passed in WAPSetup mode.
Comms::WirelessAP wirelessAP = {
    SERVER_NAME, SERVER_PASS_DEFAULT, // WAP SSID & Pass
    IPAddress(192, 168, 1, 1), // WAP localIP
    IPAddress(192, 168, 1, 1), // WAP gateway
    IPAddress(255, 255, 255, 0) // WAP subnet
};

Comms::Station station;

FlashWrite::Credentials Creds{"Network", OLED};

// Timer objects pass in the interval of reset
Clock::Timer checkWifiModeSwitch{1000}; 
Clock::Timer checkTempHum{10000};
Clock::Timer checkLight{5000};
// Clock::Timer checkSensors{10000}; 
Clock::Timer clearError{0}; // Used to clear OLED screen after errors

// Devices and threads
Devices::TempHum tempHum{DHT_PIN, DHT22, 3};
Devices::Light light{Photoresistor_PIN};  

// Thread setups to keep sensors independent from eachother and main
// ADD MUTEXS
Threads::ThreadData THDATAtempHum{tempHum, checkTempHum};
Threads::SensorThread THtempHum;

// Send all threads to OTA updates to suspend and resume threads during updates
UpdateSystem::OTAupdates otaUpdates{
  OLED, THtempHum
  };


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
  light.begin(); // must call after Wire.begin();

  Serial.begin(115200); // delete when finished

  OLED.init(); // starts the OLED and displays 

  // Initialize and start all threads
  THtempHum.initThread(THDATAtempHum);

  networkManager::initializeWAP(
    digitalRead(defaultWAPSwitch), Creds, 
    wirelessAP, station, OLED
    );
}

void loop() { 
  
  otaUpdates.manageOTA(station);

  switch(checkWifiModeSwitch.isReady()) {
    case true:;

    networkManager::handleWifiMode(
      wirelessAP, station,
      OLED, otaUpdates, Creds,
      SERVER_PASS_DEFAULT, SERVER_NAME, IPADDR
      );
  }

  // Sets timer to clear OLED error after 5 second display.
  if (OLED.getOverrideStat() == true) {
    if(clearError.setReminder(5000) == true) {
      OLED.setOverrideStat(false);
    } 
  }

  station.handleServer();
}



