#include "Threads/ThreadTasks.hpp"
#include "Threads/Threads.hpp"
#include "Threads/ThreadParameters.hpp"
#include "Config/config.hpp"
#include "Drivers/DHT_Library.hpp"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_continuous.h"
#include "Peripherals/Relay.hpp"
#include "Peripherals/TempHum.hpp"
#include "Peripherals/Soil.hpp"

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

    size_t errCt{0};
    size_t errCtMax{5};
    float temp{0.0f}, hum{0.0f};

    Peripheral::TempHumParams thParams = {params->msglogerr};
    Peripheral::TempHum* th = Peripheral::TempHum::get(&thParams);

    while (true) {
        bool read = params->dht.read(temp, hum);

        if (read) {
            th->setTemp(temp);
            th->setHum(hum);
            th->setStatus(true);
            errCt = 0;
        } else {
            errCt++;
        }

        if (errCt >= errCtMax) { // Handles if the device is up or down.
            th->setStatus(false);
            errCt = 0;
        }

        th->checkBounds();

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
    
    static adc_channel_t channels[SOIL_SENSORS] = {
        pinMapA[static_cast<uint8_t>(APIN::SOIL1)],
        pinMapA[static_cast<uint8_t>(APIN::SOIL2)],
        pinMapA[static_cast<uint8_t>(APIN::SOIL3)],
        pinMapA[static_cast<uint8_t>(APIN::SOIL4)]
    };

    Peripheral::SoilParams soilParams = {
        params->msglogerr,
        params->adc_unit,
        channels
    };

    // Init here to get a singleton class
    Peripheral::Soil* soil = Peripheral::Soil::get(&soilParams);

    while (true) {
        soil->readAll();
        soil->checkBounds();
        vTaskDelay(pdMS_TO_TICKS(params->delay));
    }
}

void relayTask(void* parameter) {
    Threads::relayThreadParams* params = 
        static_cast<Threads::relayThreadParams*>(parameter);

    while (true) {
        for (size_t i = 0; i < params->relayQty; i++) {
            params->relays[i].manageTimer();
        }

        vTaskDelay(pdMS_TO_TICKS(params->delay));
    }
}

}