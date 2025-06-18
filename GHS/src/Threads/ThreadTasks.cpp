#include "Threads/ThreadTasks.hpp"
#include "Threads/Threads.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Threads/ThreadParameters.hpp"
#include "Config/config.hpp"
#include "Drivers/SHT_Library.hpp"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_continuous.h"
#include "Peripherals/Relay.hpp"
#include "Peripherals/TempHum.hpp"
#include "Peripherals/Soil.hpp"
#include "Peripherals/Light.hpp"
#include "Peripherals/Report.hpp"
#include "UI/MsgLogHandler.hpp"
#include "math.h"
#include "Peripherals/saveSettings.hpp"

namespace ThreadTask {

// requires the tag and high water mark. Each thread routinely calls this to
// ensure that its high water mark is not approaching zero. If LTE to the 
// minimum, a critical log entry will occur.
void highWaterMark(const char* tag, UBaseType_t HWM) {

    // printf("HWM for %s is %u words\n", tag, HWM); // Uncomment for testing.

    if (HWM <= HWM_MIN_WORDS) {
        char msg[128];
        snprintf(msg, sizeof(msg), "%s Thread High Water Mark @ %u words", 
            tag, HWM);

        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::CRITICAL,
            msg, Messaging::Method::SRL_LOG);
    }
}

// Requires the netThreadParams pointer. Responsible for running
// thread dedicated to the network management.
void netTask(void* parameter) { // Runs on 1 second intervals.
    
    if (parameter == nullptr) {
        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::CRITICAL,
            "NET task fail", Messaging::Method::SRL_LOG);
        return;

    } else {
        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::INFO,
            "NET task running", Messaging::Method::SRL_LOG);
    }

    Threads::netThreadParams* params = 
        static_cast<Threads::netThreadParams*>(parameter);

    while (true) {
        // in this portion, check wifi switch for position. 
        params->netManager.handleNet();
        params->netManager.scan();

        highWaterMark("Network", uxTaskGetStackHighWaterMark(NULL));
        vTaskDelay(pdMS_TO_TICKS(params->delay));
    }
}

// Requires the shtThreadParams pointer. Responsible for running
// thread dedicated to the temperature and humidity sensor management.
void SHTTask(void* parameter) { 

    if (parameter == nullptr) {
        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::CRITICAL,
            "SHT task fail", Messaging::Method::SRL_LOG);
        return;
    } else {
        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::INFO,
            "SHT task running", Messaging::Method::SRL_LOG);
    }

    Threads::SHTThreadParams* params = 
        static_cast<Threads::SHTThreadParams*>(parameter);

    // Init here using parameters passed within the thread.
    Peripheral::TempHumParams thParams = {params->SHT};
    Peripheral::TempHum* th = Peripheral::TempHum::get(&thParams);

    while (true) {
        // Only check bounds upon successful read.
        if (th->read()) th->checkBounds(); 
        highWaterMark("TempHum", uxTaskGetStackHighWaterMark(NULL));
        vTaskDelay(pdMS_TO_TICKS(params->delay));
    }
}

// Requires the AS7341ThreadParams pointer. Responsible for running
// thread dedicated to the spectral light sensor management.
void AS7341Task(void* parameter) { // AS7341 & photo Resistor

    if (parameter == nullptr) {
        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::CRITICAL,
            "AS7341 task fail", Messaging::Method::SRL_LOG);
        return;
    } else {
        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::INFO,
            "AS7341 task running", Messaging::Method::SRL_LOG);
    }

    Threads::AS7341ThreadParams* params = 
        static_cast<Threads::AS7341ThreadParams*>(parameter);

    // set channel to the photoresistor pin
    static adc_channel_t channel = 
        CONF_PINS::pinMapA[static_cast<uint8_t>(CONF_PINS::APIN::PHOTO)];

    // Init here using parameters passed within the thread.
    Peripheral::LightParams ltParams = {
        params->adc_unit, channel, params->light
        };

    Peripheral::Light* lt = Peripheral::Light::get(&ltParams);

    while (true) {

        lt->readSpectrum(); // Reads spectrum values
    
        // Checks bounds for photo resistor upon successful read.
        if (lt->readPhoto()) lt->checkBounds();

        highWaterMark("Light", uxTaskGetStackHighWaterMark(NULL));
        vTaskDelay(pdMS_TO_TICKS(params->delay));
    }
}

