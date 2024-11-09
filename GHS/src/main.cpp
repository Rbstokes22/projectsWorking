// TO DO:

// 4. Change the DHT to be a singleton to mirror the soil. Remove boundary config
// to TempHumConf, and remove it from the relay.hpp.
// 5. build the light class to mirror the DHt and soil.
// 6. If the classes have similar features, make a base class, potentially abstract if needed
// as well as remove mutexs from the threads since they are inbedded in the classes. This means
// we can remove them from the SOCKHAND init as well.
// 7. Include stats in webpage. This way the user can see if spiffs, nvs, etc... is mounted
// Also include active sensors, or any other type of logging things. Can create a separate 
// status header/source, include it when needed, and update it by reference in the source.

// CURRENT NOTES: Continue working on averages. I should be done for the TempHum, maybe soil
// doesnt need an average. Light should definitely include an average for the spectral 
// reading. For the Light, Alerts might not be needed since it isnt critical. Maybe just
// a relay. Maybe scrap twilio and look at a strict linode PWA with push notifications.
// This will be a much more manageable and cheaper method. The phone number can be scrapped
// from the WAP setup since twilio will not need to be used. Look at having a remote subscription
// which will be a subscription to data storage, access with minute updates, and also alerts.
// The PWA will include a button for LOCAL which will open a greenhouse.local up in the web
// browser. For non-subsciptions, only the browser will be used. A cheap subscription fee 
// can pay for this, say 3 dollars per month, which means that even at 1000 customers, you
// could pay for each user to get upates several times a minute and still make like 2000 bucks
// a month with the best subscription plan that linode offers. Consider the differences between
// LAN mode and PWA mode, they will look like, its just that on the minute, the json data will 
// be sent to the PWA.

// PRE-production notes:
// Create a datasheet for socket handling codes.
// Change in config.cpp, devmode = false for production.
// On the STAOTA handler, change skip certs to false, and remove header for NGROK. The current
// settings apply to NGROK testing only. Do the same for OTAupdates.cpp. On Mutex.cpp, delete
// the print statements from lock and unlock once all testing is done with peripherals and
// everything, this is to ensure they work.

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
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
#include "Drivers/DHT_Library.hpp" 
#include "Drivers/AS7341/AS7341_Library.hpp" 
#include "Peripherals/Relay.hpp"

extern "C" {
    void app_main();
}

// If set to true, will bypass firmware validation process in the startup.
bool bypassValidation = true;

// GLOBALS
UI::Display OLED;
Messaging::MsgLogHandler msglogerr(OLED, 5, true);
adc_oneshot_unit_handle_t adc_unit; // Used for the ADC channels.

NVS::Creds creds(credNamespace, msglogerr);
Comms::NetSTA station(msglogerr, creds, mdnsName);
Comms::NetWAP wap(msglogerr, APssid, APdefPass, mdnsName);
Comms::NetManager netManager(station, wap, creds, OLED);

// PERIPHERALS
DHT_DRVR::DHT dht(pinMapD[static_cast<int>(DPIN::DHT)]);
AS7341_DRVR::CONFIG lightConf(599, 29, 0);
AS7341_DRVR::AS7341basic light(lightConf);

const size_t totalRelays{4};
Peripheral::Relay relays[totalRelays] = { // Passes to socket handler
    {pinMapD[static_cast<uint8_t>(DPIN::RE1)]},
    {pinMapD[static_cast<uint8_t>(DPIN::RE2)]},
    {pinMapD[static_cast<uint8_t>(DPIN::RE3)]},
    {pinMapD[static_cast<uint8_t>(DPIN::RE4)]}
};

// THREADS
Threads::netThreadParams netParams(1000, netManager, msglogerr);
Threads::Thread netThread(msglogerr, "NetThread"); // DO NOT SUSPEND

Threads::DHTThreadParams DHTParams(1000, dht, msglogerr);
Threads::Thread DHTThread(msglogerr, "DHTThread");

Threads::AS7341ThreadParams AS7341Params(2000, light, msglogerr, adc_unit);
Threads::Thread AS7341Thread(msglogerr, "AS7341Thread");

Threads::soilThreadParams soilParams(1000, msglogerr, adc_unit);
Threads::Thread soilThread(msglogerr, "soilThread");

Threads::relayThreadParams relayParams(1000, relays, totalRelays);
Threads::Thread relayThread(msglogerr, "relayThread");

const size_t threadQty = 4;
Threads::Thread* toSuspend[threadQty] = {
    &DHTThread, &AS7341Thread, &soilThread, &relayThread
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
        {DPIN::DHT, GPIO_MODE_INPUT, GPIO_FLOATING},
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
    OLED.init(0x3C);
    light.init(0x39);

    // Initialize NVS
    esp_err_t nvsErr = nvs_flash_init();  // Handle err in future.
    if (nvsErr == ESP_ERR_NVS_NO_FREE_PAGES || nvsErr == ESP_ERR_NVS_NEW_VERSION_FOUND) {
       ESP_ERROR_CHECK(nvs_flash_erase());
       nvsErr = nvs_flash_init();
    }

    printf("NVS STATUS: %s\n", esp_err_to_name(nvsErr));

    // Mount spiffs
    esp_vfs_spiffs_conf_t spiffsConf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    // Do not unregister for OTA update purposes.
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
    bool isInit = Comms::WSHAND::init(station, wap, creds);
    printf("WAP Setup Handler init: %d\n", isInit);

    // Passes OTA object to the STAOTA handler to allow for use.
    isInit = Comms::OTAHAND::init(ota);
    printf("OTA Handler init: %d\n", isInit);

    // Sends the peripheral threads to the socket handler to allow mutex
    // usage. Also sends relays to allow client to configured them.
    isInit = Comms::SOCKHAND::init(AS7341Params.mutex, relays);
    printf("Socket Handler init: %d\n", isInit);

    // Start threads
    netThread.initThread(ThreadTask::netTask, 4096, &netParams, 1);
    DHTThread.initThread(ThreadTask::DHTTask, 4096, &DHTParams, 3);
    AS7341Thread.initThread(ThreadTask::AS7341Task, 4096, &AS7341Params, 3);
    soilThread.initThread(ThreadTask::soilTask, 4096, &soilParams, 3);
    relayThread.initThread(ThreadTask::relayTask, 4096, &relayParams, 3);
}

