// CURRENT NOTES: 

// Reports done and need to be tested to ensure that they send with the programmed
// and default modes, so programmed and at 2359.

// Removed checksum appending and updated firmwareVal. Test to ensure that OTA works
// via LAN and web using ngrok.

// Once tests are complete, move to soil and mirror some features of tempHum. Averages
// are not important here, since all reporting will use only current values since not
// a significant change in average is expected in a 24 hour basis. Look at potentially
// creating a simliar RW packet into soil as there is in the SHT. This might not
// work here.

// Once soil is complete, move over to light and mirror features of temp hum. Averages 
// will be important here. Read the reporting data to show how the averages will be 
// obtained as well as max intensity using clear. Look at creating a similiar RW packet
// into the AS7341 driver, and also that there is a timeout method that mirros the SHT
// driver. Explor how relays and/or alerts work with the light meter, such as will a 
// relay be energized at a certain darkness level, and be shut off after reaching a total
// amount of light from the as7341? 

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

// PRE-production notes:
// Create a datasheet for socket handling codes.
// Change in config.cpp, devmode = false for production.
// On the STAOTA handler, change skip certs to false, and remove header for NGROK. The current
// settings apply to NGROK testing only. Do the same for OTAupdates.cpp. On Mutex.cpp, delete
// the print statements from lock and unlock once all testing is done with peripherals and
// everything, this is to ensure they work.

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

extern "C" {
    void app_main();
}

// If set to true, will bypass firmware validation process in the startup. 
// Used for testing
bool bypassValidation = true;

// GLOBALS
UI::Display OLED;
Messaging::MsgLogHandler msglogerr(OLED, 5, true);
adc_oneshot_unit_handle_t adc_unit; // Used for the ADC channels.

Comms::NetSTA station(msglogerr, mdnsName); 
Comms::NetWAP wap(msglogerr, APssid, APdefPass, mdnsName);
Comms::NetManager netManager(station, wap, OLED);

NVS::CredParams cp = {credNamespace, msglogerr}; // used for creds init

// PERIPHERALS
SHT_DRVR::SHT sht; // Temp hum driver 
AS7341_DRVR::CONFIG lightConf(599, 29, 0);
AS7341_DRVR::AS7341basic light(lightConf);

const size_t totalRelays{4};
Peripheral::Relay relays[totalRelays] = { // Passes to socket handler
    {pinMapD[static_cast<uint8_t>(DPIN::RE1)], 1},
    {pinMapD[static_cast<uint8_t>(DPIN::RE2)], 2},
    {pinMapD[static_cast<uint8_t>(DPIN::RE3)], 3},
    {pinMapD[static_cast<uint8_t>(DPIN::RE4)], 4}
};

// THREADS
Threads::netThreadParams netParams(1000, netManager, msglogerr);
Threads::Thread netThread(msglogerr, "NetThread"); // DO NOT SUSPEND

Threads::SHTThreadParams SHTParams(1000, sht, msglogerr); // REPLACE WITH SHT
Threads::Thread SHTThread(msglogerr, "SHTThread");

Threads::AS7341ThreadParams AS7341Params(2000, light, msglogerr, adc_unit);
Threads::Thread AS7341Thread(msglogerr, "AS7341Thread");

Threads::soilThreadParams soilParams(1000, msglogerr, adc_unit);
Threads::Thread soilThread(msglogerr, "soilThread");

Threads::routineThreadParams routineParams(1000, relays, totalRelays);
Threads::Thread routineThread(msglogerr, "routineThread");

const size_t threadQty = 4;
Threads::Thread* toSuspend[threadQty] = {
    &SHTThread, &AS7341Thread, &soilThread, &routineThread
    };

// OTA 
OTA::OTAhandler ota(OLED, station, msglogerr, toSuspend, threadQty); 

