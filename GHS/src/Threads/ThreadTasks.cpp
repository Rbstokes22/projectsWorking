#include "Threads/ThreadTasks.hpp"
#include "Threads/Threads.hpp"
#include "Threads/ThreadParameters.hpp"
#include "Config/config.hpp"
#include "Drivers/DHT_Library.hpp"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_continuous.h"

namespace ThreadTask {

void netTask(void* parameter) { // Runs on 1 second intervals.
    Threads::netThreadParams* params = 
        static_cast<Threads::netThreadParams*>(parameter);

    #define LOCK_NET params->mutex.lock()
    #define UNLOCK_NET params->mutex.unlock();

    while (true) {
        // in this portion, check wifi switch for position. 
        params->netManager.handleNet();
        params->msglogerr.OLEDMessageCheck(); // clears errors from display

        vTaskDelay(pdMS_TO_TICKS(params->delay));
    }
}

// CREATE A SEPARATE CLASS HERE EXCLUSIVE TO THE DHT SETTINGS.
void DHTTask(void* parameter) { // DHT
    Threads::DHTThreadParams* params = 
        static_cast<Threads::DHTThreadParams*>(parameter);

    #define LOCK_DHT params->mutex.lock();
    #define UNLOCK_DHT params->mutex.unlock();

    // float temp{0}, hum{0};
    while (true) {
        // bool read = params->dht.read(temp, hum);

        // if (read) {
        //     printf("Temp: %.2f, Hum: %.2f\n", temp, hum);
        // }

        vTaskDelay(pdMS_TO_TICKS(params->delay));
    }
}

void AS7341Task(void* parameter) { // AS7341, photo Resistor
    Threads::AS7341ThreadParams* params = 
        static_cast<Threads::AS7341ThreadParams*>(parameter);

    #define LOCK_AS7341 params->mutex.lock();
    #define UNLOCK_AS7341 params->mutex.unlock();

    while (true) {

        vTaskDelay(pdMS_TO_TICKS(params->delay));
    }
}

void soilTask(void* parameter) { // Soil sensors
    Threads::soilThreadParams* params = 
        static_cast<Threads::soilThreadParams*>(parameter);

    #define LOCK_SOIL params->mutex.lock();
    #define UNLOCK_SOIL params->mutex.unlock();

    int soil{0};
    adc_channel_t soilCh = pinMapA[static_cast<uint8_t>(APIN::SOIL1)];

    while (true) {

        adc_oneshot_read(params->adc_unit, soilCh, &soil);
        printf("Soil: %d\n", soil);


        vTaskDelay(pdMS_TO_TICKS(params->delay));
    }
}

}