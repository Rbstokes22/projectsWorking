#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <cstdint>
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_continuous.h"

#define CONFTAG "(CONFIG)"
extern char confLog[50]; // Reusable log for config src.

// bypass firmware validation when starting and OTA updates. Set to true 
// during development phase.
#define BYPASS_VAL true 

// Network connection and broadcasting information
#define AP_SSID "GreenHouse"
#define MDNS_NAME "greenhouse"
#define MDNS_ACTUAL "http://greenhouse.local" // Used for whitelist
#define AP_DEF_PASS "12345678"
#define CRED_NAMESPACE "netcreds"
#define WAP_MAX_CONNECTIONS 4 // Max allowable users.

// OLED displays
#define OLED_COMPANY_NAME "SSTech 2024"
#define OLED_DEVICE_NAME "Mumsy's Greenhouse"

// Web Paths
#define ALERT_PATH "/Alerts"

// OTA Updates
#define FIRMWARE_VERSION "1.0.0"
#define OTA_VERSION_PATH "/checkOTA"
#define LAN_OTA_SIG_PATH "/sigUpdate" 
#define LAN_OTA_FIRMWARE_PATH "/FWUpdate"
#define FIRMWARE_SIZE 1200288
#define FIRMWARE_SIG_SIZE 256

// Uncomment whichever WEBURL you are using
// #define WEBURL "https://major-absolutely-bluejay.ngrok-free.app"
#define WEBURL "http://shadyside.local:8080"

// White list domains used in STAOTAHandler.cpp
#define LOCAL_IP "http://192.168" // All that is required
extern const char* whiteListDomains[3]; // WEBURL, LOCAL_IP and MDNS_ACTUAL

// AS7341 (default values by datasheet are 599, 29, 0). NOTE! AGAIN not included
// due to default setting of x256. Both ATIME and ASTEP default values are not
// acceptable, which is why they are included and changed. Recommend not 
// changing, and if changed, ensure the init procedures are updated.
#define AS7341_ASTEP 599 // Range from 0 - 65534. Rec 599 per datasheet.
#define AS7341_ATIME 29 // Range from 0 - 255. Rec 29 per datasheet.
#define AS7341_WTIME 0

// I2C Addresses
#define OLED_ADDR 0x3C
#define AS7341_ADDR 0x39
#define SHT_ADDR 0x44

// Developer mode, This affects certain things like NGROK server headers.
#define DEVmode true // When set to false, production mode.

// MSGLOGERR
#define SERIAL_ON true // if set to true, will enable serial.
#define MSG_CLEAR_SECS 5 // clears OLED error messages after n seconds.
#define CONSECUTIVE_ENTRIES 2 // Prevents a repeat log entry.
#define CONSECUTIVE_ENTRY_TIMEOUT 300 // Seconds that will allow repeat entries.

// Used in functions to init object. 
#define TOTAL_RELAYS 4 
#define TOTAL_THREADS 4  

// Sample/polling frequencies in ms that each thread is executed.
#define NET_FRQ 1000
#define SHT_FRQ 1000
#define AS7341_FRQ 2000
#define SOIL_FRQ 1000
#define ROUTINE_FRQ 1000 // Keep 1000 due to OLED messages.

// Autosave frequency in seconds, once per n seconds.
#define AUTO_SAVE_FRQ 60

// Temp/hum and light trend hours
#define TREND_HOURS 12 // Takes 12 hours on the hour of trend data.

// GPIO configuration.
namespace CONF_PINS {
    #define AnalogPinQty 5
    #define DigitalPinQty 10

    extern adc_oneshot_unit_handle_t adc_unit; // Used for the ADC channels.
    void setupDigitalPins();
    void setupAnalogPins(adc_oneshot_unit_handle_t &unit);
    extern adc_channel_t pinMapA[AnalogPinQty]; // Analog pin map.
    extern gpio_num_t pinMapD[DigitalPinQty]; // Digital pin map.

    // Used with pinMaps above. MD or mode 0, 1, and 2 are for the mdns. Uses
    // 3-bit operations and used for multiple devices. MD pins are pulled high,
    // and when low, reps the binary value with MD0 being bit 0. Allows for
    // mdns name to be greenhouse.local to greenhouse7.local.
    enum class DPIN : uint8_t {WAP, STA, defWAP, RE0, RE1, RE2, RE3, 
        MD0, MD1, MD2};

    enum class APIN : uint8_t {SOIL0, SOIL1, SOIL2, SOIL3, PHOTO};
}

// NetConfig, does not include the CONF in the namespace due to this.
namespace Comms {

enum class NetMode {WAP, WAP_SETUP, STA, NONE, WAP_RECON}; // Network Mode

enum class Constat {CON, DISCON}; // Connection Status

enum class IDXSIZE { // Network Index Sizes.
    SSID = 33, // 1-32 chars
    PASS = 65, // 8-64 chars
    PHONE = 11, // 10 chars exactly
    APIKEY = 9, // 8 chars exactly
    NETCREDKEYQTY = 5,
    IPADDR = 16, // 7-15 chars
    MDNS = 16, // 1-63 chars.
};

// Used in netCreds.cpp and WAPSetupHandler.cpp, used in NVS
enum class KI {ssid, pass, phone, WAPpass, APIkey}; // Key Index
extern const char* netKeys[static_cast<int>(IDXSIZE::NETCREDKEYQTY)]; // Netkeys

}

#endif // CONFIG_HPP