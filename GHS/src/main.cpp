// CURRENT NOTES: 

// TO DO:

// Write config and build ADC into routine params just as a test. Once good, 
// incorporate into soil, and the do another with the photo. or cut a 
// soil and add light, or figure soemthing out. First test for working before 
// building.

// Ensure reports and alerts, or pretty much anything reaching out is disabled
// when not in STA mode. Disabled alerts and reports, as well as public methods
// in the OTA update class. There should be no other functions that reach out
// on the net besides those that talk to the server and have been disabled.

// Test that the relay energizes if previously energized but force off is rmvd.
// Test relay qty and manual.

// Test MDNS change. Tested good for changing from 0 to 7, but noticed when
// flipped to wap setup, the mdns said greenhouse.local instead of 
// greenhouse2.local, for example. Check into this.

// Run checks on the timer day setting. just need to run
// checks. Test relay 0, and adjust days to match. Also ensure that if the
// following day is not selected, the timer does go off at midnight. Once complete
// build the client side to incorporate this. 

// Test report. Report attempts to send on the hour now to the server. The 
// timer set is for clearing averages only. This also need to be tested.
// The report will send the entire JSON socket buffer data to the server to be 
// uploaded into a database. This has read only functionality now, but I might
// incorporate an write functionality in the future, that will be a doozy to
// update everything. Might need some sort of encoding technique that could
// send only user changes and not everything, but in some sort of bitwise format
// combined with JSON. So the JSON could have for example:
// {"reTmr": [32, 32]} // where bits 28-31 indicate the relay number
// and on the receiving side, this, it would take the JSON data, and figure out
// the corresponding socket commands and use SOCKHAND::compileData, which 
// returns and does nothing, but can execute socket commands. We would just
// use a useless buffer to meet the requirements, and pass the command data
// like we would a socket. It shouldnt be too difficult. This would allow the 
// client to make some adjustments, and the adjustments would be sent to the
// device on the next report, so instead of responding "OK" to the report, it
// would send the json and bitwise, which would then be routed and handled 
// appropriately, allowing the client to see the updates on the following 
// report. This is the best way to manage it since its a polled report. See how
// feasible this will be. If this isnt bad, incorporate before finishing the
// client page. This obviously cant be tested until actual server is built, but
// using a PWA, it shouldnt be too difficult to incorporate something like this.

// Test save light to ensure integration setting changes.

// Run testing below when there is a developed client side that makes 
// switching easier.

// Run elaborative test on the AS7341. First test the integration time. It is 
// currently set to 50 with an ASTEP of 599, and ATIME of 29. Adjust it to 50
// two more times by altering both values, and see if the results are about the
// same. If yes, it shows that integration time is the final factor and it does
// not matter if the ASTEP or ATIME is the adjusted value. 

// Next reset to default and see if the counts are proportional to the inverse
// square law. Decrease the distance by 2, to the source, and see if the counts
// quadrouple, If not, Use different values to establish a pattern.

// Next set the integration time to 25, 50, 75, and 100. Choose 3 values and 
// see how the results correspond with the change in integration time. When
// doubling, do the counts double, say from 25 to 50? If they go n % from 25 to
// 50, do they go up by the same percentage from 50 to 75? Maybe they go up the
// same percentage from 50 to 100, so it could be a quadratic and non-linear.

// Mess with the gain as well, and see how that might affect the counts, does
// doubling the gain also double the counts? What is the relationship?

// The goal is to try to tie PPFD to the counts. If I can establish a pattern,
// then I can create a PPFD which can be manually entered and correspond to 
// 50ms 256x gain. The settings might not even matter. If I get 1000 umol and
// a clear count of 5000, on default, If my clear count follows the ISL, PPFD
// can be connected to that. If adjustments are made to where the clear count is 
// 7000 because of higher int time, How can that new value be tied to PPFD? PPFD
// would remain the same, but would it be computed by some proportionality 
// variable? Thinking must be done before implementation.

