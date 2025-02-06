#include "Config/config.hpp"
#include <cstdint>
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_continuous.h"

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

adc_oneshot_unit_handle_t adc_unit; // Definition

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

}

// NETCONFIG EXCLUSIVE
namespace Comms {

// Match the case with the HTML/JS page.
const char* netKeys[static_cast<int>(IDXSIZE::NETCREDKEYQTY)] { 
    "ssid", "pass", "phone", "WAPpass", "APIkey"
};

}