// Requires the soilThreadParams pointer. Responsible for running
// thread dedicated to the capacitive soil sensor management.
void soilTask(void* parameter) { // Soil sensors

    if (parameter == nullptr) {
        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::CRITICAL,
            "SOIL task fail", Messaging::Method::SRL_LOG);
        return;
    } else {
        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::INFO,
            "SOIL task running", Messaging::Method::SRL_LOG);
    }

    Threads::soilThreadParams* params = 
        static_cast<Threads::soilThreadParams*>(parameter);
    
    // Channels of the ADC to read.
    static adc_channel_t channels[SOIL_SENSORS] = {
        CONF_PINS::pinMapA[static_cast<uint8_t>(CONF_PINS::APIN::SOIL0)],
        CONF_PINS::pinMapA[static_cast<uint8_t>(CONF_PINS::APIN::SOIL1)],
        CONF_PINS::pinMapA[static_cast<uint8_t>(CONF_PINS::APIN::SOIL2)],
        CONF_PINS::pinMapA[static_cast<uint8_t>(CONF_PINS::APIN::SOIL3)]
    };

    // Single soil parameter structure that include 4 channels.
    Peripheral::SoilParams soilParams = {params->adc_unit, channels};

    // Init here to get a singleton class.
    Peripheral::Soil* soil = Peripheral::Soil::get(&soilParams);

    while (true) {
        // Will read all, and then check bounds. Flags are incorporated
        // into the soil readings data, which will prevent action from
        // being taken on a bad read. Unlike temphum, which runs check
        // bounds only if read is good, this does not due to iteration.
        soil->readAll();
        soil->checkBounds();
        highWaterMark("Soil", uxTaskGetStackHighWaterMark(NULL));
        vTaskDelay(pdMS_TO_TICKS(params->delay));
    }
}

// Requires the routineThreadParams pointer. Responsible for running
// thread dedicated to all routine commands, such as managing timers.
void routineTask(void* parameter) {

    if (parameter == nullptr) {
        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::CRITICAL,
            "RTN task fail", Messaging::Method::SRL_LOG);
        return;
    } else {
        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::INFO,
            "RTN task running", Messaging::Method::SRL_LOG);
    }

    Threads::routineThreadParams* params = 
        static_cast<Threads::routineThreadParams*>(parameter);

    // Converts delay in milliseconds to required counts to match the 
    // autosave frequency requirement. 5000ms and frequency of 60 seconds will
    // yield 12. Once count = 12, it will autosave.
    const float autoSaveCts = roundf((1000.0f * AUTO_SAVE_FRQ) / params->delay);
    static size_t count = 0;

    while (true) {
        // Iterate each relay and manage its specific timer
        for (size_t i = 0; i < params->relayQty; i++) {
            params->relays[i].manageTimer(); 
        }

        // Manage the timer of the daily report. If a timer is not enabled,
        // will clear averages at 23:59:50 each day.
        Peripheral::Report::get()->manageTimer();

        // Calls the message check on an interval to ensure that display
        // messages are both sent and cleared from the queue.
        Messaging::MsgLogHandler::get()->OLEDMessageMgr(); 

        // Calls the autosave feature based on it's frequency setting.
        if ((++count) >= autoSaveCts) { // Increments count when checking.
            NVS::settingSaver::get()->save();
            count = 0; // Reset count.
        }

        highWaterMark("Routine", uxTaskGetStackHighWaterMark(NULL));
        vTaskDelay(pdMS_TO_TICKS(params->delay));
    }
}

}