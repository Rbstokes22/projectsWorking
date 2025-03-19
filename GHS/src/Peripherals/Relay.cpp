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
    if (ID >= RELAY_IDS) {
        snprintf(Relay::log, sizeof(Relay::log), 
            "%s ID %u <%s> exceeds allowable limit at %u. State unchanged",
            this->tag, ID, this->clientStr[ID], RELAY_IDS);

        this->sendErr(Relay::log);
        return false;
    }
    
    // Returns true if the new state matches the previous state.
    if (clients[ID] == newState) return true; 

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

    clients[ID] = newState; // Implement.

    // Client quantity is limited to the amount of clients actively
    // energizing the relay. This is managed everytime the state toggles
    // between on and off. Client quantity will have to equal 0 in order
    // for relay to shut off, meaning there are no active clients
    // currently energizing it.
    if (newState == IDSTATE::ON) {clientQty++;}
    else if (newState == IDSTATE::OFF) {clientQty--;}

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

// Requires gpio pin number, and the relay number.
Relay::Relay(gpio_num_t pin, uint8_t ReNum) : 

    pin(pin), ReNum(ReNum), relayState(RESTATE::OFF), clientQty(0),
    timer{RELAY_TIMER_OFF, RELAY_TIMER_OFF, false, false, false},
    mtx(this->createTag(ReNum)) {

        memset(this->clients, static_cast<uint8_t>(IDSTATE::AVAILABLE), 
            sizeof(this->clients));

        memset(this->clientStr, 0 , sizeof(this->clientStr));

        snprintf(Relay::log, sizeof(Relay::log), "%s Ob created", this->tag);
        this->sendErr(Relay::log, Messaging::Levels::INFO);
    }

// Requires controller ID. Checks if ID is currently registered as a client
// of the relay, if not, returns false. If successful, changes the client
// state and energizes relay, returning true.
bool Relay::on(uint8_t ID) {

    // Will not run if relay is forced off or bad ID.
    if (this->relayState == RESTATE::FORCED_OFF || ID == RELAY_BAD_ID) {
        return false;
    }

    Threads::MutexLock(this->mtx);

    // Turns on only if previously off or force removed.
    if (this->relayState != RESTATE::ON) {

        if (gpio_set_level(this->pin, 1) != ESP_OK) {

            snprintf(Relay::log, sizeof(Relay::log), 
                "%s ID %u <%s> unable to turn on", this->tag, ID, 
                this->clientStr[ID]);
            
            this->sendErr(Relay::log, Messaging::Levels::CRITICAL);
            return false;
            }

        this->relayState = RESTATE::ON;
        snprintf(Relay::log, sizeof(Relay::log), "%s ID %u <%s> energized", 
            this->tag, ID, this->clientStr[ID]);

        this->sendErr(Relay::log, Messaging::Levels::INFO);
    }

    // If relay is energized, change the ID statue by ensuring the ID is
    // registered, and then change the state ot ON. If unsuccessful, returns
    // false.
    return this->changeIDState(ID, IDSTATE::ON);
}

// Requires controller ID. Deletes the ID from the client registration. Signals
// to relay that the ID holder is relinquishing command. Serves as a redundancy
// to prevent client from shutting off relay being used by another client. 
// Returns true when client is detached from relay, and false if not.
bool Relay::off(uint8_t ID) {
    Threads::MutexLock(this->mtx);

    if (ID == RELAY_BAD_ID) return false; // Block, ID invalid.

    // Turns off only if previously on and clientQty = 0, which signals
    // that no sensor is currently employing the relay.
    if (this->relayState == RESTATE::ON && clientQty == 0) {

        if (gpio_set_level(this->pin, 0) != ESP_OK) {
            snprintf(Relay::log, sizeof(Relay::log), 
                "%s ID %u <%s> unable to turn off", this->tag, ID, 
                this->clientStr[ID]);
            
            this->sendErr(Relay::log, Messaging::Levels::CRITICAL);
            return false;
        }

        this->relayState = RESTATE::OFF;
        snprintf(Relay::log, sizeof(Relay::log), "%s ID %u <%s> de-energized", 
            this->tag, ID, this->clientStr[ID]);

        this->sendErr(Relay::log, Messaging::Levels::INFO);
    }

    // If relay is de-energized, change the ID statue by ensuring the ID control
    // is released, and then change the state to OFF. If unsuccessful, returns
    // false. 
    return this->changeIDState(ID, IDSTATE::OFF);
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

    this->relayState = RESTATE::FORCED_OFF; // inhibit on activity
    snprintf(Relay::log, sizeof(Relay::log), "%s forced off",this->tag);
    this->sendErr(Relay::log, Messaging::Levels::INFO);
}

