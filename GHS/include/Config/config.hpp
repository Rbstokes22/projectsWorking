#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <cstdint>
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_continuous.h"

#define I2C_DEF_FRQ 400000

// OTA Updates
#define FIRMWARE_VERSION "1.0.0"
#define WEBURL "https://major-absolutely-bluejay.ngrok-free.app"
// #define WEBURL "http://shadyside.local:8080"
#define OTA_VERSION_PATH "/checkOTA"
#define LAN_OTA_SIG_PATH "/sigUpdate" 
#define LAN_OTA_FIRMWARE_PATH "/FWUpdate"
#define FIRMWARE_SIZE 1198720
#define FIRMWARE_SIG_SIZE 260

// Web Paths
#define ALERT_PATH "/Alerts"

// Developer mode, This affects certain things like NGROK server.
extern bool DEVmode;

// PINS
#define AnalogPinQty 5
#define DigitalPinQty 8

extern adc_channel_t pinMapA[AnalogPinQty];
extern gpio_num_t pinMapD[DigitalPinQty];

// Used with pinMap above.
enum class DPIN : uint8_t {
    WAP, STA, defWAP, DHT, RE1, RE2, RE3, RE4
};

enum class APIN : uint8_t {
    SOIL1, SOIL2, SOIL3, SOIL4, PHOTO
};

// NET SETUP (default IP is 192.168.1.1).
const char APssid[] = "GreenHouse";
const char mdnsName[] = "greenhouse"; // must be under 11 chars.
const char APdefPass[] = "12345678";
#define credNamespace "netcreds"

// NETCONFIG EXCLUSIVE
namespace Comms {

enum class NetMode {
    WAP, WAP_SETUP, STA, NONE, WAP_RECON
};

enum class Constat {
    CON, DISCON
};

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