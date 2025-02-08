#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <cstdint>
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_continuous.h"

// bypass firmware validation when starting and OTA updates. Set to true 
// during development phase.
#define BYPASS_VAL true 

// Network connection and broadcasting information
#define AP_SSID "GreenHouse"
#define MDNS_NAME "greenhouse"
#define AP_DEF_PASS "12345678"
#define CRED_NAMESPACE "netcreds"

// Web Paths
#define ALERT_PATH "/Alerts"

// OTA Updates
#define FIRMWARE_VERSION "1.0.0"
#define OTA_VERSION_PATH "/checkOTA"
#define LAN_OTA_SIG_PATH "/sigUpdate" 
#define LAN_OTA_FIRMWARE_PATH "/FWUpdate"
#define FIRMWARE_SIZE 1200288
#define FIRMWARE_SIG_SIZE 256

// Uncommend whichever WEBURL you are using
// #define WEBURL "https://major-absolutely-bluejay.ngrok-free.app"
#define WEBURL "http://shadyside.local:8080"

// AS7341 (default values by datasheet are 599, 29, 0)
#define AS7341_ASTEP 599 
#define AS7341_ATIME 29
#define AS7341_WTIME 0

// I2C Addresses
#define OLED_ADDR 0x3C
#define AS7341_ADDR 0x39
#define SHT_ADDR 0x44

// Developer mode, This affects certain things like NGROK server headers.
#define DEVmode true // When set to false, production mode.

// MSGLOGERR
#define SERIAL_ON true // if set to true, will write to serial.
#define MSG_CLEAR_SECS 5 // clears OLED error messages after n seconds.

// Used in functions to init object. 
#define TOTAL_RELAYS 4 
#define TOTAL_THREADS 4  

// Sample/polling frequencies in ms that each thread is executed.
#define NET_FRQ 1000
#define SHT_FRQ 1000
#define AS7341_FRQ 2000
#define SOIL_FRQ 1000
#define ROUTINE_FRQ 5000

// GPIO configuration.
namespace CONF_PINS {
    #define AnalogPinQty 5
    #define DigitalPinQty 7

    extern adc_oneshot_unit_handle_t adc_unit; // Used for the ADC channels.
    void setupDigitalPins();
    void setupAnalogPins(adc_oneshot_unit_handle_t &unit);
    extern adc_channel_t pinMapA[AnalogPinQty];
    extern gpio_num_t pinMapD[DigitalPinQty];
    // Used with pinMaps above.
    enum class DPIN : uint8_t {WAP, STA, defWAP, RE1, RE2, RE3, RE4};
    enum class APIN : uint8_t {SOIL1, SOIL2, SOIL3, SOIL4, PHOTO};
}

// NetConfig, does not include the CONF in the namespace due to this.
namespace Comms {

enum class NetMode {WAP, WAP_SETUP, STA, NONE, WAP_RECON};

enum class Constat {CON, DISCON};

enum class IDXSIZE {
    SSID = 33, // 1-32 chars
    PASS = 65, // 8-64 chars
    PHONE = 11, // 10 chars exactly
    APIKEY = 9, // 8 chars exactly
    NETCREDKEYQTY = 5,
    IPADDR = 16, // 7-15 chars
    MDNS = 11, // 1-10 chars 
};

// Used in netCreds.cpp and WAPSetupHandler.cpp, used in NVS
enum class KI {ssid, pass, phone, WAPpass, APIkey}; // Key Index
extern const char* netKeys[static_cast<int>(IDXSIZE::NETCREDKEYQTY)];

}

#endif // CONFIG_HPP