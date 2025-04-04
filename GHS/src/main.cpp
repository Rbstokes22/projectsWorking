// CURRENT NOTES: 

// On client.

// TESTING: Test everything once client page is up an running. Ensure that WAP
// mode prevents certain socket commands. CODE COMPLETE FOR NOW. When html
// is done, split into css, or however things need to be splitted into chunks,
// and served together using snprintf. Ideally this will make it possible to 
// render pages with certain features.

// PRE-production notes:
// Create a datasheet for socket handling codes.
// On relays 2 & 3, use a not outlet as well mostly for light, This way it can
// cover things when off. For example, once it is light, the lights shut off,
// and a pump starts running to heat up a barrel of water. Once the light is low,
// the pump kicks off, and the inside lights come one. Also put a 15 amp relay
// maybe on relay 4 to allow 1500 watts.


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

