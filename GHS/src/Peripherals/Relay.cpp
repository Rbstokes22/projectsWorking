#include "Peripherals/Relay.hpp"
#include "driver/gpio.h"
#include "string.h"
#include "Common/Timing.hpp"

namespace Peripheral {

// Requires the controller ID. Returns true if exists and false if not.
bool Relay::checkID(uint16_t ID) {
    for (int i = 0; i < RELAY_IDS; i++) {
        if (this->IDs[i] == ID) {
            return true;
        }
    }

    return false; // Not Found.
}

// Requires the controller ID. Increments clientQty, sets array index
// to ID, and returns true if successful. Returns false if not.
bool Relay::addID(uint16_t ID) {

    for (int i = 0; i < RELAY_IDS; i++) {
        if (this->IDs[i] == 0) {
            this->IDs[i] = ID;
            clientQty++;
            return true;
        }
    }

    return false; // Not Added
}

// Requires controller ID. Decrements clientQty, resets array index
// to 0, and returns true if successful. Returns false if not.
bool Relay::delID(uint16_t ID) {
    for (int i = 0; i < RELAY_IDS; i++) {
        if (this->IDs[i] == ID) {
            this->IDs[i] = 0;
            clientQty--;
            return true;
        }
    }

    return false; // Not Deleted.
}

Relay::Relay(gpio_num_t pin) : 
    pin(pin), state(STATE::OFF), clientQty(0), IDReg(1),
    timer{0, 0, false, false, false} {

    memset(this->IDs, 0, RELAY_IDS);
}

// Requires controller ID. Turns Relay on.
void Relay::on(uint16_t ID) {
    // Will not run if relay is forced off.
    if (this->state == STATE::FORCED_OFF) return;

    // Checks if ID exists in the array. If not, adds ID to first open
    // index. 
    if (!this->checkID(ID)) {
        if (!this->addID(ID)) {
            printf("Unable to add ID to Relay Control\n");
            return;
        }
    }

    // Turns on only if previously off or force removed.
    if (this->state != STATE::ON) {
        gpio_set_level(this->pin, 1);
        this->state = STATE::ON;
    }
}

// Requires ID. Turns relay off.
void Relay::off(uint16_t ID) {
    // Does not need to check for existance first, deletes if exists.
    this->delID(ID);

    // Turns off only if previously on and clientQty = 0, which signals
    // that no sensor is currently employing the relay.
    if (this->state == STATE::ON && clientQty == 0) {
        gpio_set_level(this->pin, 0);
        this->state = STATE::OFF;
    }
}

// Requires no parameters. Forces the relay to turn off despite
// what is currently dependent upon it. Good for emergencies.
void Relay::forceOff() {
    this->state = STATE::FORCED_OFF;
    gpio_set_level(this->pin, 0);
}

// Requires no parameters. Removes the Force Off block.
void Relay::removeForce() {
    this->state = STATE::FORCE_REMOVED;
}

// Returns an ID from 1 - 65535. There will never be that many 
// devices with controlling IDs, so it is unproblematic.
uint16_t Relay::getID() {
    return Relay::IDReg++;
}

// Gets the current state of the relay. Returns 0, 1, 2, 3 for 
// off, on, forced off, and force removed, respetively.
STATE Relay::getState() {
    return this->state;
}

// Used to set single relay timer. Requires true or false if setting
// the on or off position, as well as the time in seconds. Returns
// true for successful setting, and false if not.
bool Relay::timerSet(bool on, int time) {
    if (time == 99999) { // 99999 Will disable the timer.
        timer.onSet = timer.offSet = false;
        return true;
    } else if (time < 0 || time >= 86400) { // Seconds per day
        return false;
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

    return true;
}

// Requires no params. This manages any set relay timers and will both
// turn them on and off during their set times.
void Relay::manageTimer() {
    static uint16_t ID = this->getID();

    if (timer.isReady) {
        Clock::DateTime* time = Clock::DateTime::getInstance();

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

}

