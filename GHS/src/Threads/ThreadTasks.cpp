#include "Threads/ThreadTasks.hpp"
#include "Threads/Threads.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Threads/ThreadParameters.hpp"
#include "Config/config.hpp"
#include "Drivers/SHT_Library.hpp"
#include "driver/gpio.h"
#include "Peripherals/Relay.hpp"
#include "Peripherals/TempHum.hpp"
#include "Peripherals/Soil.hpp"
#include "Peripherals/Light.hpp"
#include "Peripherals/Report.hpp"
#include "UI/MsgLogHandler.hpp"
#include "math.h"
#include "Peripherals/saveSettings.hpp"
#include "Common/heartbeat.hpp"
#include "Drivers/ADC.hpp"

namespace ThreadTask {

// requires the tag and high water mark. Each thread routinely calls this to
// ensure that its high water mark is not approaching zero. If LTE to the 
// minimum, a critical log entry will occur.
void highWaterMark(const char* tag, UBaseType_t HWM) {
    static char msg[128];

    if (HWM <= HWM_MIN_WORDS) {
        
        snprintf(msg, sizeof(msg), "%s Thread High Water Mark @ %u words", 
            tag, HWM);

        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::CRITICAL,
            msg, Messaging::Method::SRL_LOG, true, false);
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

    // Convert ms delay to to ticks.
    const TickType_t period = pdMS_TO_TICKS(params->delay);

    // Init once. This will be updated by the scheduler each time it is passed
    // as reference to vTaskDelayUntil(&last, period).
    TickType_t last = xTaskGetTickCount();

    // Register task with heartbeat.
    uint8_t HBID = heartbeat::Heartbeat::get()->getBlockID("NET", HB_DELAY);

    while (true) {

        // Tick counts used in loop are exclusively for logging when a task
        // exceeds its projected delay, since we are trying to maintain an
        // absolute vs relative delay with the scheduler.
        TickType_t t_0 = xTaskGetTickCount(); // Run before work.

        // in this portion, check wifi switch for position. 
        params->netManager.handleNet();

        // Pass heartbeat ID to scan because if a low RSSI is detected, it will
        // block the network and scan
        params->netManager.scan(HBID, NET_SCAN_HEARTBEAT);

        // Check in to reset heart beat expiration.
        heartbeat::Heartbeat::get()->rogerUp(HBID, NET_HEARTBEAT);

        highWaterMark("Network", uxTaskGetStackHighWaterMark(NULL));

        TickType_t t_f = xTaskGetTickCount(); // Run after work.
        TickType_t work = t_f - t_0; // work count, final - initial.

        // Log overruns to ensure that works tasks are not exceeding count.
        if (work > period) {
            char buf[96];
            snprintf(buf, sizeof(buf), 
                "Net overrun: Work (%lu)ms exceeds period (%lu)ms",
                (work * portTICK_PERIOD_MS), (period * portTICK_PERIOD_MS));

            Messaging::MsgLogHandler::get()->handle(Messaging::Levels::WARNING,
                buf, Messaging::Method::SRL_LOG
            );
        }

        vTaskDelayUntil(&last, period); // Sets absolute delay, vs relative.
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

    // Convert ms delay to to ticks.
    const TickType_t period = pdMS_TO_TICKS(params->delay);

    // Init once. This will be updated by the scheduler each time it is passed
    // as reference to vTaskDelayUntil(&last, period).
    TickType_t last = xTaskGetTickCount();

    // Register task with heartbeat.
    uint8_t HBID = heartbeat::Heartbeat::get()->getBlockID("TEMPHUM", HB_DELAY);

    while (true) {

        // Tick counts used in loop are exclusively for logging when a task
        // exceeds its projected delay, since we are trying to maintain an
        // absolute vs relative delay with the scheduler.
        TickType_t t_0 = xTaskGetTickCount(); // Run before work.

        // Only check bounds upon successful read.
        if (th->read()) th->checkBounds(); 

        // Check in to reset heart beat expiration.
        heartbeat::Heartbeat::get()->rogerUp(HBID, TEMPHUM_HEARTBEAT);

        highWaterMark("TempHum", uxTaskGetStackHighWaterMark(NULL));

        TickType_t t_f = xTaskGetTickCount(); // Run after work.
        TickType_t work = t_f - t_0; // work count, final - initial.

        // Log overruns to ensure that works tasks are not exceeding count.
        if (work > period) {
            char buf[96];
            snprintf(buf, sizeof(buf), 
                "SHT overrun: Work (%lu)ms exceeds period (%lu)ms",
                (work * portTICK_PERIOD_MS), (period * portTICK_PERIOD_MS));

            Messaging::MsgLogHandler::get()->handle(Messaging::Levels::WARNING,
                buf, Messaging::Method::SRL_LOG
            );
        }

        vTaskDelayUntil(&last, period); // Sets absolute delay, vs relative.
    }
}

// Requires the AS7341ThreadParams pointer. Responsible for running
// thread dedicated to the spectral light sensor management.
void LightTask(void* parameter) { // AS7341 & photo Resistor

    if (parameter == nullptr) {
        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::CRITICAL,
            "AS7341 task fail", Messaging::Method::SRL_LOG);
        return;
    } else {
        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::INFO,
            "AS7341 task running", Messaging::Method::SRL_LOG);
    }

    Threads::LightThreadParams* params = 
        static_cast<Threads::LightThreadParams*>(parameter);

    // Init here using parameters passed within the thread.
    Peripheral::LightParams ltParams = {params->photo, params->light};

    Peripheral::Light* lt = Peripheral::Light::get(&ltParams);

