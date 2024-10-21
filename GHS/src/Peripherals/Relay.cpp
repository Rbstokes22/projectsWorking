#include "Peripherals/Relay.hpp"
#include "driver/gpio.h"
#include "string.h"

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
    pin(pin), _isOn(false), clientQty(0), IDReg(1) {

    memset(this->IDs, 0, RELAY_IDS);
}

// Requires controller ID. Turns Relay on.
void Relay::on(uint16_t ID) {

    // Checks if ID exists in the array. If not, adds ID to first open
    // index. 
    if (!this->checkID(ID)) {
        if (!this->addID(ID)) {
            printf("Unable to add ID to Relay Control\n");
            return;
        }
    }

    // Turns on only if previously off.
    if (!this->_isOn) {
        gpio_set_level(this->pin, 1);
        this->_isOn = true;
    }
}

void Relay::off(uint16_t ID) {
    // Does not need to check for existance first, deletes if exists.
    this->delID(ID);

    // Turns off only if previously on and clientQty = 0, which signals
    // that no sensor is currently employing the relay.
    if (this->_isOn && clientQty == 0) {
        gpio_set_level(this->pin, 0);
        this->_isOn = false;
    }
}

// Returns an ID from 1 - 255. There will never be that many 
// devices with controlling IDs, so it is unproblematic.
uint16_t Relay::getID() {
    return Relay::IDReg++;
}

bool Relay::isOn() {
    return this->_isOn;
}

}

