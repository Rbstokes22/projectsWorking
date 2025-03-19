// CURRENT NOTES: 

// TESTING: Test everything once client page is up an running. Ensure that WAP
// mode prevents certain socket commands. CODE COMPLETE FOR NOW.

// ALERTS AND SUBSCRIPTION: I think I am set on using twilio from the server only. When a user
// subscribes, they will receive an API key that they would enter in the WAP setup page. This would
// be the key they use to send alerts and communicate with the database. Probably include this with
// the version check with the client page, that it sends back subscibed, non-subscribed, or 
// expired. If not subscibed, it wouldnt even attempt sending any alerts to save on web traffic. 
// Maybe that isnt too feasible, explore when crossing this bridge. Maybe alerts should be 
// exclusive to critical things. Such as temperature, humidity, dryness. I dont think light will
// warrant an alerts, since a daily wrapup will show them their averages and their light quality.

// TEST RESULTS:
// Temphum. Tested relays and alerts for the SHT. Relays turn on and off when criteria met.
// Alerts send, and reset when criteria is met. Both temp and hum are independent. Ran temphum
// test in report to ensure it works. Without it being set, at 2359 the values were refreshed
// and the previous values were populated. Clearing temphum average works. Report is sent properly
// and turned off using 99999, works.

// Soil. Tested alert values for soil1 and soil2. A single message is sent when criteria is met,
// and reset when the hysteresis criteria is met. All functions as expected.

// Log. Tested log to ensure that when new space was required, the beginning entry was deleted. 
// Ran test for several attempts where a value, via text, was always added in and increased 
// exponentially. When needed, The first several entries were stripped, showing it works as 
// advertised. Tested via web, getting and splitting log into array and displaying all log
// entries.

// Autosave. Tested autosave with initial settings. Worked as advertised. Next tried auto
// save on changing the temperature and humdity relays. Worked as advertised, both saved
// and loaded as expected. 

// Time calibrations work as expected.

// PRE-production notes:
// Create a datasheet for socket handling codes.
// Change in config.cpp, devmode = false for production.
// On the STAOTA handler, change skip certs to false, and remove header for NGROK. The current
// settings apply to NGROK testing only. Do the same for OTAupdates.cpp. On Mutex.cpp, delete
// the print statements from lock and unlock once all testing is done with peripherals and
// everything, this is to ensure they work.

// PERSISTING NOTES:
// Singleton classes have a static mutex which is located in the get() methods
// and protect all subsequent calls. If params are required, ensure that a 
// proper init occurs, elsewise, program will crash since a nullptr is returned.

// Polling in the averages for the SHT and AS7341 is a size_t variable. This
// leaves 136 years of polling, No polling should be quicker than 1 poll per
// second.

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "I2C/I2C.hpp"
#include "nvs_flash.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "Config/config.hpp"
#include "Drivers/SSD1306_Library.hpp" 
#include "UI/Display.hpp"
#include "UI/MsgLogHandler.hpp"
#include "Common/Timing.hpp"
#include "Threads/Threads.hpp"
#include "Threads/ThreadParameters.hpp"
#include "Threads/ThreadTasks.hpp"
#include "Network/NetCreds.hpp"
#include "Network/NetManager.hpp"
#include "Network/NetSTA.hpp"
#include "Network/NetWAP.hpp"
#include "Network/Handlers/WAPsetupHandler.hpp"
#include "Network/Handlers/STAHandler.hpp"
#include "Network/Handlers/socketHandler.hpp"
#include "OTA/OTAupdates.hpp"
#include "FirmwareVal/firmwareVal.hpp"
#include "esp_spiffs.h"
#include "esp_vfs.h"
#include <cstddef>
#include "Drivers/SHT_Library.hpp"
#include "Drivers/AS7341/AS7341_Library.hpp" 
#include "Peripherals/Relay.hpp"
#include "Peripherals/saveSettings.hpp"

extern "C" {
    void app_main();
}

// GLOBALS 
// Prefered using globals over local variables declared statically.
UI::Display OLED;

// Network specific objects. Station, Wireless Access Point, and their manager.
Comms::NetSTA station(MDNS_NAME); 
Comms::NetWAP wap(AP_SSID, AP_DEF_PASS, MDNS_NAME);
Comms::NetManager netManager(station, wap, OLED);

// PERIPHERALS
SHT_DRVR::SHT sht; // Temp hum driver 
AS7341_DRVR::CONFIG lightConf(AS7341_ASTEP, AS7341_ATIME, AS7341_WTIME);
AS7341_DRVR::AS7341basic light(lightConf);

