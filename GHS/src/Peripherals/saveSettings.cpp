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

namespace NVS {

Threads::Mutex settingSaver::mtx; // define static var.

settingSaver::settingSaver() : nvs(NVS_NAMESPACE_SAVE), expected(0), total(0),
    relays(nullptr) {
    memset(&this->master, 0, sizeof(this->master));
}

void settingSaver::saveTH() {
    expected = 5; // Expected true values (5 per device)
    total = 0; // accumulation of true values

    // Do temperature followed by humidity.
    Peripheral::TH_TRIP_CONFIG* th = Peripheral::TempHum::get()->getTempConf();
    total += this->compare(this->master.temp.relayNum, th->relay.num);
    total += this->compare(this->master.temp.relayCond, th->relay.condition);
    total += this->compare(this->master.temp.altCond,th->alt.condition);
    total += this->compare(this->master.temp.relayTripVal, th->relay.tripVal);
    total += this->compare(this->master.temp.altTripVal, th->alt.tripVal);

    if (total != expected) { // If changes are detected, rewrite to NVS.
        nvs.write(TEMP_KEY, &this->master.temp, sizeof(this->master.temp));
    }

    total = 0; // Reset for humidity.

    // Change to humidity to compare
    th = Peripheral::TempHum::get()->getHumConf();
    total += this->compare(this->master.hum.relayNum, th->relay.num);
    total += this->compare(this->master.hum.relayCond, th->relay.condition);
    total += this->compare(this->master.hum.altCond, th->alt.condition);
    total += this->compare(this->master.hum.relayTripVal, th->relay.tripVal);
    total += this->compare(this->master.hum.altTripVal, th->alt.tripVal);

    if (total != expected) { // If changes are detected, rewrite to NVS.
        nvs.write(HUM_KEY, &this->master.hum, sizeof(this->master.hum));
    }
}

void settingSaver::saveRelayTimers() {
    
    if (this->relays == nullptr) {
        // log here
        return;
    }

    const char* keys[] = {RELAY1_KEY, RELAY2_KEY, RELAY3_KEY, RELAY4_KEY};

    for (int i = 0; i < TOTAL_RELAYS; i++) {
        expected = 2; // Expected true values 
        total = 0; // accumulation of true values

        total += this->compare(this->master.relays[i].onTime,
            this->relays[i].getTimer()->onTime);

        total += this->compare(this->master.relays[i].offTime,
            this->relays[i].getTimer()->offTime);

        if (total != expected) { // If changes are detected, rewrite to NVS.
            nvs.write(keys[i], &this->master.relays[i], 
            sizeof(this->master.relays[i]));
        }
    }
}

void settingSaver::saveSoil() {
    const char* keys[] = {SOIL1_KEY, SOIL2_KEY, SOIL3_KEY, SOIL4_KEY};

    for (int i = 0; i < SOIL_SENSORS; i++) {
        expected = 2; // Expected true values 
        total = 0; // accumulation of true values

        Peripheral::AlertConfigSo* so = Peripheral::Soil::get()->getConfig(i);
        total += this->compare(this->master.soil[i].tripVal, so->tripVal);
        total += this->compare(this->master.soil[i].cond, so->condition);

        if (total != expected) { // If changes are detected, rewrite to NVS.
            nvs.write(keys[i], &this->master.soil[i], 
            sizeof(this->master.soil[i]));

            vTaskDelay(pdMS_TO_TICKS(10)); // Brief delay between writes
        }
    }
}

void settingSaver::saveLight() {
    expected = 4;
    total = 0;

    Peripheral::RelayConfigLight* lt = Peripheral::Light::get()->getConf();
    total += this->compare(this->master.light.relayNum, lt->num);
    total += this->compare(this->master.light.relayCond, lt->condition);
    total += this->compare(this->master.light.relayTripVal, lt->tripVal);
    total += this->compare(this->master.light.darkVal, lt->darkVal);

    if (total != expected) {
        nvs.write(LIGHT_KEY, &this->master.light, sizeof(this->master.light));
    }
}

void settingSaver::loadTH() {

    // Read the temp and hum structs, and populate data into master struct.
    this->nvs.read(TEMP_KEY, &this->master.temp, sizeof(this->master.temp));
    this->nvs.read(HUM_KEY, &this->master.hum, sizeof(this->master.hum));

    auto copy = [](Peripheral::TH_TRIP_CONFIG* to, configSaveReAltTH &from) {

        if (from.relayCond != Peripheral::RECOND::NONE) { 

            to->relay.condition = from.relayCond;
            to->relay.tripVal = from.relayTripVal;
        }

        if (from.altCond != Peripheral::ALTCOND::NONE) { 

            to->alt.condition = from.altCond;
            to->alt.tripVal = from.altTripVal;
        }

        // Relay attached, copy data.
        if (from.relayNum != TEMP_HUM_NO_RELAY) { 
            Comms::SOCKHAND::attachRelayTH(from.relayNum, to);
        }

    };

    // Load the values from NVS to the temperature config first.
    Peripheral::TH_TRIP_CONFIG* th = Peripheral::TempHum::get()->getTempConf();
    copy(th, this->master.temp);

    // Then load into the hum and run the same stuff again.
    th = Peripheral::TempHum::get()->getHumConf();
    copy(th, this->master.hum);
}

void settingSaver::loadRelayTimers() {

    for (int i = 0; i < TOTAL_RELAYS; i++) {
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

void settingSaver::loadSoil() {

    for (int i = 0; i < SOIL_SENSORS; i++) {
        Peripheral::AlertConfigSo* so = Peripheral::Soil::get()->getConfig(i);

        if (this->master.soil[i].cond != so->condition) {
            so->condition = this->master.soil[i].cond;
            so->tripVal = this->master.soil[i].tripVal;
        }
    }
}

void settingSaver::loadLight() {

    // Read the light struct and copy data into master light.
    this->nvs.read(LIGHT_KEY, &this->master.light, sizeof(this->master.light));

    Peripheral::RelayConfigLight* lt = Peripheral::Light::get()->getConf();

    // If relay condition != none, copy settings over.
    if (this->master.light.relayCond != Peripheral::RECOND::NONE) {

        lt->condition = this->master.light.relayCond;
        lt->tripVal = this->master.light.relayTripVal;
    }

    // Copy dark val settings.
    lt->darkVal = this->master.light.darkVal;

    // If relay number exists and is not LIGHT_NO_RELAY, will reattach relay
    if (this->master.light.relayNum != LIGHT_NO_RELAY) {
        Comms::SOCKHAND::attachRelayLT(this->master.light.relayNum, lt);
    }
}

settingSaver* settingSaver::get() {
    Threads::MutexLock(settingSaver::mtx);
    static settingSaver instance;
    
    return &instance;
}

void settingSaver::save() { // Runs at a set interval from routine tasks
    // Even though NVS has the functionality, manage all savings here and 
    // only write what is intended to change.

    this->saveTH();
    this->saveRelayTimers();
    this->saveSoil();
    this->saveLight();
}

void settingSaver::load() {
    // Load will load the settings from NVS, and copy them over to the
    // classes.
     
    this->loadTH();
    this->loadLight();
    this->loadRelayTimers();
    this->loadSoil();
}

// Requires relay array to be passed. Array must be global or static to ensure
// it stays within scope.
void settingSaver::initRelays(Peripheral::Relay* relayArray) {
    this->relays = relayArray;
}

}