// COMPLETE:
// photoresistor trends.
// AGAIN, ASTEP, and ATIME built in both driver and peripheral. 
// Tested all Axxxx, works as advertised.
// Tested RELAY_CTRL, works as advertised.
// Tested ATTACH_RELAYS, works as advertised.
// Tested SET_TEMPHUM, works as advertised.
// Tested SET_SOIL, works as advertised.
// Tested RELAY_TIMER, works as advertised after some ID issues.
// Tested SET_LIGHT, works as advertised.
// Tested CLEAR_AVERAGES, works as advertised.

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
// second to achieve this.

// Ran tests on integration time for the AS7341. Suspected that increasing the
// int time by n, would also yield proportional counts. I wanted to calibrate
// PPFD to this value, and scale it accordingly so that the user could always
// estimate the ppfd depending on counts that were adjusted to the default
// level. This could still be fesible, run additional tests.

// Scrapped the report sending time, it will auto on the hour, and will allow
// the ability to modify settings, which will occur at the next hour. Data based
// on the smallest linode with 25 GB storage and 1 TB transfer per month.
// We assume each get will return 2 KB data to the client. Limit client to 24
// checks per day for 48 KB of data. This is accumulative to 1460 KB per month
// or 730 checks per month. Each client will get 3 KB to hold their sensor
// data, as well as API key and other data, so roughly 1 KB for that. The 
// limiting factor is the 1 TB transfer, which will support roughly 723K clients
// @ 2.25 GB of storage. 

// Was having issues with relay settings saving over and over causing a rewrite
// but the problem has been fixed. Continue watching.

// In manual, include the MDNS 3-pin logic that allows you to have values
// greenhouse.local to greenhouse7.local.

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
    void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName);
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
    {CONF_PINS::pinMapD[static_cast<uint8_t>(CONF_PINS::DPIN::RE0)], 0},
    {CONF_PINS::pinMapD[static_cast<uint8_t>(CONF_PINS::DPIN::RE1)], 1},
    {CONF_PINS::pinMapD[static_cast<uint8_t>(CONF_PINS::DPIN::RE2)], 2},
    {CONF_PINS::pinMapD[static_cast<uint8_t>(CONF_PINS::DPIN::RE3)], 3}
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
// Must be called at a 1 Hz frequency to ensure proper management throughout 
// the program.
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
    netThread.initThread(ThreadTask::netTask, NET_WORDS, &netParams, 2);
    SHTThread.initThread(ThreadTask::SHTTask, SHT_WORDS, &SHTParams, 1); 
    soilThread.initThread(ThreadTask::soilTask, SOIL_WORDS, &soilParams, 3);

    AS7341Thread.initThread(ThreadTask::AS7341Task, AS7341_WORDS, 
        &AS7341Params, 3);

    routineThread.initThread(ThreadTask::routineTask, ROUTINE_WORDS, 
        &routineParams, 3);

    // Sends pointer of relay array to initRelays method, and then loads all
    // the last saved data.
    NVS::settingSaver::get()->initRelays(relays);
    NVS::settingSaver::get()->load(); 
}

// Used if stack overflow is detected. Will save and restart while logging
// the previous entries to the NVS to be loaded upon restart.
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *taskName) {

    char log[128]{0};
    snprintf(log, sizeof(log), "Stack Overflow detected in task %s", taskName);
    

    Messaging::MsgLogHandler::get()->handle(Messaging::Levels::CRITICAL,
        log, Messaging::Method::SRL_OLED_LOG);

    taskDISABLE_INTERRUPTS(); // disable interrupts to prevent further proc.
    static uint8_t count = 0;

    while (true) { // Handle business here, then execute a save and restart.

        printf("Shutting down in %u\n", count);

        if (count++ >= STACK_OVERFLOW_RESTART_SECONDS) {
            NVS::settingSaver::get()->saveAndRestart(); 
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // 1s delay.
    }
}

