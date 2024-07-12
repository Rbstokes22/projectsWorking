// TO do, finish library for ssd, Build Display class as well as msglogerr.

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "config.hpp"
#include "Drivers/SSD1306_Library.hpp"
#include "UI/Display.hpp"
#include "UI/MsgLogHandler.hpp"
#include "Threads/Threads.hpp"
#include "Threads/ThreadParameters.hpp"

extern "C" {
    void app_main();
}

// ALL OBJECTS
UI::Display OLED;
Messaging::MsgLogHandler msglogerr(OLED, 5, true);

// ALL THREADS
Threads::Thread mainThread(msglogerr);

void continuousTask(void* parameter) {
    Threads::mainThreadParams* params = static_cast<Threads::mainThreadParams*>(parameter);
    #define LOCK params->mutex.lock()
    #define UNLOCK params->mutex.unlock();

    while (true) {

        msglogerr.OLEDMessageCheck(); // clears errors from display
        
        vTaskDelay(params->delay / portTICK_PERIOD_MS);

    }
}

void setupDigitalPins() {
    ESP_ERROR_CHECK(gpio_set_direction(pinMapD[static_cast<int>(DPIN::WAP)], GPIO_MODE_INPUT));
    ESP_ERROR_CHECK(gpio_set_direction(pinMapD[static_cast<int>(DPIN::STA)], GPIO_MODE_INPUT));
    ESP_ERROR_CHECK(gpio_set_direction(pinMapD[static_cast<int>(DPIN::defWAP)], GPIO_MODE_INPUT));
    ESP_ERROR_CHECK(gpio_set_direction(pinMapD[static_cast<int>(DPIN::RE1)], GPIO_MODE_OUTPUT));

    // Configure all pins 
    ESP_ERROR_CHECK(gpio_set_pull_mode(pinMapD[static_cast<int>(DPIN::WAP)], GPIO_PULLUP_ONLY));
    ESP_ERROR_CHECK(gpio_set_pull_mode(pinMapD[static_cast<int>(DPIN::STA)], GPIO_PULLUP_ONLY));
    ESP_ERROR_CHECK(gpio_set_pull_mode(pinMapD[static_cast<int>(DPIN::defWAP)], GPIO_PULLUP_ONLY));
}

void setupAnalogPins() {
    adc_oneshot_unit_handle_t adc_unit;
    adc_oneshot_unit_init_cfg_t unit_cfg{};
    unit_cfg.unit_id = ADC_UNIT_1;
    unit_cfg.ulp_mode = ADC_ULP_MODE_DISABLE;

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc_unit));
    
    // Configure the ADC channel
    adc_oneshot_chan_cfg_t chan_cfg{};
    chan_cfg.bitwidth = ADC_BITWIDTH_DEFAULT,
    chan_cfg.atten = ADC_ATTEN_DB_12;  // Highest voltage reading.

    ESP_ERROR_CHECK(adc_oneshot_config_channel(
        adc_unit, pinMapA[static_cast<int>(APIN::SOIL1)], &chan_cfg));

    ESP_ERROR_CHECK(adc_oneshot_config_channel(
        adc_unit, pinMapA[static_cast<int>(APIN::PHOTO)], &chan_cfg));
}

void app_main() {
    setupDigitalPins();
    setupAnalogPins();
    OLED.init(0x3C);

    Threads::mainThreadParams mainParams(2000, msglogerr);
    mainThread.initThread(continuousTask, "MainLoop", 2048, &mainParams, 5);
    

}

