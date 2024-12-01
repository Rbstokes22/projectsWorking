#include "Config/config.hpp"
#include <cstdint>
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_continuous.h"

// Used in the STAOTA handler to send NGROK headers.
bool DEVmode = true; // false = in production

adc_channel_t pinMapA[AnalogPinQty]{
    ADC_CHANNEL_0, ADC_CHANNEL_3, ADC_CHANNEL_6, ADC_CHANNEL_7,
    ADC_CHANNEL_4
};

gpio_num_t pinMapD[DigitalPinQty]{
    GPIO_NUM_16, GPIO_NUM_17, GPIO_NUM_4, GPIO_NUM_27, 
    GPIO_NUM_26, GPIO_NUM_25, GPIO_NUM_33
}; 

// NETCONFIG EXCLUSIVE
namespace Comms {

// Match the case with the HTML/JS page.
const char* netKeys[static_cast<int>(IDXSIZE::NETCREDKEYQTY)] { 
    "ssid", "pass", "phone", "WAPpass", "APIkey"
};

}