void setupDigitalPins() {
    struct pinConfig {
        DPIN pin;
        gpio_mode_t IOconfig;
        gpio_pull_mode_t pullConfig;
    };

    // Floating on outputs has no function within lambda.
    pinConfig pins[DigitalPinQty] = {
        {DPIN::WAP, GPIO_MODE_INPUT, GPIO_PULLUP_ONLY},
        {DPIN::STA, GPIO_MODE_INPUT, GPIO_PULLUP_ONLY},
        {DPIN::defWAP, GPIO_MODE_INPUT, GPIO_PULLUP_ONLY},
        {DPIN::RE1, GPIO_MODE_OUTPUT, GPIO_FLOATING},
        {DPIN::RE2, GPIO_MODE_OUTPUT, GPIO_FLOATING},
        {DPIN::RE3, GPIO_MODE_OUTPUT, GPIO_FLOATING},
        {DPIN::RE4, GPIO_MODE_OUTPUT, GPIO_FLOATING}
    };

    auto pinSet = [](pinConfig pin){
        uint8_t pinIndex = static_cast<uint8_t>(pin.pin);
        gpio_num_t pinNum = pinMapD[pinIndex];

        ESP_ERROR_CHECK(gpio_set_direction(pinNum, pin.IOconfig));

        if (pin.IOconfig != GPIO_MODE_OUTPUT) { // Ignores OUTPUT pull
            ESP_ERROR_CHECK(gpio_set_pull_mode(pinNum, pin.pullConfig));
        }
    };

    for (int i = 0; i < DigitalPinQty; i++) {
        pinSet(pins[i]);
    }
}

void setupAnalogPins(adc_oneshot_unit_handle_t &unit) {
    adc_oneshot_unit_init_cfg_t unit_cfg{};
    unit_cfg.unit_id = ADC_UNIT_1;
    unit_cfg.ulp_mode = ADC_ULP_MODE_DISABLE;

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &unit));

    APIN pins[AnalogPinQty] = {
        APIN::SOIL1, APIN::SOIL2, APIN::SOIL3, APIN::SOIL4, APIN::PHOTO
        };

    // Configure the ADC channel
    adc_oneshot_chan_cfg_t chan_cfg{};
    chan_cfg.bitwidth = ADC_BITWIDTH_DEFAULT,
    chan_cfg.atten = ADC_ATTEN_DB_12;  // Highest voltage reading.

    auto pinSet = [unit, &chan_cfg](APIN pin){
        adc_channel_t pinNum = pinMapA[static_cast<uint8_t>(pin)];
 
        ESP_ERROR_CHECK(adc_oneshot_config_channel(
            adc_unit, pinNum, &chan_cfg
        ));
    };

    for (int i = 0; i < AnalogPinQty; i++) {
        pinSet(pins[i]);
    }
}

void app_main() {
    setupDigitalPins();
    setupAnalogPins(adc_unit);

    // Init I2C at frequency 100 khz.
    Serial::I2C::get()->i2c_master_init(Serial::I2C_FREQ::STD);

    // Init OLED, AS7341 light sensor, and sht temp/hum sensor.
    OLED.init(0x3C);
    light.init(0x39);
    sht.init(0x44); 

    // Init credential singleton with global parameters above
    // which will also init the NVS.
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
    if (spiffs != ESP_OK) {
        printf("SPIFFS FAILED TO MOUNT");
    }

    // Checks partition upon boot to ensure that it is valid by matching
    // the signature with the firmware running. If invalid, the program
    // will not continue.
    if (bypassValidation == false) {
        Boot::VAL err = Boot::checkPartition(
            Boot::PART::CURRENT, 
            FIRMWARE_SIZE, 
            FIRMWARE_SIG_SIZE
            );

        if (err != Boot::VAL::VALID) {
            OLED.invalidFirmware(); 
            
            if (ota.rollback()) esp_restart(); 
            return;
        }
    } 

    // Passes objects to the WAPSetup handler to allow for use.
    bool isInit = Comms::WSHAND::init(station, wap);
    printf("WAP Setup Handler init: %d\n", isInit);

    // Passes OTA object to the STAOTA handler to allow for use.
    isInit = Comms::OTAHAND::init(ota);
    printf("OTA Handler init: %d\n", isInit);

    // Sends the peripheral threads to the socket handler to allow mutex
    // usage. Also sends relays to allow client to configured them.
    isInit = Comms::SOCKHAND::init(relays);
    printf("Socket Handler init: %d\n", isInit);

    // Start threads
    netThread.initThread(ThreadTask::netTask, 4096, &netParams, 2);
    SHTThread.initThread(ThreadTask::SHTTask, 4096, &SHTParams, 1); 
    AS7341Thread.initThread(ThreadTask::AS7341Task, 4096, &AS7341Params, 3);
    soilThread.initThread(ThreadTask::soilTask, 4096, &soilParams, 3);
    routineThread.initThread(ThreadTask::routineTask, 4096, &routineParams, 3);
}

