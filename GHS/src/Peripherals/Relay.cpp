#include "Peripherals/Relay.hpp"
#include "driver/gpio.h"
#include "Threads/Mutex.hpp"
#include "string.h"
#include "Common/Timing.hpp"

namespace Peripheral {

// Requires controller ID. Returns true if currently attached to a
// client, or false if available.
bool Relay::isAttached(uint8_t ID) {
    return (clients[ID] != IDSTATE::AVAILABLE);
}

// Requires controller ID and the new state. Returns true upon a 
// state change, and false if error.
bool Relay::changeIDState(uint8_t ID, IDSTATE newState) {
    // Immediately filters IDs outside the index scope.
    if (ID >= RELAY_IDS) return false;

    // Returns true if the new state matches the previous state.
    if (clients[ID] == newState) return true; 

    // Filters requests to ensure that the ID is currently attached to the 
    // relay. If not, there is only one way this will pass, and that is if
    // the IDSTATE passed is RESERVED which happens only when calling the 
    // getID().This means that the ID is still listed as AVAILABLE which
    // returns a false from isAttached(). A typical actionable call will
    // have isAttached() return true, which passes this filter.
    if (!this->isAttached(ID) && newState != IDSTATE::RESERVED) {
        return false;
    }

    // Returns true indicating a success, despite the state not changing.
    if (clients[ID] == newState) {
        return true; 
    } else {
        clients[ID] = newState;
    }

    // Client quantity is limited to the amount of clients actively
    // energizing the relay. This is managed everytime the state toggles
    // between on and off. Client quantity will have to equal 0 in order
    // for relay to shut off, meaning there are no active clients
    // currently energizing it.
    if (newState == IDSTATE::ON) {clientQty++;}
    else if (newState == IDSTATE::OFF) {clientQty--;}

    return true;
}

// Requires gpio pin number, and the relay number.
Relay::Relay(gpio_num_t pin, uint8_t ReNum) : 
    pin(pin), ReNum(ReNum), relayState(RESTATE::OFF), clients{
    IDSTATE::AVAILABLE, IDSTATE::AVAILABLE, IDSTATE::AVAILABLE, 
    IDSTATE::AVAILABLE, IDSTATE::AVAILABLE, IDSTATE::AVAILABLE, 
    IDSTATE::AVAILABLE, IDSTATE::AVAILABLE, IDSTATE::AVAILABLE, 
    IDSTATE::AVAILABLE}, clientQty(0),
    timer{RELAY_TIMER_OFF, RELAY_TIMER_OFF, false, false, false},
    mtx() {}

// Requires controller ID. Checks if ID is currently registered as a client
// of the relay, if not, returns false. If successful, changes the client
// state and energizes relay, returning true.
bool Relay::on(uint8_t ID) {
    // Will not run if relay is forced off.
    if (this->relayState == RESTATE::FORCED_OFF) return false;
    Threads::MutexLock(this->mtx);

    // Ensures the ID is registerd, and then changes state to on. If 
    // unsuccessful, returns false.
    if (!this->changeIDState(ID, IDSTATE::ON)) {
        printf("Relay: %u, ID: %u unable to change ID state\n", 
        this->ReNum, ID);
        return false;
    }

    // Turns on only if previously off or force removed.
    if (this->relayState != RESTATE::ON) {
        gpio_set_level(this->pin, 1);
        this->relayState = RESTATE::ON;
    }

    return true;
}

// Requires controller ID. Deletes ID from the client registration. This
// signals to the relay that the client with that ID is no longer energizing
// it. Serves as a redundancy to prevent a client from shutting off a relay
// that is being used by another client. Ex: If the temperature and humidity
// both use relay 1 controlling a fan, and the temperature falls within an 
// acceptable range, but the humidity does not, the temperature will not be
// able to de-energize the relay.
bool Relay::off(uint8_t ID) {
    Threads::MutexLock(this->mtx);

    if (!this->changeIDState(ID, IDSTATE::OFF)) {
        printf("Relay: %u, ID: %u unable to change ID state\n", 
        this->ReNum, ID);
        return false;
    }

    // Turns off only if previously on and clientQty = 0, which signals
    // that no sensor is currently employing the relay.
    if (this->relayState == RESTATE::ON && clientQty == 0) {
        gpio_set_level(this->pin, 0);
        this->relayState = RESTATE::OFF;
    }
    return true;
}

// Requires no parameters. Forces the relay to turn off despite
// what is currently dependent upon it. Does not delete
// the current clients powering the device, those must be deleted
// using off(). Works well for emergencies.
void Relay::forceOff() {
    Threads::MutexLock(this->mtx);

    this->relayState = RESTATE::FORCED_OFF;
    printf("Relay %u forced off\n", this->ReNum);
    gpio_set_level(this->pin, 0);
}

// Requires no parameters. Removes the Force Off block to allow normal op.
void Relay::removeForce() {
    Threads::MutexLock(this->mtx);

    printf("Relay %u force off is removed\n", this->ReNum);
    this->relayState = RESTATE::FORCE_REMOVED;
}

// Iterates clients array and reserves the first spot available.
// Returns the index value of the reserved spot that will be used
// in all relay requests going forward. Returns 99 if no allocations
// are available.
uint8_t Relay::getID() {
    Threads::MutexLock(this->mtx);

    for (int i = 0; i < RELAY_IDS; i++) {
        if (clients[i] == IDSTATE::AVAILABLE) {
            this->changeIDState(i, IDSTATE::RESERVED);
            printf("Relay %u attached to ID %u\n", this->ReNum, i);
            return i;
        }
    }

    printf("Relay %u, not attached\n", this->ReNum);
    return 99;  // This indicates no ID available.
}

// Requires controller ID. Turns off relay if previously on meaning
// you do not have to call off() specifically before removing ID.
// Returns true if successful, and false if ID is out of range.
bool Relay::removeID(uint8_t ID) {
    Threads::MutexLock(this->mtx);

    if (ID >= RELAY_IDS) return false; // prevent error.
    this->off(ID);
    this->changeIDState(ID, IDSTATE::AVAILABLE);
    printf("Relay %u Detached from ID %u\n", this->ReNum, ID);
    return true;
}

// Gets the current state of the relay. Returns 0, 1, 2, 3 for 
// off, on, forced off, and force removed, respetively.
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

    if (time == RELAY_TIMER_OFF || time >= 86400) { // Disables timer
        timer.onSet = timer.offSet = false;
        return true;
    } else if (time >= 86400) { // Seconds per day
        return false; // Prevents overflow
    }
    
    if (on) {
        this->timer.onTime = time;
        timer.onSet = true;
    } else {
        this->timer.offTime = time;
        timer.offSet = true;
    }

    // Enusres that both on and off are set, and the are not equal.
    this->timer.isReady = (this->timer.onSet && this->timer.offSet && 
        (this->timer.onTime != this->timer.offTime));

    printf("Relay %u, timer set with on time %zu, and off time %zu\n",
            this->ReNum, (size_t)this->timer.onTime, 
            (size_t)this->timer.offTime);

    return true;
}

// Requires no params. This manages any set relay timers and will both
// turn them on and off during their set times.
void Relay::manageTimer() {
    Threads::MutexLock(this->mtx);

    static uint8_t ID = this->getID();

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

