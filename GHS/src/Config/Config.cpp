#include "Config/config.hpp"
#include <cstdint>
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_continuous.h"

adc_channel_t pinMapA[2]{
    ADC_CHANNEL_6, ADC_CHANNEL_7
};

gpio_num_t pinMapD[4]{
    GPIO_NUM_16, GPIO_NUM_17, GPIO_NUM_4, GPIO_NUM_15
};
