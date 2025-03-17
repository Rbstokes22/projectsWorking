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

namespace NVS {

Threads::Mutex settingSaver::mtx; // define static var.

settingSaver::settingSaver() : 

    relayKeys{RELAY1_KEY, RELAY2_KEY, RELAY3_KEY, RELAY4_KEY},
    soilKeys{SOIL1_KEY, SOIL2_KEY, SOIL3_KEY, SOIL4_KEY},
    tag("(SETTINGS)"),
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
    return this->saveTH() && this->saveRelayTimers() && this->saveSoil() &&
    this->saveLight();
}

// No params. Attempts an NVS read/load for all the peripheral settings. 
// Returns true if all settings loaded, and false if not.
bool settingSaver::load() {
    // Load will load the settings from NVS, and copy them over to the
    // classes.
    return this->loadTH() && this->loadLight() && this->loadRelayTimers() &&
        this->loadSoil();
}

// Requires relay array to be passed. Array must be global or static to ensure
// it stays within scope.
void settingSaver::initRelays(Peripheral::Relay* relayArray) {
    this->relays = relayArray;
    snprintf(this->log, sizeof(this->log), "%s: Relays init", this->tag);
    this->sendErr(this->log, Messaging::Levels::INFO);
}

}