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

// Requires no params. Copies the current light settings to the master
// struct, and then writes to NVS if settings have changed since last save.
// Returns true if NVS matches expected value, and false if not.
bool settingSaver::saveLight() {
    this->expected = 7; // Expected comparison values,
    this->total = 0; // Accumulation of trues, zero out.

    Peripheral::RelayConfigLight* lt = Peripheral::Light::get()->getConf();
    Peripheral::Spec_Conf* spec = Peripheral::Light::get()->getSpecConf();

    if (lt == nullptr || spec == nullptr) {
        snprintf(this->log, sizeof(this->log), "%s light is nullptr", 
            this->tag);

        this->sendErr(this->log);
        return false;
    }
    // 7 expected
    this->total += this->compare(this->master.light.relayNum, lt->num);
    this->total += this->compare(this->master.light.relayCond, lt->condition);
    this->total += this->compare(this->master.light.relayTripVal, lt->tripVal);
    this->total += this->compare(this->master.light.darkVal, lt->darkVal);
    this->total += this->compare(this->master.light.ASTEP, spec->ASTEP);
    this->total += this->compare(this->master.light.ATIME, spec->ATIME);
    this->total += this->compare(this->master.light.AGAIN, spec->AGAIN);

    if (this->total != this->expected) { // Changes detected, write to NVS.

        this->err = nvs.write(LIGHT_KEY, &this->master.light, 
            sizeof(this->master.light));

        vTaskDelay(pdMS_TO_TICKS(NVS_SETTING_DELAY)); // brief delay

        if (this->err != nvs_ret_t::NVS_WRITE_OK) {
            snprintf(this->log, sizeof(this->log), "%s light not saved", 
                this->tag);

            this->sendErr(this->log);
            return false;
        }

        snprintf(this->log, sizeof(this->log), "%s light saved", this->tag);
        this->sendErr(this->log, Messaging::Levels::INFO);
    } 

    return true; // light NVS matches expected value.
}

// Requires no params. Reads the light settings from the NVS, and copies
// setting data over to the light current configuration structs.
bool settingSaver::loadLight() {
   
    Peripheral::Light* lt = Peripheral::Light::get();

    if (lt == nullptr) {
        snprintf(this->log, sizeof(this->log), "%s light is nullptr", 
            this->tag);

        this->sendErr(this->log);
        return false;
    }

    Peripheral::RelayConfigLight* conf = lt->getConf(); // Set config ptr.

    // Read the light struct and copy data into master light.
    this->err = this->nvs.read(LIGHT_KEY, &this->master.light, 
        sizeof(this->master.light));

    vTaskDelay(pdMS_TO_TICKS(NVS_SETTING_DELAY)); // brief delay

    if (this->err != nvs_ret_t::NVS_READ_OK) {
        snprintf(this->log, sizeof(this->log), "%s light config not loaded", 
            this->tag);

        this->sendErr(this->log);
        return false; // Prevent the copying below.
    }

    snprintf(this->log, sizeof(this->log), "%s light config loaded", this->tag);
    this->sendErr(this->log, Messaging::Levels::INFO);

    // If relay condition != NONE, copy settings over and reattach relay.
    if (this->master.light.relayCond != Peripheral::RECOND::NONE) {
        conf->condition = this->master.light.relayCond;
        conf->tripVal = this->master.light.relayTripVal;

        if (this->master.light.relayNum < TOTAL_RELAYS &&
            this->master.light.relayNum != LIGHT_NO_RELAY) {

            Comms::SOCKHAND::attachRelayLT(this->master.light.relayNum, conf, 
                "light");
        }
    }

    // Copy dark val settings.
    conf->darkVal = this->master.light.darkVal;

    // Copy the integration and spectral settings.
    bool ATIME = lt->setATIME(this->master.light.ATIME);
    bool ASTEP = lt->setASTEP(this->master.light.ASTEP);
    bool AGAIN = lt->setAGAIN(this->master.light.AGAIN);

    if (ATIME && ASTEP && AGAIN) return true;

    // If any data is bad, log.
    snprintf(this->log, sizeof(this->log), 
        "Loaded: ATIME = %d, ASTEP = %d, AGAIN = %d", ATIME, ASTEP, AGAIN);

    this->sendErr(this->log);
    return false;
}
    
}