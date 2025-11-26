#include "Peripherals/saveSettings.hpp"
#include "NVS2/NVS.hpp"
#include "string.h"
#include "Peripherals/TempHum.hpp"
#include "Peripherals/Soil.hpp"
#include "Peripherals/Light.hpp"
#include "Drivers/AS7341/AS7341_Library.hpp"
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

        this->err = this->nvs.write(LIGHT_KEY, &this->master.light, 
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
    size_t tempVal = 0;

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

    // Good read. Continue on with checks and settings.

    snprintf(this->log, sizeof(this->log), "%s light config loaded", this->tag);
    this->sendErr(this->log, Messaging::Levels::INFO);

    // If relay condition != NONE, indicates a saved relay.
    if (this->master.light.relayCond != Peripheral::RECOND::NONE) {
        conf->condition = this->master.light.relayCond; // Set since exists.
        tempVal = this->master.light.relayTripVal;
        conf->tripVal = tempVal ? tempVal : 0; // Default to 0 if non-exist.

        if (this->master.light.relayNum < TOTAL_RELAYS &&
            this->master.light.relayNum != LIGHT_NO_RELAY) {

            Comms::SOCKHAND::attachRelayLT(this->master.light.relayNum, conf, 
                "light");
        }
    }

    // Copy dark val settings.
    tempVal = this->master.light.darkVal;
    conf->darkVal = tempVal ? tempVal : 0; // Default to 0 if non-exist.

    // Check the values for the saved integration settings to ensure default
    // setting if non-exist. System sets to default vals upon init. If saved
    // value is not 0 or the default setting, will set here. Set to true 
    // assuming all values are good, upon a set function, will adjust to false
    // if bad.

    bool ATIME = true, ASTEP = true, AGAIN = true;

    tempVal = this->master.light.ATIME; 

    if (tempVal > 0 && tempVal != AS7341_ATIME) ATIME = lt->setATIME(tempVal);
    
    tempVal = this->master.light.ASTEP;

    if (tempVal > 0 && tempVal != AS7341_ASTEP) ASTEP = lt->setASTEP(tempVal);

    AS7341_DRVR::AGAIN again = this->master.light.AGAIN; // set AGAIN.

    // Checks agains default value only, which the sensor is always init to.
    if (again != AS7341_DRVR::AGAIN::X256) lt->setAGAIN(again);

    if (ATIME && ASTEP && AGAIN) {
        return true;

    } else {
        // Log the load status.
        snprintf(this->log, sizeof(this->log), 
            "%s Int load stat ATIME(%d), ASTEP(%d), AGAIN(%d)", this->tag, 
            ATIME, ASTEP, AGAIN);

        this->sendErr(this->log);
        return false;
    }
}
    
}