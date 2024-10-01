// TO DO:

// 8. Build peripherals (start with device drivers for the DHT and then the as7341)
// 9. Once drivers are good, build everything and integrate into webpage.
// 10. Include stats in webpage. This way the user can see if spiffs, nvs, etc... is mounted
// Also include active sensors, or any other type of logging things.

// Current Note:
// The AS7341 is running. Everything seems to be going well and I have all channel outputs.
// Run comparisons on values with the arduino as well as this, to ensure that the color
// counts match.

// PRE-production notes:
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
#include "Network/NetCreds.hpp"
#include "Network/NetManager.hpp"
#include "Network/NetSTA.hpp"
#include "Network/NetWAP.hpp"
#include "Network/Handlers/WAPsetupHandler.hpp"
#include "Network/Handlers/STAHandler.hpp"
#include "OTA/OTAupdates.hpp"
#include "FirmwareVal/firmwareVal.hpp"
#include "esp_spiffs.h"
#include "esp_vfs.h"
#include <cstddef>

#include "Drivers/DHT_Library.hpp"
#include "Drivers/AS7341/AS7341_Library.hpp"

extern "C" {
    void app_main();
}

// If set to true, will bypass firmware validation process in the startup.
bool bypassValidation = true;

// ALL OBJECTS
UI::Display OLED;
Messaging::MsgLogHandler msglogerr(OLED, 5, true);

// NET SETUP (default IP is 192.168.1.1).
const char APssid[]{"GreenHouse"};
const char mdnsName[]{"greenhouse"}; // must be under 11 chars.
char APdefPass[]{"12345678"};
char credNamespace[] = "netcreds";
NVS::Creds creds(credNamespace, msglogerr);
Comms::NetSTA station(msglogerr, creds, mdnsName);
Comms::NetWAP wap(msglogerr, APssid, APdefPass, mdnsName);
Comms::NetManager netManager(station, wap, creds, OLED);

// THREADS
Threads::netThreadParams netParams(1000, msglogerr);
Threads::Thread netThread(msglogerr, "NetThread"); // DO NOT SUSPEND

Threads::periphThreadParams periphParams(5000, msglogerr);
Threads::Thread periphThread(msglogerr, "PeriphThread");

const size_t threadQty = 1;
Threads::Thread* toSuspend[threadQty] = {&periphThread};

// OTA 
OTA::OTAhandler ota(OLED, station, msglogerr, toSuspend, threadQty); 

// PERIPHERALS
DHT_DRVR::DHT dht(pinMapD[static_cast<int>(DPIN::DHT)]);
AS7341_DRVR::CONFIG lightConf(599, 29, 0);
AS7341_DRVR::AS7341basic light(lightConf);

void netTask(void* parameter) {
    Threads::netThreadParams* params = 
        static_cast<Threads::netThreadParams*>(parameter);

    #define LOCK params->mutex.lock()
    #define UNLOCK params->mutex.unlock();

    Clock::Timer netCheck(1000); 
    light.setAGAIN(AS7341_DRVR::AGAIN::X256);

    while (true) {
        // in this portion, check wifi switch for position. 
        if (netCheck.isReady()) {
            netManager.handleNet();
        }

        msglogerr.OLEDMessageCheck(); // clears errors from display

        vTaskDelay(pdMS_TO_TICKS(params->delay));
    }
}

void periphTask(void* parameter) {
    Threads::periphThreadParams* params = 
        static_cast<Threads::periphThreadParams*>(parameter);

    #define LOCK params->mutex.lock()
    #define UNLOCK params->mutex.unlock();

   
    AS7341_DRVR::COLOR color{0};
    
    while (true) {
        bool RA = light.readAll(color);

        if (RA) {
            printf("\nReadouts\n");
            printf("Violet: %u\n", color.F1_415nm_Violet);
            printf("Indigo: %u\n", color.F2_445nm_Indigo);
            printf("Blue: %u\n", color.F3_480nm_Blue);
            printf("Cyan: %u\n", color.F4_515nm_Cyan);
            printf("Green: %u\n", color.F5_555nm_Green);
            printf("Yellow: %u\n", color.F6_590nm_Yellow);
            printf("Orange: %u\n", color.F7_630nm_Orange);
            printf("Red: %u\n", color.F8_680nm_Red);
            printf("Clear: %u\n", color.Clear);
            printf("NIR: %u\n", color.NIR);
            
        } else {
            printf("Data Errors\n");
        }
     
        vTaskDelay(pdMS_TO_TICKS(params->delay)); 
    }
}

void setupDigitalPins() {
    // Set all Pins
    ESP_ERROR_CHECK(gpio_set_direction(pinMapD[static_cast<int>(DPIN::WAP)], GPIO_MODE_INPUT));
    ESP_ERROR_CHECK(gpio_set_direction(pinMapD[static_cast<int>(DPIN::STA)], GPIO_MODE_INPUT));
    ESP_ERROR_CHECK(gpio_set_direction(pinMapD[static_cast<int>(DPIN::defWAP)], GPIO_MODE_INPUT));
    ESP_ERROR_CHECK(gpio_set_direction(pinMapD[static_cast<int>(DPIN::DHT)], GPIO_MODE_INPUT));
    ESP_ERROR_CHECK(gpio_set_direction(pinMapD[static_cast<int>(DPIN::RE1)], GPIO_MODE_OUTPUT));
    
    // Configure all pins 
    ESP_ERROR_CHECK(gpio_set_pull_mode(pinMapD[static_cast<int>(DPIN::WAP)], GPIO_PULLUP_ONLY));
    ESP_ERROR_CHECK(gpio_set_pull_mode(pinMapD[static_cast<int>(DPIN::STA)], GPIO_PULLUP_ONLY));
    ESP_ERROR_CHECK(gpio_set_pull_mode(pinMapD[static_cast<int>(DPIN::defWAP)], GPIO_PULLUP_ONLY));
}

void setupAnalogPins() {
    adc_oneshot_unit_handle_t adc_unit;
    adc_oneshot_unit_init_cfg_t unit_cfg{};
    unit_cfg.unit_id = ADC_UNIT_1;
    unit_cfg.ulp_mode = ADC_ULP_MODE_DISABLE;

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc_unit));
    
    // Configure the ADC channel
    adc_oneshot_chan_cfg_t chan_cfg{};
    chan_cfg.bitwidth = ADC_BITWIDTH_DEFAULT,
    chan_cfg.atten = ADC_ATTEN_DB_12;  // Highest voltage reading.

    ESP_ERROR_CHECK(adc_oneshot_config_channel(
        adc_unit, pinMapA[static_cast<int>(APIN::SOIL1)], &chan_cfg));

    ESP_ERROR_CHECK(adc_oneshot_config_channel(
        adc_unit, pinMapA[static_cast<int>(APIN::PHOTO)], &chan_cfg));
}

void app_main() {
    setupDigitalPins();
    setupAnalogPins();
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
    Comms::setJSONObjects(station, wap, creds);

    // Passes OTA object to the STA handler to allow for use.
    Comms::setOTAObject(ota);

    // Start threads
    netThread.initThread(netTask, 4096, &netParams, 1);
    periphThread.initThread(periphTask, 4096, &periphParams, 5);
}

