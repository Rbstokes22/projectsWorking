#include "Threads/ThreadTasks.hpp"
#include "Threads/Threads.hpp"
#include "Threads/ThreadParameters.hpp"
#include "Config/config.hpp"
#include "Drivers/SHT_Library.hpp"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_continuous.h"
#include "Peripherals/Relay.hpp"
#include "Peripherals/TempHum.hpp"
#include "Peripherals/Soil.hpp"
#include "Peripherals/Report.hpp"

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

// CREATE A SEPARATE CLASS HERE EXCLUSIVE TO THE SHT SETTINGS.
void SHTTask(void* parameter) { // SHT
    Threads::SHTThreadParams* params = 
        static_cast<Threads::SHTThreadParams*>(parameter);

    Peripheral::TempHumParams thParams = {params->msglogerr, params->SHT};
    Peripheral::TempHum* th = Peripheral::TempHum::get(&thParams);

    while (true) {
        // Only check bounds upon successful read.
        if (th->read()) th->checkBounds(); // Read. Comment out when testing.
        vTaskDelay(pdMS_TO_TICKS(params->delay));
    }
}

void AS7341Task(void* parameter) { // AS7341, photo Resistor
    Threads::AS7341ThreadParams* params = 
        static_cast<Threads::AS7341ThreadParams*>(parameter);

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

void routineTask(void* parameter) {
    Threads::routineThreadParams* params = 
        static_cast<Threads::routineThreadParams*>(parameter);

    while (true) {
        // Iterate each relay and manage its specific timer
        for (size_t i = 0; i < params->relayQty; i++) {
            params->relays[i].manageTimer(); // Acquires first control ID
        }

        // Manage the timer of the daily report.
        Peripheral::Report::get()->manageTimer();

        vTaskDelay(pdMS_TO_TICKS(params->delay));
    }
}

}