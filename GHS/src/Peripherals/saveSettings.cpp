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

namespace NVS {

Threads::Mutex settingSaver::mtx; // define static var.

settingSaver::settingSaver() : 

    relayKeys{RELAY1_KEY, RELAY2_KEY, RELAY3_KEY, RELAY4_KEY},
    soilKeys{SOIL1_KEY, SOIL2_KEY, SOIL3_KEY, SOIL4_KEY},
    tag("NVS Settings"),
    nvs(NVS_NAMESPACE_SAVE), expected(0), total(0),
    relays(nullptr) {

    memset(this->log, 0, sizeof(this->log));
    memset(&this->master, 0, sizeof(this->master));
}

// Requires no params. Copies the current temp/hum settings to the master
// struct, and then writes to NVS if settings have changed since last save.
void settingSaver::saveTH() {
    expected = 5; // Expected true values (5 per device)
    total = 0; // accumulation of true values

    // Do temperature followed by humidity. Compare current values with the
    // master settings.
    Peripheral::TH_TRIP_CONFIG* th = Peripheral::TempHum::get()->getTempConf();
    total += this->compare(this->master.temp.relayNum, th->relay.num);
    total += this->compare(this->master.temp.relayCond, th->relay.condition);
    total += this->compare(this->master.temp.altCond,th->alt.condition);
    total += this->compare(this->master.temp.relayTripVal, th->relay.tripVal);
    total += this->compare(this->master.temp.altTripVal, th->alt.tripVal);

    if (total != expected) { // If changes are detected, rewrite to NVS.
        this->err = nvs.write(TEMP_KEY, &this->master.temp, 
            sizeof(this->master.temp));

        vTaskDelay(pdMS_TO_TICKS(NVS_SETTING_DELAY));

        if (err != nvs_ret_t::NVS_WRITE_OK) { // Handle bad write
            snprintf(this->log, sizeof(this->log), "%s: Temp not written", tag);
            this->sendLog();
        } 
    } 

    total = 0; // Reset for humidity.
    
    // Change pointer to humidity to compare
    th = Peripheral::TempHum::get()->getHumConf();
    total += this->compare(this->master.hum.relayNum, th->relay.num);
    total += this->compare(this->master.hum.relayCond, th->relay.condition);
    total += this->compare(this->master.hum.altCond, th->alt.condition);
    total += this->compare(this->master.hum.relayTripVal, th->relay.tripVal);
    total += this->compare(this->master.hum.altTripVal, th->alt.tripVal);

    if (total != expected) { // If changes are detected, rewrite to NVS.
        this->err = nvs.write(HUM_KEY, &this->master.hum, 
            sizeof(this->master.hum));

        vTaskDelay(pdMS_TO_TICKS(NVS_SETTING_DELAY));

        if (err != nvs_ret_t::NVS_WRITE_OK) { // Handle bad write
            snprintf(this->log, sizeof(this->log), "%s: Hum not written", tag);
            this->sendLog();
        } 
    } 
}

// Requires no params. Iterates each relay and copies the current settings to
// the master struct, and write to NVS if settings have changed since last save.
void settingSaver::saveRelayTimers() {
    
    if (this->relays == nullptr) { // ensures the relays have been init.
        snprintf(this->log, sizeof(this->log), "%s: Relay is nullptr", tag);
        this->sendLog();
        return;
    }

    for (int i = 0; i < TOTAL_RELAYS; i++) { // Iterate each relay.
        expected = 2; // Expected true values 
        total = 0; // accumulation of true values

        total += this->compare(this->master.relays[i].onTime,
            this->relays[i].getTimer()->onTime);

        total += this->compare(this->master.relays[i].offTime,
            this->relays[i].getTimer()->offTime);

        if (total != expected) { // If changes are detected, rewrite to NVS.
            this->err = nvs.write(this->relayKeys[i], &this->master.relays[i], 
                sizeof(this->master.relays[i]));

            vTaskDelay(pdMS_TO_TICKS(NVS_SETTING_DELAY));

            if (this->err != nvs_ret_t::NVS_WRITE_OK) {
                snprintf(this->log, sizeof(this->log), 
                    "%s: Relay %d not written", tag, i);

                this->sendLog();
            }
        } 
    }
}

// Requires no params. Iterates each sensor and copies the current settings to
// the master struct, and write to NVS if settings have changed since last save.
void settingSaver::saveSoil() {

    for (int i = 0; i < SOIL_SENSORS; i++) { // Iterate each sensor
        expected = 2; // Expected true values 
        total = 0; // accumulation of true values

        Peripheral::AlertConfigSo* so = Peripheral::Soil::get()->getConfig(i);
        total += this->compare(this->master.soil[i].tripVal, so->tripVal);
        total += this->compare(this->master.soil[i].cond, so->condition);

        if (total != expected) { // If changes are detected, rewrite to NVS.
            this->err = nvs.write(this->soilKeys[i], &this->master.soil[i], 
            sizeof(this->master.soil[i]));
  
            vTaskDelay(pdMS_TO_TICKS(NVS_SETTING_DELAY));

            if (this->err != nvs_ret_t::NVS_WRITE_OK) {
                snprintf(this->log, sizeof(this->log), 
                    "%s: Soil %d not written", tag, i);

                this->sendLog();
            }
        } 
    }
}

// Requires no params. Copies the current light settings to the master
// struct, and then writes to NVS if settings have changed since last save.
void settingSaver::saveLight() {
    expected = 4;
    total = 0;

    Peripheral::RelayConfigLight* lt = Peripheral::Light::get()->getConf();
    total += this->compare(this->master.light.relayNum, lt->num);
    total += this->compare(this->master.light.relayCond, lt->condition);
    total += this->compare(this->master.light.relayTripVal, lt->tripVal);
    total += this->compare(this->master.light.darkVal, lt->darkVal);

    if (total != expected) {
        this->err = nvs.write(LIGHT_KEY, &this->master.light, 
            sizeof(this->master.light));

        vTaskDelay(pdMS_TO_TICKS(NVS_SETTING_DELAY));

        snprintf(this->log, sizeof(this->log), "%s: Light not written", tag);
        this->sendLog();
    } 
}

// Requires no params. Reads the temp/hum settings from the NVS, and copies
// setting data over to the temp/hum current configuration structs.
void settingSaver::loadTH() {

    // Copies data from both the temp and hum to current settings. 
    auto copy = [](Peripheral::TH_TRIP_CONFIG* to, configSaveReAltTH &from) {

        if (from.relayCond != Peripheral::RECOND::NONE) { 
            // If relay condition is set, copy the condition and value.
            to->relay.condition = from.relayCond;
            to->relay.tripVal = from.relayTripVal;
        }

        if (from.altCond != Peripheral::ALTCOND::NONE) { 
            // If alert condition is set, copy the condition and value.
            to->alt.condition = from.altCond;
            to->alt.tripVal = from.altTripVal;
        }

        // If relay was set, attaches relay to the temp/hum settings.
        if (from.relayNum < TOTAL_RELAYS) { 
            Comms::SOCKHAND::attachRelayTH(from.relayNum, to);
        }
    };

    // Read the temperature struct from the NVS.
    nvs_ret_t load = this->nvs.read(TEMP_KEY, &this->master.temp, 
        sizeof(this->master.temp));
    vTaskDelay(pdMS_TO_TICKS(NVS_SETTING_DELAY));

    // Load the values from NVS to the temperature config first.
    Peripheral::TH_TRIP_CONFIG* th = Peripheral::TempHum::get()->getTempConf();

    // If successful read, load and copy the data over to the temp config.
    if (load == nvs_ret_t::NVS_READ_OK) {
        copy(th, this->master.temp);
    } else {
        snprintf(this->log, sizeof(this->log), "%s: Temp not read", tag);
        this->sendLog();
    }

    // Repeat for humidity.
    load = this->nvs.read(HUM_KEY, &this->master.hum, sizeof(this->master.hum));
    vTaskDelay(pdMS_TO_TICKS(NVS_SETTING_DELAY));

    th = Peripheral::TempHum::get()->getHumConf();

    if (load == nvs_ret_t::NVS_READ_OK) {
        copy(th, this->master.hum);
    } else {
        snprintf(this->log, sizeof(this->log), "%s: Hum not read", tag);
        this->sendLog();
    }
}

// Requires no params. Iterates each relay, reads its settings from the
// NVS, and copies setting data over to the appropriate relay current 
// configuration structure.
void settingSaver::loadRelayTimers() {

    for (int i = 0; i < TOTAL_RELAYS; i++) { // Iterate each relay config.

        nvs_ret_t load = this->nvs.read(this->relayKeys[i], 
            &this->master.relays[i], sizeof(this->master.relays[i]));

        vTaskDelay(pdMS_TO_TICKS(NVS_SETTING_DELAY));

        if (load != nvs_ret_t::NVS_READ_OK) { 
            snprintf(this->log, sizeof(this->log), "%s: Timer %d not read", 
                tag, i);

            this->sendLog();
            continue; // skip copy if data is bad.
        }

        // If good, and the relay on and off times are appropriate meaning they
        // do not equal 99999 or eachother, the settings are copied over.
        if (this->master.relays[i].onTime != RELAY_TIMER_OFF &&
            this->master.relays[i].offTime != RELAY_TIMER_OFF &&
            this->master.relays[i].onTime != this->master.relays[i].offTime) {

            // Attempt to set, should be no issues that require a bool return.
            // Filtered possibilities in this if statement.
            this->relays[i].timerSet(true, this->master.relays[i].onTime);
            this->relays[i].timerSet(false, this->master.relays[i].offTime);
        }
    }
}

// Requires no params. Iterates each soil sensor, reads its settings from the
// NVS, and copies setting data over to the appropriate sensor current 
// configuration structure.
void settingSaver::loadSoil() {

    for (int i = 0; i < SOIL_SENSORS; i++) { // Iterate each config setting

        nvs_ret_t load = this->nvs.read(this->soilKeys[i], 
            &this->master.soil[i], sizeof(this->master.soil[i]));

        vTaskDelay(pdMS_TO_TICKS(NVS_SETTING_DELAY));

        if (load != nvs_ret_t::NVS_READ_OK) {
            snprintf(this->log, sizeof(this->log), "%s: Soil %d not read", 
                tag, i);

            this->sendLog();
            continue; // skip copy if data is bad.
        }

        Peripheral::AlertConfigSo* so = Peripheral::Soil::get()->getConfig(i);

        // If alert condition is actually set, copy the condition and value
        // to the current soil config.
        if (this->master.soil[i].cond != Peripheral::ALTCOND::NONE) {
            so->condition = this->master.soil[i].cond;
            so->tripVal = this->master.soil[i].tripVal;
        }
    }
}

// Requires no params. Reads the light settings from the NVS, and copies
// setting data over to the light current configuration structs.
void settingSaver::loadLight() {
   
    // Read the light struct and copy data into master light.
    nvs_ret_t load = this->nvs.read(LIGHT_KEY, &this->master.light, 
        sizeof(this->master.light));

    vTaskDelay(pdMS_TO_TICKS(NVS_SETTING_DELAY));

    if (load != nvs_ret_t::NVS_READ_OK) {
        snprintf(this->log, sizeof(this->log), "%s: Light not read", tag);
        this->sendLog();
        return; // Prevent the copying below.
    }
 
    Peripheral::RelayConfigLight* lt = Peripheral::Light::get()->getConf();

    // If relay condition != NONE, copy settings over.
    if (this->master.light.relayCond != Peripheral::RECOND::NONE) {

        lt->condition = this->master.light.relayCond;
        lt->tripVal = this->master.light.relayTripVal;
    }

    // Copy dark val settings.
    lt->darkVal = this->master.light.darkVal;

    // If relay number exists and is not LIGHT_NO_RELAY, will reattach relay
    if ((this->master.light.relayNum != LIGHT_NO_RELAY) && 
        this->master.light.relayNum < TOTAL_RELAYS) {
        Comms::SOCKHAND::attachRelayLT(this->master.light.relayNum, lt);
    }
}

// Requires messagining level type, which is default to ERROR. If other, 
// include it in the params. Before using, copy formatted string to log, and 
// then call this method to handle the log.
void settingSaver::sendLog(Messaging::Levels level) {
    this->log[sizeof(this->log) - 1] = '\0'; // Null terminate redundancy.
    Messaging::MsgLogHandler::get()->handle(level, this->log, 
        SAVESETTING_ERR_METHOD);
}

// Returns a pointer to the singleton instance.
settingSaver* settingSaver::get() {
    Threads::MutexLock(settingSaver::mtx);
    static settingSaver instance;
    
    return &instance;
}

// No params. Attempts an NVS save for all the peripheral settings.
void settingSaver::save() { 
    // Even though NVS has the functionality, manage all savings here and 
    // only write what is intended to change.

    this->saveTH(); 
    this->saveRelayTimers();
    this->saveSoil();
    this->saveLight();
}

// No params. Attempts an NVS read/load for all the peripheral settings.
void settingSaver::load() {
    // Load will load the settings from NVS, and copy them over to the
    // classes.
    
    this->loadTH();
    this->loadLight();
    this->loadRelayTimers();
    this->loadSoil();

    snprintf(this->log, sizeof(this->log), "%s: Data Loaded", tag);
    this->sendLog(Messaging::Levels::INFO);
}

// Requires relay array to be passed. Array must be global or static to ensure
// it stays within scope.
void settingSaver::initRelays(Peripheral::Relay* relayArray) {
    this->relays = relayArray;
    snprintf(this->log, sizeof(this->log), "%s: Relays init", tag);
    this->sendLog(Messaging::Levels::INFO);
}

}