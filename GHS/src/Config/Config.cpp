#include "Config/config.hpp"
#include <cstdint>
#include "string.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_continuous.h"
#include "UI/MsgLogHandler.hpp"

// PINS
namespace CONF_PINS {

adc_channel_t pinMapA[AnalogPinQty]{
    ADC_CHANNEL_0, ADC_CHANNEL_3, ADC_CHANNEL_6, ADC_CHANNEL_7,
    ADC_CHANNEL_4
};

gpio_num_t pinMapD[DigitalPinQty]{
    GPIO_NUM_16, GPIO_NUM_17, GPIO_NUM_4, GPIO_NUM_27, 
    GPIO_NUM_26, GPIO_NUM_25, GPIO_NUM_33
}; 

adc_oneshot_unit_handle_t adc_unit; // Analog Digital Converter handle

// Requires no params. Configures all digital pins set through the header.
void setupDigitalPins() {
    struct pinConfig {
        DPIN pin; // Digital Pin
        gpio_mode_t IOconfig; 
        gpio_pull_mode_t pullConfig;
    };

    char log[20]{0}; // Used for quick logging.

    // "GPIO_FLOATING" in OUTPUT configuration, has no function within lambda,
    // and is a necessary placeholder.
    pinConfig pins[DigitalPinQty] = {
        {DPIN::WAP, GPIO_MODE_INPUT, GPIO_PULLUP_ONLY},
        {DPIN::STA, GPIO_MODE_INPUT, GPIO_PULLUP_ONLY},
        {DPIN::defWAP, GPIO_MODE_INPUT, GPIO_PULLUP_ONLY},
        {DPIN::RE1, GPIO_MODE_OUTPUT, GPIO_FLOATING},
        {DPIN::RE2, GPIO_MODE_OUTPUT, GPIO_FLOATING},
        {DPIN::RE3, GPIO_MODE_OUTPUT, GPIO_FLOATING},
        {DPIN::RE4, GPIO_MODE_OUTPUT, GPIO_FLOATING}
    };

    // Configures and sets the pins.
    auto pinSet = [&log](pinConfig pin, int i){
        uint8_t pinIndex = static_cast<uint8_t>(pin.pin);
        gpio_num_t pinNum = pinMapD[pinIndex];
        esp_err_t err;

        // Sets direction to to all pins.
        err = gpio_set_direction(pinNum, pin.IOconfig);

        if (err != ESP_OK) { // Handle error indicating dig pin problem.
            snprintf(log, sizeof(log),"DPin %d err", i);
            Messaging::MsgLogHandler::get()->handle(
                Messaging::Levels::CRITICAL,
                log, Messaging::Method::SRL_LOG
            );
        }

        // Filters OUTPUT pins since this sets the pull mode pullup only.
        if (pin.IOconfig != GPIO_MODE_OUTPUT) { 

            // Sets pull mode.
            err = gpio_set_pull_mode(pinNum, pin.pullConfig);

            if (err != ESP_OK) { // Handle error indicating dig pin problem.
            snprintf(log, sizeof(log),"DPin %d err", i);
            Messaging::MsgLogHandler::get()->handle(
                Messaging::Levels::CRITICAL,
                log, Messaging::Method::SRL_LOG
            );
            }
        }
    };

    for (int i = 0; i < DigitalPinQty; i++) {
        pinSet(pins[i], i);
    }
}

// Requires adc unit handle. Configures all analog pins set through the header.
void setupAnalogPins(adc_oneshot_unit_handle_t &unit) {
    adc_oneshot_unit_init_cfg_t unit_cfg{};
    unit_cfg.unit_id = ADC_UNIT_1;
    unit_cfg.ulp_mode = ADC_ULP_MODE_DISABLE;
    char log[20]{0}; // Used for quick logging.

    // register the unit
    esp_err_t err = adc_oneshot_new_unit(&unit_cfg, &unit);

    if (err != ESP_OK) {
            Messaging::MsgLogHandler::get()->handle(
                Messaging::Levels::CRITICAL, "ADC handle err", 
                Messaging::Method::SRL_LOG
            );
    }

    // All analog pins indexes that coresond to ADC channels.
    APIN pins[AnalogPinQty] = {
        APIN::SOIL1, APIN::SOIL2, APIN::SOIL3, APIN::SOIL4, APIN::PHOTO
        };

    // Configure the ADC channel
    adc_oneshot_chan_cfg_t chan_cfg{};
    chan_cfg.bitwidth = ADC_BITWIDTH_DEFAULT,
    chan_cfg.atten = ADC_ATTEN_DB_12;  // Highest voltage reading.

    // sets pins based on configuration passed.
    auto pinSet = [unit, &chan_cfg, &err, &log](APIN pin, int i){
        adc_channel_t pinNum = pinMapA[static_cast<uint8_t>(pin)];

        // Configure the pin channel.
        err = adc_oneshot_config_channel(adc_unit, pinNum, &chan_cfg);

        if (err != ESP_OK) {
            snprintf(log, sizeof(log),"APin %d err", i);
            Messaging::MsgLogHandler::get()->handle(
                Messaging::Levels::CRITICAL,
                log, Messaging::Method::SRL_LOG
            );
        }
    };

    for (int i = 0; i < AnalogPinQty; i++) {
        pinSet(pins[i], i);
    }
}

}

// NETCONFIG EXCLUSIVE
namespace Comms {

// Match the case with the HTML/JS page.
const char* netKeys[static_cast<int>(IDXSIZE::NETCREDKEYQTY)] { 
    "ssid", "pass", "phone", "WAPpass", "APIkey"
};

}

