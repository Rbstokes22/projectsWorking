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

// Requires no params. Iterates each relay and copies the current settings to
// the master struct, and write to NVS if settings have changed since last save.
// Returns true if NVS matches the expected value, false if not.
bool settingSaver::saveRelayTimers() {
    
    if (this->relays == nullptr) { // ensures the relays have been init.
        snprintf(this->log, sizeof(this->log), "%s: Relay is nullptr", 
            this->tag);

        this->sendErr(this->log);
        return false;
    }

    this->expected = 3; // Expected true values per relay.

    // Iterate relays and compare values with master config values.
    auto checkVals = [this](uint8_t relayNum){
        this->total = 0; // Accumulation of true values, zero out.
        Peripheral::Timer* tmr = this->relays[relayNum].getTimer();
        configSaveReTimer &mst = this->master.relays[relayNum]; // ref to update

        if (tmr == nullptr) {
            snprintf(this->log, sizeof(this->log), "%s relay %u is nullptr",
                this->tag, relayNum);

            this->sendErr(this->log);
            return false;
        }

        // Run comparisons and accumulate total.
        this->total += this->compare(mst.onTime, tmr->onTime);
        this->total += this->compare(mst.offTime, tmr->offTime);
        this->total += this->compare(mst.days, tmr->days);

        if (this->total != this->expected) { // Changes, rewrite to NVS.

            this->err = this->nvs.write(this->relayKeys[relayNum],
                &this->master.relays[relayNum], 
                sizeof(this->master.relays[relayNum]));

            vTaskDelay(pdMS_TO_TICKS(NVS_SETTING_DELAY)); // brief delay

            if (this->err != nvs_ret_t::NVS_WRITE_OK) {
                snprintf(this->log, sizeof(this->log), "%s relay %u not saved",
                    this->tag, relayNum);

                this->sendErr(this->log);
                return false;
            }

            snprintf(this->log, sizeof(this->log), "%s relay %u saved", 
                this->tag, relayNum);

            this->sendErr(this->log, Messaging::Levels::INFO);
        }

        return true; // Value in NVS is what is expected.
    };

    size_t count = 0; // Will be used to count successes.

    for (int i = 0; i < TOTAL_RELAYS; i++) {
        count += checkVals(i);
    }

    return (count == TOTAL_RELAYS); // Return true if ok, false if any error.
}

// Requires no params. Iterates each relay, reads its settings from the
// NVS, and copies setting data over to the appropriate relay current 
// configuration structure. Returns true if a successful load, and false if
// not. Upon first load, will return false if data hasnt been written.
bool settingSaver::loadRelayTimers() {

    if (this->relays == nullptr) { // ensures the relays have been init.
        snprintf(this->log, sizeof(this->log), "%s: Relay is nullptr", 
            this->tag);

        this->sendErr(this->log);
        return false;
    }

    // Copies loaded NVS settings to actual relay settings. Will not copy over
    // if criteria is not met.
    auto copy = [this](uint8_t reNum){
        uint32_t onTime = this->master.relays[reNum].onTime;
        uint32_t offTime = this->master.relays[reNum].offTime;
        uint8_t days = this->master.relays[reNum].days;

        // Sets proceed to true if conditions are met meaning that the last
        // state of the relay is legit.
        bool proceed = (onTime != offTime) && (onTime != RELAY_TIMER_OFF) &&
            (offTime != RELAY_TIMER_OFF);

        if (proceed) { // If good, set on and off times, err hand in funcs.
            this->relays[reNum].timerSet(onTime, offTime);
            
            // Ensure days are appropriate bitwise meaning max = 0b01111111
            // for each day of the week.
            if (days < 0b10000000) this->relays[reNum].timerSetDays(days);
        }
        
        return true; // No false returns, helps increment counts.
    };

    // Loads the NVS data to the master configuration struct.
    auto load = [this](uint8_t reNum){
        this->err = this->nvs.read(this->relayKeys[reNum], 
            &this->master.relays[reNum], sizeof(this->master.relays[reNum]));

        vTaskDelay(pdMS_TO_TICKS(NVS_SETTING_DELAY)); // Brief delay

        if (this->err != nvs_ret_t::NVS_READ_OK) {
            snprintf(this->log, sizeof(this->log), 
                "%s relay %u config not loaded", this->tag, reNum);

            this->sendErr(this->log);
            return false;
        }

        snprintf(this->log, sizeof(this->log), "%s relay %u config loaded",
            this->tag, reNum);

        this->sendErr(this->log, Messaging::Levels::INFO);
        return true;
    };

    size_t counts = 0; // Will count load and copy successes.

    // Iterate and load/copy all relay settings.
    for (int i = 0; i < TOTAL_RELAYS; i++) {
        if (load(i)) counts += copy(i);
    }

    return (counts == TOTAL_RELAYS); // True if all loaded properly.
}
    
}