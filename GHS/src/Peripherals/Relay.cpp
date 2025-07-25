#include "Peripherals/Relay.hpp"
#include "driver/gpio.h"
#include "Threads/Mutex.hpp"
#include "string.h"
#include "Common/Timing.hpp"
#include "UI/MsgLogHandler.hpp"
#include "NVS2/NVS.hpp"
#include "Threads/Mutex.hpp"
#include "Peripherals/Soil.hpp"
#include "Config/config.hpp"
#include "Peripherals/Relay.hpp"
#include "UI/MsgLogHandler.hpp"
#include "Peripherals/Alert.hpp"

namespace Peripheral {

// ATTENTION: Basic Relay Flow.
// INIT: relay inits to off state, clients ID state init to available.
// GETID: Assigns an ID 0 - 9 to client, registers ID in clients arr with that 
//    clients state changing to reserved.
// That ID is now good to use to control the relay. ID will not change back to
// available until removeID is called using that ID.

// Static log 
char Relay::log[LOG_MAX_ENTRY]{0};
const char* Relay::IDSTATEMap[RELAY_ID_STATES] = {
    "AVAILABLE", "RESERVED", "ON", "OFF"
};

// Requires controller ID. Returns true if currently attached to a
// client, or false if available. Ensures that the ID is within its specified
// range, and that 
bool Relay::isAttached(uint8_t ID) {
    return (clients[ID] != IDSTATE::AVAILABLE); 
}

// Requires controller ID and the new requested state. If a different state
// is detected than the requested state, ensures the relay is attached to an
// active client. If so, the state will be changed. If the new state request is
// ON, the client quantity will be incrememented, and decremented if OFF. 
// Returns true if the current state matches the requested state, and false if
// it does not.
bool Relay::changeIDState(uint8_t ID, IDSTATE newState) {

    // Immediately filters IDs outside the index scope.
    if (ID >= RELAY_IDS || ID == RELAY_NO_ID) {
        snprintf(Relay::log, sizeof(Relay::log), 
            "%s ID %u [%s] exceeds allowable limit at %u. State unchanged",
            this->tag, ID, this->clientStr[ID], RELAY_IDS);

        this->sendErr(Relay::log);
        return false;
    }
    
    // Returns true if the new state matches the previous state.
    if (clients[ID] == newState) return true; // clients[ID] = prev state.

    // Filters requests to ensure that the ID is currently attached to the 
    // relay. If not, there is only one way this will pass, and that is if
    // the IDSTATE passed is RESERVED which happens only when calling the 
    // getID().This means that the ID is still listed as AVAILABLE which
    // returns a false from isAttached(). A typical actionable call will
    // have isAttached() return true, which passes this filter.
    if (!this->isAttached(ID) && newState != IDSTATE::RESERVED) {
        snprintf(Relay::log, sizeof(Relay::log), 
            "%s ID %u [%s] state not changed from %s to %s", 
            this->tag, ID, this->clientStr[ID],
            IDSTATEMap[static_cast<uint8_t>(clients[ID])],
            IDSTATEMap[static_cast<uint8_t>(newState)]);

        this->sendErr(Relay::log, Messaging::Levels::CRITICAL);
        return false; // Block.
    }

    // Log the state of the change before implementing below.
    snprintf(Relay::log, sizeof(Relay::log), 
        "%s ID %u [%s] state changing from %s to %s", 
        this->tag, ID, this->clientStr[ID],
        IDSTATEMap[static_cast<uint8_t>(clients[ID])],
        IDSTATEMap[static_cast<uint8_t>(newState)]);

    this->sendErr(Relay::log, Messaging::Levels::INFO);

    // Qty is limited to amount of clients actively energizing relay. Managed
    // everytime the state toggles between on and off. Qty must = 0 in order
    // for the relay to shut off, to prevent an off call with an active user.
    bool OK = clients[ID] == IDSTATE::OFF || clients[ID] == IDSTATE::RESERVED;

    if (OK && newState == IDSTATE::ON) { // Must start from off or reserved.
        this->clientQty = (this->clientQty >= RELAY_IDS) ? 
            RELAY_IDS : this->clientQty + 1;

    } else if (clients[ID] == IDSTATE::ON && newState == IDSTATE::OFF) {
        this->clientQty = (this->clientQty == 0) ? 0 : this->clientQty - 1;
    }

    clients[ID] = newState; // Set after log and client quantity change.
    return true;
}

// Requires message and messaging level. Level default to ERROR. Ignore repeat
// is set to true to allow all relay on/off activity to be tracked, even when
// repeated.
void Relay::sendErr(const char* msg, Messaging::Levels lvl) {
    Messaging::MsgLogHandler::get()->handle(lvl, msg, RELAY_LOG_METHOD, 
        true);
}

// Require Relay Number. Dynamically creates tag. Returns the pointer to the
// tag. Called in the construction of the mutex.
const char* Relay::createTag(uint8_t ReNum) {
    snprintf(this->tag, sizeof(this->tag), "(RELAY%u)", ReNum);
    return this->tag;
}

// Requires gpio pin number, and the relay number. Default timer days to ALL.
Relay::Relay(gpio_num_t pin, uint8_t ReNum) : 

