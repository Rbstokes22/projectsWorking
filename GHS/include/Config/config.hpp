#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <cstdint>
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_continuous.h"

#define I2C_DEF_FRQ 400000

// OTA Updates
#define FIRMWARE_VERSION "1.0.4"
#define WEBURL "https://major-absolutely-bluejay.ngrok-free.app"
#define FIRMWARE_SIZE 1184704
#define FIRMWARE_SIG_SIZE 260

// Pin quantities
#define AnalogPinQty 5
#define DigitalPinQty 8

extern bool DEVmode;

extern adc_channel_t pinMapA[AnalogPinQty];
extern gpio_num_t pinMapD[DigitalPinQty];

// Used with pinMap above.
enum class DPIN : uint8_t {
    WAP, STA, defWAP, DHT, RE1, RE2, RE3, RE4
};

enum class APIN : uint8_t {
    SOIL1, SOIL2, SOIL3, SOIL4, PHOTO
};

#endif // CONFIG_HPP