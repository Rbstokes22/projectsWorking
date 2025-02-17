#include "Peripherals/saveSettings.hpp"
#include "NVS2/NVS.hpp"
#include "string.h"
#include "Peripherals/TempHum.hpp"
#include "Peripherals/Soil.hpp"
#include "Peripherals/Light.hpp"
#include "Peripherals/Relay.hpp"

namespace NVS {

Threads::Mutex settingSaver::mtx; // define static var.

settingSaver::settingSaver() : nvs(NVS_NAMESPACE_SAVE) {
    memset(&this->master, 0, sizeof(this->master));
}

void settingSaver::checkTH() {
    const uint8_t exp = 5; // Expected true values (5 per device)
    uint8_t total = 0; // accumulation of true values

    // Do temperature followed by humidity.
    Peripheral::TH_TRIP_CONFIG* th = Peripheral::TempHum::get()->getTempConf();
    total += this->compare(this->master.temp.relayNum, th->relay.num);
    total += this->compare(this->master.temp.relayCond, 
        static_cast<uint8_t>(th->relay.condition));
    total += this->compare(this->master.temp.altCond, 
        static_cast<uint8_t>(th->alt.condition));
    total += this->compare(this->master.temp.relayTripVal, th->relay.tripVal);
    total += this->compare(this->master.temp.altTripVal, th->alt.tripVal);

    if (total != exp) { // If changes are detected, rewrite to NVS.
        nvs.write(TEMP_KEY, &this->master.temp, sizeof(this->master.temp));
    }

    total = 0; // Reset for humidity.

    // Change to humidity to compare
    th = Peripheral::TempHum::get()->getHumConf();
    total += this->compare(this->master.hum.relayNum, th->relay.num);
    total += this->compare(this->master.hum.relayCond, 
        static_cast<uint8_t>(th->relay.condition));
    total += this->compare(this->master.hum.altCond, 
        static_cast<uint8_t>(th->alt.condition));
    total += this->compare(this->master.hum.relayTripVal, th->relay.tripVal);
    total += this->compare(this->master.hum.altTripVal, th->alt.tripVal);

    if (total != exp) { // If changes are detected, rewrite to NVS.
        nvs.write(HUM_KEY, &this->master.hum, sizeof(this->master.hum));
    }
}

void settingSaver::checkRelayTimers() {
    const uint8_t exp = 5; // Expected true values (5 per device)
    uint8_t total = 0; // accumulation of true values

    // Make this like sockhand static var init, but create the 
    // relay class static varaible the same way as sockhand.
}

void settingSaver::checkSoil() {
    const uint8_t exp = 8; // Expected true values (2 per device)
    uint8_t total = 0; // accumulation of true values

    for (int i = 0; i < SOIL_SENSORS; i++) {
        
        Peripheral::AlertConfigSo* so = Peripheral::Soil::get()->getConfig(i);
        total += this->compare(this->master.soil[i].tripVal, so->tripVal);
        total += this->compare(this->master.soil[i].cond, 
            static_cast<uint8_t>(so->condition));
    }

    if (total != exp) { // If changes are detected, rewrite to NVS.
        nvs.write(SOIL_KEY, this->master.soil, sizeof(this->master.soil));
    }
}

void settingSaver::checkLight() {

}

settingSaver* settingSaver::get() {
    Threads::MutexLock(settingSaver::mtx);

    static settingSaver instance;
    return &instance;
}

void settingSaver::save() {
    // Even though NVS has the functionality, manage all savings here and 
    // only write what is intended to change.

    this->checkTH();
    this->checkRelayTimers();
    this->checkSoil();
    this->checkLight();

}

void settingSaver::load() {
    // Load will load the settings from NVS, and copy them over to the
    // classes. It will be a bit verbose, there might be a way to make
    // it easier.
}

}