    // Convert ms delay to to ticks.
    const TickType_t period = pdMS_TO_TICKS(params->delay);

    // Init once. This will be updated by the scheduler each time it is passed
    // as reference to vTaskDelayUntil(&last, period).
    TickType_t last = xTaskGetTickCount();

     // Register task with heartbeat.
    uint8_t HBID = heartbeat::Heartbeat::get()->getBlockID("LIGHT", HB_DELAY);

    while (true) {

        // Tick counts used in loop are exclusively for logging when a task
        // exceeds its projected delay, since we are trying to maintain an
        // absolute vs relative delay with the scheduler.
        TickType_t t_0 = xTaskGetTickCount(); // Run before work.

        lt->readSpectrum(); // Reads spectrum values
    
        // Checks bounds for photo resistor upon successful read.
        if (lt->readPhoto()) lt->checkBounds();

        // Check in to reset heart beat expiration.
        heartbeat::Heartbeat::get()->rogerUp(HBID, LIGHT_HEARTBEAT);

        highWaterMark("Light", uxTaskGetStackHighWaterMark(NULL));

        TickType_t t_f = xTaskGetTickCount(); // Run after work.
        TickType_t work = t_f - t_0; // work count, final - initial.

        // Log overruns to ensure that works tasks are not exceeding count.
        if (work > period) {
            char buf[96];
            snprintf(buf, sizeof(buf), 
                "Light overrun: Work (%lu)ms exceeds period (%lu)ms",
                (work * portTICK_PERIOD_MS), (period * portTICK_PERIOD_MS));

            Messaging::MsgLogHandler::get()->handle(Messaging::Levels::WARNING,
                buf, Messaging::Method::SRL_LOG
            );
        }

        vTaskDelayUntil(&last, period); // Sets absolute delay, vs relative.
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
    
    // Single soil parameter structure that include ADC1 driver.
    Peripheral::SoilParams soilParams = {params->soil};

    // Init here to get a singleton class.
    Peripheral::Soil* soil = Peripheral::Soil::get(&soilParams);

    // Convert ms delay to to ticks.
    const TickType_t period = pdMS_TO_TICKS(params->delay);

    // Init once. This will be updated by the scheduler each time it is passed
    // as reference to vTaskDelayUntil(&last, period).
    TickType_t last = xTaskGetTickCount();

     // Register task with heartbeat.
    uint8_t HBID = heartbeat::Heartbeat::get()->getBlockID("SOIL", HB_DELAY);

    while (true) {

        // Tick counts used in loop are exclusively for logging when a task
        // exceeds its projected delay, since we are trying to maintain an
        // absolute vs relative delay with the scheduler.
        TickType_t t_0 = xTaskGetTickCount(); // Run before work.

        // Will read all, and then check bounds. Flags are incorporated
        // into the soil readings data, which will prevent action from
        // being taken on a bad read. Unlike temphum, which runs check
        // bounds only if read is good, this does not due to iteration.
        soil->readAll();
        soil->checkBounds();

        // Check in to reset heart beat expiration.
        heartbeat::Heartbeat::get()->rogerUp(HBID, SOIL_HEARTBEAT);

        highWaterMark("Soil", uxTaskGetStackHighWaterMark(NULL));

        TickType_t t_f = xTaskGetTickCount(); // Run after work.
        TickType_t work = t_f - t_0; // work count, final - initial.

        // Log overruns to ensure that works tasks are not exceeding count.
        if (work > period) {
            char buf[96];
            snprintf(buf, sizeof(buf), 
                "Soil overrun: Work (%lu)ms exceeds period (%lu)ms",
                (work * portTICK_PERIOD_MS), (period * portTICK_PERIOD_MS));

            Messaging::MsgLogHandler::get()->handle(Messaging::Levels::WARNING,
                buf, Messaging::Method::SRL_LOG
            );
        }

        vTaskDelayUntil(&last, period); // Sets absolute delay, vs relative.
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

    // Convert ms delay to to ticks.
    const TickType_t period = pdMS_TO_TICKS(params->delay);

    // Init once. This will be updated by the scheduler each time it is passed
    // as reference to vTaskDelayUntil(&last, period).
    TickType_t last = xTaskGetTickCount();

     // Register task with heartbeat.
    uint8_t HBID = heartbeat::Heartbeat::get()->getBlockID("ROUTINE", HB_DELAY);

    while (true) {

        // Tick counts used in loop are exclusively for logging when a task
        // exceeds its projected delay, since we are trying to maintain an
        // absolute vs relative delay with the scheduler.
        TickType_t t_0 = xTaskGetTickCount(); // Run before work.

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

        // Check in to reset heart beat expiration.
        heartbeat::Heartbeat::get()->rogerUp(HBID, ROUTINE_HEATBEAT);

        // Call at a 1 hz frequency to properly employ.
        heartbeat::Heartbeat::get()->manage();

        highWaterMark("Routine", uxTaskGetStackHighWaterMark(NULL));
        
        TickType_t t_f = xTaskGetTickCount(); // Run after work.
        TickType_t work = t_f - t_0; // work count, final - initial.

        // Log overruns to ensure that works tasks are not exceeding count.
        if (work > period) {
            char buf[96];
            snprintf(buf, sizeof(buf), 
                "Rtn overrun: Work (%lu)ms exceeds period (%lu)ms",
                (work * portTICK_PERIOD_MS), (period * portTICK_PERIOD_MS));

            Messaging::MsgLogHandler::get()->handle(Messaging::Levels::WARNING,
                buf, Messaging::Method::SRL_LOG
            );
        }

        vTaskDelayUntil(&last, period); // Sets absolute delay, vs relative.
    }
}

}