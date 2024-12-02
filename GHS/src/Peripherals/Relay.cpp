#include "Peripherals/Relay.hpp"
#include "driver/gpio.h"
#include "string.h"
#include "Common/Timing.hpp"

namespace Peripheral {

// Requires controller ID. Returns true if currently attached to a
// client, or false if available.
bool Relay::checkID(uint8_t ID) {
    return (clients[ID] != CLIENTS::AVAILABLE);
}

// Requires controller ID and the new state. Returns true upon a 
// state change, and false if error.
bool Relay::changeIDState(uint8_t ID, CLIENTS newState) {
    if (ID < 0 || ID >= RELAY_IDS) return false; // prevent error

    // Not grouped within the if-statement above, to prevent calling 
    // and ID outside the scope of the array.
    if (!this->checkID(ID)) return false;

    clients[ID] = newState;

    // Client quantity is limited to the amount of clients actively
    // energizing the relay. This is manage everytime the state toggles
    // between on and off. Client quantity will have to equal 0 in order
    // for relay to shut off, meaning there are no active clients
    // currently energizing it.
    if (newState == CLIENTS::ON) {
        clientQty++;
    } else if (newState == CLIENTS::OFF) {
        clientQty--;
    }

    return true;
}

// Requires gpio pin number, and the relay number.
Relay::Relay(gpio_num_t pin, uint8_t ReNum) : 
    pin(pin), ReNum(ReNum), state(STATE::OFF), clients{
    CLIENTS::AVAILABLE, CLIENTS::AVAILABLE, CLIENTS::AVAILABLE, 
    CLIENTS::AVAILABLE, CLIENTS::AVAILABLE, CLIENTS::AVAILABLE, 
    CLIENTS::AVAILABLE, CLIENTS::AVAILABLE, CLIENTS::AVAILABLE, 
    CLIENTS::AVAILABLE}, clientQty(0),
    timer{0, 0, false, false, false} {}

// Requires controller ID. Checks if ID is currently registered as a client
// of the relay, if not, returns false. If successful, changes the client
// state and energizes relay, returning true.
bool Relay::on(uint8_t ID) {
    // Will not run if relay is forced off.
    if (this->state == STATE::FORCED_OFF) return false;

    // Ensures the ID is registerd, and then changes state to on. If 
    // unsuccessful, returns false.
    if (!this->changeIDState(ID, CLIENTS::ON)) {
        printf("Relay: %u, ID: %u unable to change ID state\n", 
        this->ReNum, ID);
        return false;
    }

    // Turns on only if previously off or force removed.
    if (this->state != STATE::ON) {
        gpio_set_level(this->pin, 1);
        this->state = STATE::ON;
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
    if (!this->changeIDState(ID, CLIENTS::OFF)) {
        printf("Relay: %u, ID: %u unable to change ID state\n", 
        this->ReNum, ID);
        return false;
    }

    // Turns off only if previously on and clientQty = 0, which signals
    // that no sensor is currently employing the relay.
    if (this->state == STATE::ON && clientQty == 0) {
        gpio_set_level(this->pin, 0);
        this->state = STATE::OFF;
    }
    return true;
}

// Requires no parameters. Forces the relay to turn off despite
// what is currently dependent upon it. Good for emergencies.
void Relay::forceOff() {
    this->state = STATE::FORCED_OFF;
    printf("Relay %u forced off\n", this->ReNum);
    gpio_set_level(this->pin, 0);
}

// Requires no parameters. Removes the Force Off block.
void Relay::removeForce() {
    printf("Relay %u force off is removed\n", this->ReNum);
    this->state = STATE::FORCE_REMOVED;
}

// Iterates clients array and reserves the first spot available.
// Returns the index value of the reserved spot that will be used
// in all relay requests going forward. Returns 99 if no allocations
// are available.
uint8_t Relay::getID() {
    for (int i = 0; i < RELAY_IDS; i++) {
        if (clients[i] == CLIENTS::AVAILABLE) {
            this->changeIDState(i, CLIENTS::RESERVED);
            printf("Relay %u attached to ID %u\n",this->ReNum, i);
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
    if (ID < 0 || ID >= RELAY_IDS) return false; // prevent error.
    this->off(ID);
    this->changeIDState(ID, CLIENTS::AVAILABLE);
    printf("Relay %u Detached from ID %u\n", this->ReNum, ID);
    return true;
}

// Gets the current state of the relay. Returns 0, 1, 2, 3 for 
// off, on, forced off, and force removed, respetively.
STATE Relay::getState() {
    return this->state;
}

// Used to set single relay timer. Requires true or false if setting
// the on or off position, as well as the time in seconds. Returns
// true for successful setting, and false if not.
bool Relay::timerSet(bool on, uint32_t time) {
    if (time == 99999) { // 99999 Will disable the timer.
        timer.onSet = timer.offSet = false;
        return true;
    } else if (time < 0 || time >= 86400) { // Seconds per day
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
    static uint16_t ID = this->getID();

    if (timer.isReady) {
        Clock::DateTime* time = Clock::DateTime::get();

        // Logis applies to see if the timer runs through midnight.
        bool runsThruMid = (timer.offTime < timer.onTime);
        uint32_t curTime = time->getTime()->raw; // current time

        if (runsThruMid) {
            if (curTime >= timer.onTime || curTime <= timer.offTime) {
                this->on(ID);
            } else {
                this->off(ID);
            }

        } else {
            if (curTime >= timer.onTime && curTime <= timer.offTime) {
                this->on(ID);
            } else {
                this->off(ID);
            }
        }
    }
}

// Returns timer.
Timer* Relay::getTimer() {
    return &this->timer;
}

}

