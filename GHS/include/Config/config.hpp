#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <cstdint>
#include "driver/gpio.h"

#define CONFTAG "(CONFIG)"
extern char confLog[128]; // Reusable log for config src.

// bypass firmware validation when starting and OTA updates. Set to true 
// during development phase.
#define BYPASS_VAL true 

// Network connection and broadcasting information
#define AP_SSID "GreenHouse"
#define MDNS_NAME "greenhouse"
#define MDNS_ACTUAL "http://greenhouse" // Used for whitelist
#define AP_DEF_PASS "12345678"
#define CRED_NAMESPACE "netcreds"
#define WAP_MAX_CONNECTIONS 4 // Max allowable users.

// I2C GPIO PINS
#define I2C_SCL_PIN GPIO_NUM_22
#define I2C_SDA_PIN GPIO_NUM_21

// Sensor health information. These are used to monitor the health of all 
// sensors by accumulating upon error, and decaying upon each non-error.
// ATTENTION. if the error max is changed, update the station html.
#define HEALTH_ERR_UNIT 1.0f // Added per error
#define HEALTH_EXP_DECAY 0.9f // Decays rate upon each non-error.
#define HEALTH_ERR_MAX 10.0f // Will not exceed this score.
#define HEALTH_ERR_BAD 5.0f // Will flag device as problematic when err score
                            // exceeds  this value.
#define HEALTH_RECON_BAD 10.0f // Used for I2C reconnect health.
#define HEALTH_RECON_EXP_DECAY 0.99f // Decays rate upon each normal operation.

// Threads. The following values are in bytes. The documentation defines 
// StackType_t as 1 byte, as opposed to 4 bytes, which would explain a word 
// if used properly.
#define NET_STACK 8096 // Increased due to high water mark issues and overflow.
#define SHT_STACK 4096
#define LIGHT_STACK 4096
#define SOIL_STACK 4096
#define ROUTINE_STACK 4096

// OLED displays
#define OLED_COMPANY_NAME "SSTech 2024"
#define OLED_DEVICE_NAME "Castiel's Greenhouse"

// Web Paths
#define ALERT_PATH "/Alerts"

// OTA Updates
#define FIRMWARE_VERSION "1.0.0"
#define OTA_VERSION_PATH "/checkOTA"
#define LAN_OTA_SIG_PATH "/sigUpdate" 
#define LAN_OTA_FIRMWARE_PATH "/FWUpdate"
#define FIRMWARE_SIZE 1469456
#define FIRMWARE_SIG_SIZE 256

// Uncomment whichever WEBURL you are using
// #define WEBURL "https://major-absolutely-bluejay.ngrok-free.app"
#define WEBURL "http://mysterygraph.local:8080"
#define WEB_TIMEOUT_MS 3000 // timeout when in client mode in milliseconds.

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
#define ADC1_ADDR 0x48 // Bridged to ground (SOIL SENSORS)
#define ADC2_ADDR 0x49 // Bridged to VDD (PhotoResistor)

#define I2C_DEF_ADDR 0xFF // Used only as a placeholder until overwritten.

// ATTENTION. I2C Default frequency params in I2C.hpp. Set to SLOW @ 50kHz.

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

// Sample/polling frequencies in ms that each thread is executed. If any of
// these change, ensure to change the hearbeat checks on threadTasks.hpp.
#define NET_FRQ 1000
#define SHT_FRQ 1000
#define LIGHT_FRQ 3000 // Overruns at 2000 for polling, use 3000 min.
#define SOIL_FRQ 1000
#define ROUTINE_FRQ 1000 // Keep 1000 due to OLED messages and hearbeat.

// Autosave frequency in seconds, once per n seconds.
#define AUTO_SAVE_FRQ 60

// Temp/hum and light trend hours
#define TREND_HOURS 12 // Takes 12 hours on the hour of trend data.

#define STACK_OVERFLOW_RESTART_SECONDS 10 // Restarts after n seconds

// GPIO configuration.
namespace CONF_PINS {
    #define DigitalPinQty 10

    void setupDigitalPins();
    extern gpio_num_t pinMapD[DigitalPinQty]; // Digital pin map.

    // Used with pinMaps above. MD or mode 0, 1, and 2 are for the mdns. Uses
    // 3-bit operations and used for multiple devices. MD pins are pulled high,
    // and when low, reps the binary value with MD0 being bit 0. Allows for
    // mdns name to be greenhouse.local to greenhouse7.local. // NOTE: Relay
    // pins are going to be set low to power active low relays to avoid the
    // use of BJT.
    enum class DPIN : uint8_t {WAP, STA, defWAP, RE0, RE1, RE2, RE3, 
        MD0, MD1, MD2};

    // For the ADC, enum pos will be the pin num on the ADS1115
    enum class ADC1 : uint8_t {SOIL0, SOIL1, SOIL2, SOIL3}; // 4 max
    enum class ADC2 : uint8_t {PHOTO, PH1, PH2, PH3}; // Place holders = PH
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