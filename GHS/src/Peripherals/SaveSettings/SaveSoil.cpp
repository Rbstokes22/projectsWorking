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

// Requires no params. Iterates each sensor and copies the current settings to
// the master struct, and write to NVS if settings have changed since last save.
// Returns true if NVS matches expected value, and false if not.
bool settingSaver::saveSoil() {

    this->expected = 2; // Expecte true values per iteration.
    Peripheral::AlertConfigSo* so = nullptr;

    auto checkVals = [this, &so](uint8_t sensorNum){
        this->total = 0; // Accumulation of true vals, zero out.
        
        // get soil alert configuration. check for non nullptr
        so = Peripheral::Soil::get()->getConfig(sensorNum);
        if (so == nullptr) {
            snprintf(this->log, sizeof(this->log), "%s soil %u is nullptr",
                this->tag, sensorNum);

            this->sendErr(this->log);
            return false;
        }

        this->total += this->compare(this->master.soil[sensorNum].tripVal,
            so->tripVal);

        this->total += this->compare(this->master.soil[sensorNum].cond,
            so->condition);

        if (this->total != this->expected) { // Changes, rewrite to NVS.

            this->err = nvs.write(this->soilKeys[sensorNum], 
                &this->master.soil[sensorNum], 
                sizeof(this->master.soil[sensorNum]));

            vTaskDelay(pdMS_TO_TICKS(NVS_SETTING_DELAY)); // brief delay

            if (this->err != nvs_ret_t::NVS_WRITE_OK) {
                snprintf(this->log, sizeof(this->log), "%s soil %u not saved",
                    this->tag, sensorNum);

                this->sendErr(this->log);
                return false;
            }

            snprintf(this->log, sizeof(this->log), "%s soil %u saved", 
                this->tag, sensorNum);

            this->sendErr(this->log, Messaging::Levels::INFO);
        }

        return true; // Value in NVS is what is expected.
    };

    size_t count = 0; // Will be used to count successes.

    for (int i = 0; i < SOIL_SENSORS; i++) {
        count += checkVals(i);
    }

    return (count == SOIL_SENSORS); // True if good, false if not.
}

// Requires no params. Iterates each soil sensor, reads its settings from the
// NVS, and copies setting data over to the appropriate sensor current 
// configuration structure. Returns true if successful load, or false if not.
// Upon first load, will return false if no data is saved in NVS.
bool settingSaver::loadSoil() {

    Peripheral::AlertConfigSo* so = nullptr;

    // Copies data read from NVS and the master struct to the soil configuration
    // settings. Does not need to reset pointer, because copy is set in load,
    // so if successful, the poiter will maintain address to copy over.
    auto copy = [this, &so](uint8_t sensorNum){

        // If alert condition is actually set, copy the condition and value
        // to the current soil config.
        if (this->master.soil[sensorNum].cond != Peripheral::ALTCOND::NONE) {
            so->condition = this->master.soil[sensorNum].cond;
            so->tripVal = this->master.soil[sensorNum].tripVal;
        }
        return true; // Used to incremement counts of success.
    };

    // Reads the NVS and loads the soil sensor data into the appropriate
    // soil configuration index.
    auto load = [this, &so](uint8_t sensorNum){

        // Get soil alert configuration. Check for nullptr.
        so = Peripheral::Soil::get()->getConfig(sensorNum);

        if (so == nullptr) {
            snprintf(this->log, sizeof(this->log), "%s soil %u is nullptr",
                this->tag, sensorNum);

            this->sendErr(this->log);
            return false;
        }

        this->err = this->nvs.read(this->soilKeys[sensorNum],
            &this->master.soil[sensorNum], 
            sizeof(this->master.soil[sensorNum]));

        vTaskDelay(pdMS_TO_TICKS(NVS_SETTING_DELAY)); // brief delay

        if (this->err != nvs_ret_t::NVS_READ_OK) {
            snprintf(this->log, sizeof(this->log), 
                "%s soil %d config not loaded", this->tag, sensorNum);

            this->sendErr(this->log);
            return false;
        }

        snprintf(this->log, sizeof(this->log), "%s soil %d config loaded",
            this->tag, sensorNum);

        this->sendErr(this->log, Messaging::Levels::INFO);
        return true;
    };

    size_t counts = 0; // Used to count successes

    for (int i = 0; i < SOIL_SENSORS; i++) {
        if (load(i)) counts += copy(i);
    }

    return (counts == SOIL_SENSORS); // Returns true if good.
}

}