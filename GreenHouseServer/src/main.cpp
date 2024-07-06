// IDEAS: Twilio updates, changes, PWA to purchase as well as offer converters 
// for controlling code. Include a warnings class, like the relays, you have 
// predefined conditions, ie, Temp < 35F, send warning 1x, will not send anymore 
// temp warnings. (Work a natural reset for that). Replace serial prints with logging
// and consider applying error handling logging throughout the program. Also incorp
// watchdog timers in here for the logging before crashes and what not. 


// BOOKMARK.
// Finish checksum for creds, and then test using null ptr, and exceeding chars.
// look through program to handle pointer arguments,
// finish checksum for peripherals.

// 1. Refine code to make it more modular. add desriptive handlers and stuff.
// split up the Network.h into several files, maybe create a directories.

// MAKE RELAY OBJECTS AND GROUP TO BE ABLE TO PASS TO ACTIVATE AND DEACTIVATE
// RELAY BASED ON CERTAIN LOGIC. ALLOCATE PINS TO ALL RELAYS AND SOIL SENSORS

// 2. CREATE A WAY TO SET DEFAULT SETTINGS UNLESS WHEN INIT, READ FROM NVS TO APPLY
// SENSOR SETTINGS !!!!!!!!!!!!!!!!!

// CREATE AN ALERT SYSTEM, SORT OF LIKE THE RELAYS. THEY CAN ALERT BASED ON TEMP/HUM,
// AND SOIL. DONT NEED AN ALERT FOR LIGHT. MINIMIZE ALERTS SENT UNLESS CRITERIA IS
// RESET. THINK ABOUT FAULTY READINGS SUCH AS IF 20 READINGS MEET CRITERIA. ETC...

// HAVE CALIBRATIONS FOR SOIL ONLY. PHOTO RESISTOR DOESNT NEED CALIBRATION, ITS PURPOSE
// IS TO SAY WHETHER DARK OR NOT. THAT CAN BE SET BY THE CLIENT FOR WHAT THEY DEEM
// TO BE DARK.

#define FIRMWARE_VERSION 1.0
#define MODEL "GH-01"

#include <Arduino.h>
#include <Wire.h>
#include "UI/Display.h"
#include "UI/MsgLogHandler.h"
#include "Common/Timing.h"
#include "Network/NetworkWAP.h"
#include "Network/NetworkSTA.h"
#include "Network/NetworkManager.h"
#include "Common/OTAupdates.h"
#include "Network/Creds.h"
#include "Threads/Threads.h"
#include "Peripherals/TempHum.h"
#include "Peripherals/Light.h"
#include "Peripherals/Soil.h"
#include "NVS/NVS.h" // REMOVE TESTING ONLY !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

char SERVER_NAME[20]{"MumsyGH"}; 
char SERVER_PASS_DEFAULT[10]{"12345678"};

// Will also set below in IPAddress. Used for display only.
#define IPADDR "192.168.1.1" 

  // Main display, passes dimensions of screen
UI::Display OLED{128, 64};

// Handles all messaging, errors, and logging. Either remove 
// true or set to false when Serial is no longer beings used.
// passed to nearly all class objects.
Messaging::MsgLogHandler msglogerr(OLED, 5, true);

NVS::NVSctrl controller(msglogerr, "NS");

// Uses prefs library to store to the NVS. Creds are network credentials
// and periphSettings are for all sensor settings.
NVS::Credentials Creds{"Network", msglogerr}; 
NVS::PeripheralSettings periphSettings{"periphSettings", msglogerr}; 

// Created with default password, will set password during init if 
// one is saved in the NVS, or if one is passed in WAPSetup mode.
Comms::WirelessAP wirelessAP = {
    SERVER_NAME, SERVER_PASS_DEFAULT, // WAP SSID & Pass
    IPAddress(192, 168, 1, 1), // WAP localIP
    IPAddress(192, 168, 1, 1), // WAP gateway
    IPAddress(255, 255, 255, 0), // WAP subnet
    msglogerr
};

// Station mode network object.
Comms::Station station(msglogerr);

// Timer object to check 3-way switch poisition at set interval
Clock::Timer checkWifiModeSwitch{1000}; 

// Peripheral AND THREADS (adjust intervals for production).
Peripheral::TempHum tempHum{Peripheral::PERPIN::DHT, DHT22, 3, msglogerr, 4000};
Peripheral::Light light{Peripheral::PERPIN::PHOTO, 3, msglogerr, 3000};  
Peripheral::Soil soil1{Peripheral::PERPIN::SOIL1, 3, msglogerr, 10000};

Peripheral::Sensors* allSensors[Peripheral::PeripheralQty] // updated on PeripheralMain.h
    {&tempHum, &light, &soil1}; 

Threads::SensorThread sensorThread(msglogerr); // Master single thread

// Send thread to OTA to be suspended during OTA updates
UpdateSystem::OTAupdates otaUpdates{OLED, sensorThread};

// Used to determine free heap, avoids div by 0
size_t networkManager::startupHeap{1}; 

void setup() {
  networkManager::startupHeap = ESP.getFreeHeap(); 

  pinMode(static_cast<int>(Comms::NETPIN::WAPswitch), INPUT_PULLUP);  // Wireless Access Point
  pinMode(static_cast<int>(Comms::NETPIN::STAswitch), INPUT_PULLUP); // Station
  pinMode(static_cast<int>(Comms::NETPIN::defaultWAPSwitch), INPUT_PULLUP); // default password override
  pinMode(static_cast<int>(Peripheral::PERPIN::RE1), OUTPUT);
  pinMode(static_cast<int>(Peripheral::PERPIN::SOIL1), INPUT);
  pinMode(static_cast<int>(Peripheral::PERPIN::PHOTO), INPUT);

  Serial.begin(115200); // DELETE WHEN PROJECT COMPLETE1!!!!!!!!!!!!!!!!!
  Wire.begin(); // Setup the i2c bus.

  soil1.init(periphSettings); // WORKING WILL DO THIS TO ALL PERIPH
  delay(500); light.begin(); // must call after Wire.begin(); delay to init
  tempHum.begin(); 
  OLED.init(); // starts the OLED 

  sensorThread.initThread(allSensors); // begins the new thread

  // Checks the default WAP button position during start. If depressed,
  // will start in default mode. Beneficial for forgotten passwords.
  networkManager::initializeWAP(
    digitalRead(static_cast<int>(Comms::NETPIN::defaultWAPSwitch)), Creds, 
    wirelessAP, station, msglogerr);

  // DELETE TESTING ONLY !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  // int a{0};
  // controller.readNum("key1", NVS::DType::INT32_T, a);

}

void loop() { 
  
  otaUpdates.manageOTA(station);

  switch(checkWifiModeSwitch.isReady()) {
    case true:

    networkManager::handleWifiMode(
      wirelessAP, station,
      OLED, otaUpdates, Creds,
      SERVER_PASS_DEFAULT, SERVER_NAME, IPADDR
      );
  }

  // Checks to see if OLED has displayed error, if so, sets reminder
  // to clear in the argued time in seconds.
  msglogerr.OLEDMessageCheck(); 

  station.handleServer(); // Doesn't matter if station or wirelessAP
}


