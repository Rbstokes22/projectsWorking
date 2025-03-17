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

// Requires no params. Copies the current temp/hum settings to the master
// struct, and then writes to NVS if settings have changed since last save.
// Returns true if NVS matches the expected value, false if not.
bool settingSaver::saveTH() {
    this->expected = 5; // Expected true values (5 per device)

    auto checkVals = [this](Peripheral::TH_TRIP_CONFIG* conf, 
        configSaveReAltTH &ob, const char* key) {

        if (conf == nullptr || key == nullptr) {
            snprintf(this->log, sizeof(this->log), "%s saveTH nullptr", 
                this->tag);

            this->sendErr(this->log);
            return false;
        }

        this->total = 0; // zero out, accumulation of true values.
        this->total += this->compare(ob.relayNum, conf->relay.num);
        this->total += this->compare(ob.relayCond, conf->relay.condition);
        this->total += this->compare(ob.altCond, conf->alt.condition);
        this->total += this->compare(ob.relayTripVal, conf->relay.tripVal);
        this->total += this->compare(ob.altTripVal, conf->alt.tripVal);

        if (this->total != this->expected) { // Changes detected, rewrite.

            this->err = this->nvs.write(key, &ob, sizeof(ob));

            vTaskDelay(pdMS_TO_TICKS(NVS_SETTING_DELAY)); // brief delay

            if (this->err != nvs_ret_t::NVS_WRITE_OK) {
                snprintf(this->log, sizeof(this->log), "%s %s not saved",
                    this->tag, key);

                this->sendErr(this->log);
                return false; // Only false if write attempt and failed.
            }

            snprintf(this->log, sizeof(this->log), "%s %s saved",
                this->tag, key);

            this->sendErr(this->log, Messaging::Levels::INFO);
        }

        return true; // Value in NVS equals value expected
    };

    // Do temperature followed by humidity. Compare current values with the
    // master settings.
    Peripheral::TH_TRIP_CONFIG* th = Peripheral::TempHum::get()->getTempConf();
    bool tempChk = checkVals(th, this->master.temp, TEMP_KEY);

    // Change pointer to humidity
    th = Peripheral::TempHum::get()->getHumConf();
    bool humChk = checkVals(th, this->master.hum, HUM_KEY);

    return (tempChk && humChk); // True if both good.
}

// Requires no params. Reads the temp/hum settings from the NVS, and copies
// setting data over to the temp/hum current configuration structs. Returns
// true if successful, and false if not. Upon first load, will return false
// if data hasnt been written.
bool settingSaver::loadTH() {

    Peripheral::TH_TRIP_CONFIG* th = nullptr;

    // Copies data from both the temp and hum NVS read to the current settings. 
    auto copy = [this](Peripheral::TH_TRIP_CONFIG* to, 
        configSaveReAltTH &from, const char* caller) {

        if (to == nullptr || caller == nullptr) {
            snprintf(this->log, sizeof(this->log), "%s loadTH nullptr", 
                this->tag);

            this->sendErr(this->log);
            return false;
        }

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

        // If relay was set, attaches relay to the temp/hum settings. Indicies
        // begin at zero, so checks against total relays to ensure range 
        // compliance.
        if (from.relayNum != TEMP_HUM_NO_RELAY && 
            from.relayNum < TOTAL_RELAYS) {
                
            Comms::SOCKHAND::attachRelayTH(from.relayNum, to, caller);
        }

        return true;
    };

    // Loads the NVS data into the master settings before copying.
    auto load = [this](configSaveReAltTH &ob, const char* key){

        if (key == nullptr) {
            snprintf(this->log, sizeof(this->log), "%s loadTH nullptr", 
                this->tag);

            this->sendErr(this->log);
            return false;
        }

        this->err = this->nvs.read(key, &ob, sizeof(ob));
        vTaskDelay(pdMS_TO_TICKS(NVS_SETTING_DELAY)); // brief delay

        if (this->err != nvs_ret_t::NVS_READ_OK) {
            snprintf(this->log, sizeof(this->log), "%s %s config not loaded",
                this->tag, key);

            this->sendErr(this->log);
            return false;
        }

        snprintf(this->log, sizeof(this->log), "%s %s config loaded", 
            this->tag, key);

        this->sendErr(this->log, Messaging::Levels::INFO);
        return true;
    };

    size_t counts = 0; // These counts will incremements based upon success.
    const size_t expCounts = 2; // 1 for temp, 1 for hum.

    // If successful NVS load into master config temperature, copy those values
    // to the temperature configuration.
    if (load(this->master.temp, TEMP_KEY)) {
        th = Peripheral::TempHum::get()->getTempConf();
        if (copy(th, this->master.temp, "temp")) counts++; // Incremement if true.
    }

    // If successful NVS load into master config humidity, copy those values to
    // the humidity configuration.
    if (load(this->master.hum, HUM_KEY)) {
        th = Peripheral::TempHum::get()->getHumConf();
        if (copy(th, this->master.hum, "hum")) counts++;
    }

    return (counts == expCounts);
}
    
}