// Requires no parameters. Removes the Force Off block to allow normal op.
void Relay::removeForce() {
    Threads::MutexLock(this->mtx);

    snprintf(Relay::log, sizeof(Relay::log), "%s force off rmvd", this->tag);
    this->sendErr(Relay::log, Messaging::Levels::INFO);
    this->relayState = RESTATE::FORCE_REMOVED;
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

            return this->changeIDState(i, IDSTATE::RESERVED) ? i : RELAY_BAD_ID;
        }
    }

    snprintf(Relay::log, sizeof(Relay::log), "%s not attached, zero avail", 
        this->tag);

    this->sendErr(Relay::log);
    return RELAY_BAD_ID;  // This indicates no ID available.
}

// Requires controller ID. Turns off relay if previously on meaning
// you do not have to call off() specifically before removing ID.
// Returns true if successful, and false if ID is out of range.
bool Relay::removeID(uint8_t ID) {
    Threads::MutexLock(this->mtx);

    if (ID >= RELAY_IDS || ID == RELAY_BAD_ID) return false; // prevent error.

    if (this->off(ID)) { // Logging within function, none req here.
        return this->changeIDState(ID, IDSTATE::AVAILABLE);
        // No requirement to change anything other than available as string
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

// Requires bool on or off, to set the on or off time, as well as the time.
// The value passed must be between 0 and 86399, and the value 99999 will
// set the on and off setting to false preventing the relay from being 
// energized on time schedule. Returns true if successful, or false if
// parameters are exceeded.
bool Relay::timerSet(bool on, uint32_t time) {
    Threads::MutexLock(this->mtx);

    if (time == RELAY_TIMER_OFF) { // Disables timer
        this->timer.onSet = this->timer.offSet = false;
        this->timer.onTime = this->timer.offTime = RELAY_TIMER_OFF;
        snprintf(Relay::log, sizeof(Relay::log), "%s timer disabled",
            this->tag);

        this->sendErr(Relay::log, Messaging::Levels::INFO);
        return true;

    } else if (time >= 86400) { // Seconds per day are exceeded
        snprintf(Relay::log, sizeof(Relay::log), 
            "%s timer notset, Exceeds time @ %lu", this->tag, time);

        this->sendErr(Relay::log);
        return false; // Prevents overflow, must be between 0 and 64399.
    }
    
    if (on) { // Checks if setting on time or off time.
        this->timer.onTime = time;
        timer.onSet = true;
        snprintf(Relay::log, sizeof(Relay::log), "%s on time = %lu", this->tag, 
            time);

    } else {

        this->timer.offTime = time;
        timer.offSet = true;
        snprintf(Relay::log, sizeof(Relay::log), "%s off time = %lu", this->tag, 
            time);
    }

    this->sendErr(Relay::log, Messaging::Levels::INFO);

    // Enusres that both on and off are set, and the are not equal.
    this->timer.isReady = (this->timer.onSet && this->timer.offSet && 
        (this->timer.onTime != this->timer.offTime));

    if (this->timer.isReady) { // If ready, log ready.
        snprintf(Relay::log, sizeof(Relay::log), "%s timer enabled", this->tag);
        this->sendErr(Relay::log, Messaging::Levels::INFO);
    }

    return true;
}

// Requires no params. This manages any set relay timers and will both
// turn them on and off during their set times.
void Relay::manageTimer() {
    Threads::MutexLock(this->mtx);

    static uint8_t ID = this->getID("ManTimer");

    if (this->timer.isReady) {
        Clock::DateTime* time = Clock::DateTime::get();

        // Logis applies to see if the timer runs through midnight.
        bool runsThruMid = (this->timer.offTime < this->timer.onTime);
        uint32_t curTime = time->getTime()->raw; // current time

        // Checks the current time with the set on and off times. If conditions
        // are met, timer will turn on or off. The mutex is unlocked before
        // calling on and off to avoid both of those functions from reaquiring
        // an existing lock, but the other variables need to be protected.
        if (runsThruMid) {
            if (curTime >= this->timer.onTime || 
                curTime <= this->timer.offTime) {
                this->mtx.unlock(); 
                this->on(ID);
            } else {
                this->mtx.unlock();
                this->off(ID);
            }

        } else {
            if (curTime >= timer.onTime && curTime <= timer.offTime) {
                this->mtx.unlock();
                this->on(ID);
            } else {
                this->mtx.unlock();
                this->off(ID);
            }
        }
    }
}

// Returns timer for modication or review.
Timer* Relay::getTimer() {
    Threads::MutexLock(this->mtx);
    return &this->timer;
}

}

