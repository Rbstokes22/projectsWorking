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

extern bool DEVmode;

extern adc_channel_t pinMapA[2];
extern gpio_num_t pinMapD[5];

// Used with pinMap above.
enum class DPIN : uint8_t {
    WAP, STA, defWAP, DHT, RE1
};

enum class APIN : uint8_t {
    SOIL1, PHOTO
};

#endif // CONFIG_HPP