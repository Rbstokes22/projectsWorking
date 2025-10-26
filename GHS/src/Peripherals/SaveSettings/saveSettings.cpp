#include "Peripherals/saveSettings.hpp"
#include "NVS2/NVS.hpp"
#include "string.h"
#include "Peripherals/TempHum.hpp"
#include "Peripherals/Soil.hpp"
#include "Peripherals/Light.hpp"
#include "Peripherals/Relay.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Network/Handlers/socketHandler.hpp"
#include "UI/MsgLogHandler.hpp"
#include "Peripherals/Alert.hpp"
#include "Common/Timing.hpp"

namespace NVS {

Threads::Mutex settingSaver::mtx(SETTINGS_TAG); // define static var.

settingSaver::settingSaver() : 

    relayKeys{RELAY1_KEY, RELAY2_KEY, RELAY3_KEY, RELAY4_KEY},
    soilKeys{SOIL1_KEY, SOIL2_KEY, SOIL3_KEY, SOIL4_KEY},
    tag(SETTINGS_TAG), logTail(0), restartTime(0),
    nvs(NVS_NAMESPACE_SAVE), expected(0), total(0),
    relays(nullptr) {

    memset(&this->master, 0, sizeof(this->master));
    snprintf(this->log, sizeof(this->log), "%s Ob created", this->tag);
    this->sendErr(this->log, Messaging::Levels::INFO);
}

// Requires messaging and messaging level. Level default to error.
void settingSaver::sendErr(const char* msg, Messaging::Levels lvl) {
    Messaging::MsgLogHandler::get()->handle(lvl, msg, SAVESETTING_ERR_METHOD);
}

// Returns a pointer to the singleton instance.
settingSaver* settingSaver::get() {
    Threads::MutexLock(settingSaver::mtx);
    static settingSaver instance;
    
    return &instance;
}

// No params. Attempts an NVS save for all the peripheral settings. Returns
// true if all items saved, and false if not.
bool settingSaver::save() { 

    // Even though NVS has the functionality, manage all savings here and 
    // only write what is intended to change. All logic within these calls.
    // Separated to ensure all functions are executed.
    bool TH = this->saveTH(); bool LT = this->saveLight();
    bool RE = this->saveRelayTimers(); bool SO = this->saveSoil();

    return TH && LT && RE && SO;
}

// No params. Attempts an NVS read/load for all the peripheral settings. 
// Returns true if all settings loaded, and false if not.
bool settingSaver::load() {

    // Load will load the settings from NVS, and copy them over to the
    // classes. Separated to ensure all functions are executed.
    bool TH = this->loadTH(); bool LT = this->loadLight();
    bool RE = this->loadRelayTimers(); bool SO = this->loadSoil();

    // Read the last logs before the save and restart.
    nvs_ret_t tail = this->nvs.read(LOG_TAIL_KEY, this->logTail, 
        sizeof(this->logTail));

    nvs_ret_t time = this->nvs.read(RESTART_TIME_KEY, this->restartTime, 
        sizeof(this->restartTime));

    // If logs exist, write to current log to review. 3 different logs, done for
    // convienence, the impact is minimal by 2 delimiters only.
    if (tail == nvs_ret_t::NVS_READ_OK && strlen(this->logTail) > 1) {
        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::INFO,
            "PREV TAIL ENTRIES BEGIN", Messaging::Method::SRL_LOG);

        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::INFO,
            this->logTail, Messaging::Method::SRL_LOG, false, true);

        // If time read is OK, add the time restarted to the log entries.
        if (time == nvs_ret_t::NVS_READ_OK && strlen(this->restartTime) > 1) { 
            Messaging::MsgLogHandler::get()->handle(Messaging::Levels::INFO,
            this->restartTime, Messaging::Method::SRL_LOG);
        }

        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::INFO,
            "PREV TAIL ENTRIES END", Messaging::Method::SRL_LOG);
    }

    return TH && LT && RE && SO;
}

// Requires no params. Saves settings, and logs the last 3 entries.
void settingSaver::saveAndRestart() {
    this->save(); // Save settings.
    Clock::DateTime* dtg = Clock::DateTime::get();

    // Next extract the tail of the current log to write to NVS.
    const char* curLog = Messaging::MsgLogHandler::get()->getLog();
    size_t logTailIdx = strlen(curLog); // use strlen to include the null term.

    // Check if the current log is larger than the required pull. Set to the 
    // smaller of the 2.
    size_t moveSize = logTailIdx < LOG_TAIL_SIZE ? logTailIdx : LOG_TAIL_SIZE;
    int delimPos = -2; // Delimiter position init, use to ensure success.

    if (moveSize == logTailIdx) { // Indicates that log is < requested size.
        delimPos = -1; // Allows for extraction of full log.

    } else {
        // Iterate the current log to find the first delim within req size.
        for (int i = logTailIdx - moveSize; i < logTailIdx; i++) {
            if (curLog[i] == MLH_DELIM) {
                delimPos = i; // Set new position to i.
                break;
            }
        }
    }

    if (delimPos != -2) { // Successfully found, populate the logTail.
        memcpy(this->logTail, &curLog[delimPos + 1], moveSize);
        this->logTail[moveSize] = '\0'; // Null term to be safe.
    } else {
        snprintf(this->logTail, strlen(this->logTail) + 1, "NO LOG TAIL");
    }

    // Write the log tail to the NVS to be logged as a load entry upon restart.
    nvs_ret_t err = this->nvs.write(LOG_TAIL_KEY, this->logTail, 
        strlen(this->logTail) + 1);

    if (err != nvs_ret_t::NVS_WRITE_OK) {
        snprintf(this->log, sizeof(this->log), "%s log tail write err", 
            this->tag);

        this->sendErr(this->log);
    }

    // Log the time restarted to include hhmmss as well as sys run time.
    snprintf(this->restartTime, sizeof(this->restartTime), 
        "Restarted @ %u:%u:%u. SysTime sec @ %lu",
        dtg->getTime()->hour, dtg->getTime()->minute, dtg->getTime()->second, 
        dtg->seconds());

    this->nvs.write(RESTART_TIME_KEY, this->restartTime, 
        strlen(this->restartTime) + 1);
    
    if (err != nvs_ret_t::NVS_WRITE_OK) {
        snprintf(this->log, sizeof(this->log), "%s restart time write err", 
            this->tag);

        this->sendErr(this->log);
    }

    esp_restart(); // Restart after log.
}

// Requires relay array to be passed. Array must be global or static to ensure
// it stays within scope.
void settingSaver::initRelays(Peripheral::Relay* relayArray) {
    this->relays = relayArray;
    snprintf(this->log, sizeof(this->log), "%s: Relays init", this->tag);
    this->sendErr(this->log, Messaging::Levels::INFO);
}

}