    pin(pin), ReNum(ReNum), relayState(RESTATE::OFF), clientQty(0),
    timer{RELAY_TIMER_OFF, RELAY_TIMER_OFF, RELAY_DAYS, false},
    mtx(this->createTag(ReNum)) {

        memset(this->clients, static_cast<uint8_t>(IDSTATE::AVAILABLE), 
            sizeof(this->clients));

        memset(this->clientStr, 0 , sizeof(this->clientStr));

        // Set the timer manager ID after memset.
        char ID[16] = {0};
        snprintf(ID, sizeof(ID), "ManTmr%u", ReNum);
        this->timerID = this->getID(ID);

        snprintf(Relay::log, sizeof(Relay::log), "%s Ob created", this->tag);
        this->sendErr(Relay::log, Messaging::Levels::INFO);
    }

// Requires controller ID. Checks if ID is currently registered as a client
// of the relay, if not, returns false. If successful, changes the client
// state and energizes relay, returning true.
bool Relay::on(uint8_t ID) {
    Threads::MutexLock(this->mtx);

    // Will not run if relay is forced off or bad ID.
    bool frcOff = this->relayState == RESTATE::FORCED_OFF;
    bool badID = ((ID == RELAY_NO_ID) || (ID >= RELAY_IDS));

    if (frcOff || badID) return false; // Block use if true.

    IDSTATE prev = clients[ID]; // Previous ID state.
    bool IDstate = this->changeIDState(ID, IDSTATE::ON); // Change state to on.

    // Off position, but the first client will have been acquired @ turn on.
    if (this->relayState != RESTATE::ON && this->clientQty == 1) { 

        if (gpio_set_level(this->pin, 1) != ESP_OK) { // Unable to set. 

            snprintf(Relay::log, sizeof(Relay::log), 
                "%s ID %u [%s] unable to turn on", this->tag, ID, 
                this->clientStr[ID]);
            
            this->sendErr(Relay::log, Messaging::Levels::CRITICAL);

            this->changeIDState(ID, prev); // If error, return to previous state
            return false;
        }

        // Able to set, change relay/ID state and log.
        this->relayState = RESTATE::ON;
        snprintf(Relay::log, sizeof(Relay::log), "%s ID %u [%s] energized", 
            this->tag, ID, this->clientStr[ID]);

        this->sendErr(Relay::log, Messaging::Levels::INFO);
    } 

    return IDstate;
}

// Requires controller ID. Deletes the ID from the client registration. Signals
// to relay that the ID holder is relinquishing command. Serves as a redundancy
// to prevent client from shutting off relay being used by another client. 
// Returns true when client is detached from relay, and false if not.
bool Relay::off(uint8_t ID) {
    Threads::MutexLock(this->mtx);

    bool badID = ((ID == RELAY_NO_ID) || (ID >= RELAY_IDS));

    if (badID) return false; // Block, ID invalid.

    // Relinquish control over ID automatically regardless of relay state. This
    // is fine here because the client is "washing their hands" and no longer
    // desires control, and will decrement the client qty by 1.
    IDSTATE prev = clients[ID]; // Previous ID state.
    bool IDstate = this->changeIDState(ID, IDSTATE::OFF);

    // Turns off iff not previously off and client quantity is 1. 1 is because
    // it shows there is 1 remaining client, which will have their ID state 
    // changed to off upon success. Prevents relay from being de-energized by
    // any client, except the last client.
    if (this->relayState != RESTATE::OFF && clientQty == 0) { // relay on.
     
        if (gpio_set_level(this->pin, 0) != ESP_OK) {
            snprintf(Relay::log, sizeof(Relay::log), 
                "%s ID %u [%s] unable to turn off", this->tag, ID, 
                this->clientStr[ID]);
            
            this->sendErr(Relay::log, Messaging::Levels::CRITICAL);

            // If error, change ID state back to previous state.
            this->changeIDState(ID, prev);
            return false; // did not change level.
        }

        // Changed level successfully. Log and change ID state.
        this->relayState = RESTATE::OFF;
        snprintf(Relay::log, sizeof(Relay::log), "%s ID %u [%s] de-energized", 
            this->tag, ID, this->clientStr[ID]);

        this->sendErr(Relay::log, Messaging::Levels::INFO);
    } 

    return IDstate; 
}

// Require no parameters. Forces the relay to turn off despite any current
// clients. Does not delete the clients, but acts as a suspension, rather than
// a switch. Works well for emergencies.
void Relay::forceOff() { // No ID req, since it is global to the relay.
    Threads::MutexLock(this->mtx);

    if (gpio_set_level(this->pin, 0) != ESP_OK) { 
        snprintf(Relay::log, sizeof(Relay::log), "%s unable to turn off", 
            this->tag);

        this->sendErr(Relay::log, Messaging::Levels::CRITICAL);
        return; // Block.
    }

    // Upon success, change state to forced off.
    this->relayState = RESTATE::FORCED_OFF; // inhibit "on" activity
    snprintf(Relay::log, sizeof(Relay::log), "%s forced off",this->tag);
    this->sendErr(Relay::log, Messaging::Levels::INFO);
}

// Requires no parameters. Removes the Force Off block to allow normal op.
void Relay::removeForce() {
    Threads::MutexLock(this->mtx);

    // Block if the relay state was not previously forced off.
    if (this->relayState != RESTATE::FORCED_OFF) return; 

    snprintf(Relay::log, sizeof(Relay::log), "%s force off rmvd", this->tag);
    this->sendErr(Relay::log, Messaging::Levels::INFO);
    this->relayState = RESTATE::FORCE_REMOVED;

    // Check client quantity when removing force, if there are IDs still 
    // attached to the relay, re-energize with first device.
    if (this->clientQty > 0) {
        uint8_t firstOn = RELAY_NO_ID; // Will be used to find first on val.
        for (int i = 0; i < RELAY_IDS; i++) {
            if (clients[i] == IDSTATE::ON) {
                firstOn = i; // The index will also be the ID.
                break;
            }
        }

        if (firstOn != RELAY_NO_ID) this->on(firstOn); // re-energize
    }
}

// Requires caller string, that will be assigned to returned ID. Iterates
// clients array and reserves the first spot available. Returns the index value
// of the reserved spot that will be used in all relay requests going forward.
// Returns 255 if no allocations are available, or there was an error. This
// will prevent use of class features.
uint8_t Relay::getID(const char* caller) {
    Threads::MutexLock(this->mtx);

    for (int i = 0; i < RELAY_IDS; i++) {

        if (clients[i] == IDSTATE::AVAILABLE) {

            // If available, load caller into the client for log purposes.
            snprintf(this->clientStr[i], sizeof(this->clientStr[i]),
                    "%s", caller);

            return this->changeIDState(i, IDSTATE::RESERVED) ? i : RELAY_NO_ID;
        }
    }

    snprintf(Relay::log, sizeof(Relay::log), "%s not attached, zero avail", 
        this->tag);

    this->sendErr(Relay::log);
    return RELAY_NO_ID;  // This indicates no ID available.
}

// Requires controller ID. Turns off relay if previously on meaning
// you do not have to call off() specifically before removing ID.
// Returns true if successful, and false if ID is out of range.
bool Relay::removeID(uint8_t ID) {
    Threads::MutexLock(this->mtx);

    if (ID >= RELAY_IDS || ID == RELAY_NO_ID) return false; // prevent error.

    if (this->off(ID)) { // Logging within function, none req here.
        return this->changeIDState(ID, IDSTATE::AVAILABLE);
        // No requirement to change to anything other than available, as string
        // will be re-acquired.
    }

    return false; // Unable to turn off.
}

// Requires no params. Gets the current state of the relay. Returns 0, 1, 2, 3
// for off, on, forced off, and force removed, respectively.
RESTATE Relay::getState() {
    Threads::MutexLock(this->mtx);
    return this->relayState;
}

// Return the current client quantity.
uint8_t Relay::getQty() {
    Threads::MutexLock(this->mtx);
    return this->clientQty;
}

// Checks if the relay is being energized manually. Returns true if yes.
bool Relay::isManual() {
    Threads::MutexLock(this->mtx);

    for (int i = 0; i < RELAY_IDS; i++) {
        char caller[10] = {0}; // Just interested in the first 9 chars
        strncpy(caller, this->clientStr[i], sizeof(caller) - 1);
        if (strcmp(caller, "SKTHANDRE") == 0) return true;
    }

    return false;
}

// Requires the on and off seconds between 0 and 86399. If 99999 is passed for 
// either the on or off seconds, or they are equal in value, the timer will
// disable to its default settings. Returns true if successful, false if not.
bool Relay::timerSet(uint32_t onSeconds, uint32_t offSeconds) {
    Threads::MutexLock(this->mtx);

    if (onSeconds == RELAY_TIMER_OFF || offSeconds == RELAY_TIMER_OFF ||
        onSeconds == offSeconds) { // Set to default if conditions are met.

        this->timer.isReady = false;
        this->timer.onTime = RELAY_TIMER_OFF;
        this->timer.offTime = RELAY_TIMER_OFF;
        snprintf(Relay::log, sizeof(Relay::log), "%s timer disabled",
            this->tag);

        this->sendErr(Relay::log, Messaging::Levels::INFO);
        return true; // True because shutting off is intentional.

    } else if (onSeconds >= 86400 || offSeconds >= 86400) { // Exceeds max
        snprintf(Relay::log, sizeof(Relay::log), 
            "%s timer exceeds boundaries. Seconds on @ %lu, off @ %lu.", 
            this->tag, onSeconds, offSeconds);

        this->sendErr(Relay::log);
        return false; // Prevents overflow.
    }

    // Checks are good, set timer and duration.
    this->timer.onTime = onSeconds;
    this->timer.offTime = offSeconds;
    this->timer.isReady = true;

    snprintf(Relay::log, sizeof(Relay::log), 
        "%s timer enabled. On = %lu , Off = %lu", this->tag, onSeconds,
        offSeconds);

    this->sendErr(Relay::log, Messaging::Levels::INFO);
    return true;
}

// Requires the bitwise uint8_t storage variable. Sets the days the timer is
// to be enabled, monday = 0, sunday = 6. They will be stored in bits 0 - 6,
// with bit 6 being sunday, and bit 0 being monday. Returns true.
bool Relay::timerSetDays(uint8_t bitwise) {
    Threads::MutexLock(this->mtx);
    this->timer.days = bitwise; // No err checking needed, either good or not.
    return true;
}

// Requires no params. This manages any set relay timers and will both
// turn them on and off during their set times.
void Relay::manageTimer() { 
    Threads::MutexLock(this->mtx);

    if (this->timer.isReady) {
        Clock::TIME* tm = Clock::DateTime::get()->getTime();

        // Logis applies to see if the timer runs through midnight.
        bool runsThruMid = (this->timer.offTime < this->timer.onTime);

        uint32_t curTime = tm->raw; // current time
        uint8_t day = tm->day; // Current day.

        // Run bitwise checks to see if the timer is expected to be used today.
        // Works by using the bitwise storage of active days, and shifting by
        // the current day. EXAMPLE:
        // days = 0b01001010 means on Sun, Thurs, and Tues.
        // Using day, mon = 0, sun = 6. If today is monday, it will not shift
        // and & 0b1, which will be 0, indicating monday is an off day. On Tues,
        // it will shift 1, and & 0b1, which will be true, meaning run today.
        bool runToday = (this->timer.days >> day) & 0b1;
        
        // Checks the current time and ensures it is within the correct range.
        // Also checks that it is scheduled to run today. 
        // WARNING: If timer runs through midnight, and the following day is 
        // not selected, the timer will shutoff at midnight.
        // If conditions are met, timer will turn on or off. The mutex is 
        // unlocked before on and off calls, to prevent those functions from
        // trying to aquired a locked lock. The design is because an and off 
        // functiosn can be called publically outside of the timer function.
        if (runsThruMid) {
            if ((curTime >= this->timer.onTime || 
                curTime <= this->timer.offTime) && runToday) {

                this->mtx.unlock(); 
                this->on(this->timerID);
            } else {
                this->mtx.unlock();
                this->off(this->timerID);
            }

        } else { // Does not run through midnight.

            if (curTime >= timer.onTime && curTime <= timer.offTime && 
                runToday) {

                this->mtx.unlock();
                this->on(this->timerID);
            } else {
                this->mtx.unlock();
                this->off(this->timerID);
            }
        }

    } else { // Captures if a timer is shut off with an energized relay.
        this->mtx.unlock();
        this->off(this->timerID);
    }
}

// Returns timer for modication or review.
Timer* Relay::getTimer() {
    Threads::MutexLock(this->mtx);
    return &this->timer;
}

}