Peripheral::Relay relays[TOTAL_RELAYS] = { // Passes to socket handler
    {CONF_PINS::pinMapD[static_cast<uint8_t>(CONF_PINS::DPIN::RE1)], 1},
    {CONF_PINS::pinMapD[static_cast<uint8_t>(CONF_PINS::DPIN::RE2)], 2},
    {CONF_PINS::pinMapD[static_cast<uint8_t>(CONF_PINS::DPIN::RE3)], 3},
    {CONF_PINS::pinMapD[static_cast<uint8_t>(CONF_PINS::DPIN::RE4)], 4}
};

// THREADS
Threads::netThreadParams netParams(NET_FRQ, netManager); // Net
Threads::Thread netThread("NetThread"); // DO NOT SUSPEND

Threads::SHTThreadParams SHTParams(SHT_FRQ, sht);  // Temp and Humidity
Threads::Thread SHTThread("SHTThread");

Threads::AS7341ThreadParams AS7341Params(AS7341_FRQ, light, 
    CONF_PINS::adc_unit); // light
Threads::Thread AS7341Thread("AS7341Thread");

Threads::soilThreadParams soilParams(SOIL_FRQ, CONF_PINS::adc_unit); // Soil 
Threads::Thread soilThread("soilThread");

// Routine thread such as randomly monitoring and managing sensor states.
Threads::routineThreadParams routineParams(ROUTINE_FRQ, relays, TOTAL_RELAYS);
Threads::Thread routineThread("routineThread");

Threads::Thread* toSuspend[TOTAL_THREADS] = {&SHTThread, &AS7341Thread, 
    &soilThread, &routineThread};

// OTA 
OTA::OTAhandler ota(OLED, station, toSuspend, TOTAL_THREADS); 

void app_main() {
    // Add OLED to msglogerr handler
    Messaging::MsgLogHandler::get()->addOLED(OLED);

    char log[30] = {0}; // Used for logging, size accomodates all msging.
    Messaging::Levels msgType[] = { // Used for mounting and init, with log.
        Messaging::Levels::CRITICAL, Messaging::Levels::INFO
    };

    CONF_PINS::setupDigitalPins(); // Config.hpp
    CONF_PINS::setupAnalogPins(CONF_PINS::adc_unit); // Config.hpp

    // Init I2C at frequency 100 khz.
    Serial::I2C::get()->i2c_master_init(Serial::I2C_FREQ::STD);

    // Init OLED, AS7341 light sensor, and sht temp/hum sensor.
    OLED.init(OLED_ADDR);
    light.init(AS7341_ADDR);
    sht.init(SHT_ADDR); 

    // Init credential singleton with global parameters above
    // which will also init the NVS.
    static NVS::CredParams cp = {CRED_NAMESPACE}; // used for creds init
    NVS::Creds::get(&cp);

    // Mount spiffs with path /spiffs.
    esp_vfs_spiffs_conf_t spiffsConf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    // Do not unregister for OTA update purposes since signature and checksum are
    // uploaded to spiffs.
    esp_err_t spiffs = esp_vfs_spiffs_register(&spiffsConf);
    
    snprintf(log, sizeof(log), "SPIFFS mounted = %d", (spiffs == ESP_OK));

    // Adjust message level depending on success.
    Messaging::MsgLogHandler::get()->handle(msgType[spiffs == ESP_OK],
            log, Messaging::Method::SRL_LOG);

    // Checks partition upon boot to ensure that it is valid by matching
    // the signature with the firmware running. If invalid, the program
    // will not continue.
    if (!BYPASS_VAL) {
        Boot::val_ret_t bootErr = Boot::FWVal::get()->checkPartition(
            Boot::PART::CURRENT, FIRMWARE_SIZE, FIRMWARE_SIG_SIZE);

        if (bootErr == Boot::val_ret_t::INVALID) { // FW invalid
            OLED.invalidFirmware(); // Displays message on OLED

            // If invalid, tries to rollback to the previously working
            // firmware and restarts
            if (ota.rollback()) esp_restart(); 
        } 
    }

    // Passes object to http handlers. No error handling as all logging
    // is done within the handler classes themselves. All public class methods
    // will not run, at least the gateways, without init.
    Comms::WSHAND::init(station, wap);
    Comms::OTAHAND::init(ota);
    Comms::SOCKHAND::init(relays);

    // Start threads, and init periph. Must occur before loading settings
    // due to initialized periph params before calling the get(), to prevent
    // nullptr return.
    netThread.initThread(ThreadTask::netTask, 4096, &netParams, 2);
    SHTThread.initThread(ThreadTask::SHTTask, 4096, &SHTParams, 1); 
    AS7341Thread.initThread(ThreadTask::AS7341Task, 4096, &AS7341Params, 3);
    soilThread.initThread(ThreadTask::soilTask, 4096, &soilParams, 3);
    routineThread.initThread(ThreadTask::routineTask, 4096, &routineParams, 3);

    // Sends pointer of relay array to initRelays method, and then loads all
    // the last saved data.
    NVS::settingSaver::get()->initRelays(relays);
    NVS::settingSaver::get()->load(); 
}

