#include "Config/config.hpp"
#include <cstdint>
#include "string.h"
#include "driver/gpio.h"
#include "UI/MsgLogHandler.hpp"

char confLog[50]{0}; // Reusable log. 50 should be plenty big.
const char* whiteListDomains[3] = {WEBURL, LOCAL_IP, MDNS_ACTUAL};

// PINS
namespace CONF_PINS {

gpio_num_t pinMapD[DigitalPinQty]{
    GPIO_NUM_16, GPIO_NUM_17, GPIO_NUM_4, GPIO_NUM_27, 
    GPIO_NUM_26, GPIO_NUM_25, GPIO_NUM_33, GPIO_NUM_19, GPIO_NUM_18, GPIO_NUM_5
}; 

// Requires no params. Configures all digital pins set through the header.
void setupDigitalPins() {
    struct pinConfig {
        DPIN pin; // Digital Pin
        gpio_mode_t IOconfig; 
        gpio_pull_mode_t pullConfig;
    };

    // "GPIO_FLOATING" in OUTPUT configuration, has no function within lambda,
    // and is a necessary placeholder.
    pinConfig pins[DigitalPinQty] = {
        {DPIN::WAP, GPIO_MODE_INPUT, GPIO_PULLUP_ONLY},
        {DPIN::STA, GPIO_MODE_INPUT, GPIO_PULLUP_ONLY},
        {DPIN::defWAP, GPIO_MODE_INPUT, GPIO_PULLUP_ONLY},
        {DPIN::RE0, GPIO_MODE_OUTPUT, GPIO_FLOATING},
        {DPIN::RE1, GPIO_MODE_OUTPUT, GPIO_FLOATING},
        {DPIN::RE2, GPIO_MODE_OUTPUT, GPIO_FLOATING},
        {DPIN::RE3, GPIO_MODE_OUTPUT, GPIO_FLOATING},
        {DPIN::MD0, GPIO_MODE_INPUT, GPIO_PULLUP_ONLY},
        {DPIN::MD1, GPIO_MODE_INPUT, GPIO_PULLUP_ONLY},
        {DPIN::MD2, GPIO_MODE_INPUT, GPIO_PULLUP_ONLY}
    };

    // Configures and sets the pins.
    auto pinSet = [](pinConfig pin, int i){
        uint8_t pinIndex = static_cast<uint8_t>(pin.pin);
        gpio_num_t pinNum = pinMapD[pinIndex];
        esp_err_t err;

        // Sets direction to to all pins.
        err = gpio_set_direction(pinNum, pin.IOconfig);

        if (err != ESP_OK) { // Handle error indicating dig pin problem.
            snprintf(confLog, sizeof(confLog),"%s DPin %d err", CONFTAG, i);
            Messaging::MsgLogHandler::get()->handle(
                Messaging::Levels::CRITICAL,
                confLog, Messaging::Method::SRL_LOG
            );
        }

        // Filters OUTPUT pins since this sets the pull mode pullup only.
        if (pin.IOconfig != GPIO_MODE_OUTPUT) { 

            // Sets pull mode.
            err = gpio_set_pull_mode(pinNum, pin.pullConfig);

            if (err != ESP_OK) { // Handle error indicating dig pin problem.
            snprintf(confLog, sizeof(confLog),"%s DPin %d err", CONFTAG, i);
            Messaging::MsgLogHandler::get()->handle(
                Messaging::Levels::CRITICAL,
                confLog, Messaging::Method::SRL_LOG
            );
            }
        }
    };

    for (int i = 0; i < DigitalPinQty; i++) {
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

