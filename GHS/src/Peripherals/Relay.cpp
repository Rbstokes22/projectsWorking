#include "Peripherals/Relay.hpp"
#include "driver/gpio.h"
#include "string.h"

namespace Peripheral {

bool Relay::checkID(uint8_t ID) {
    for (int i = 0; i < RELAY_IDS; i++) {
        if (this->IDs[i] == ID) {
            return true;
        }
    }

    return false; // Not Found.
}

bool Relay::addID(uint8_t ID) {

    for (int i = 0; i < RELAY_IDS; i++) {
        if (this->IDs[i] == 0) {
            this->IDs[i] = ID;
            clientQty++;
            return true;
        }
    }

    return false; // Not Added
}

bool Relay::delID(uint8_t ID) {
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
    pin(pin), isOn(false), clientQty(0) {

    memset(this->IDs, 0, RELAY_IDS);
}

// ID must be greater than 0
void Relay::on(uint8_t ID) {
    // When commanded to turn on, if ID does not exist, adds it to the first
    // spot in the IDs array. Checks ID first to avoid polluting array. 
    if (!this->checkID(ID)) {
        this->addID(ID);
    }

    if (!this->isOn) {
        gpio_set_level(this->pin, 1);
        this->isOn = true;
    }
}

void Relay::off(uint8_t ID) {
    // Does not need to check for existance first, deletes if exists.
    this->delID(ID);

    if (this->isOn && clientQty == 0) {
        gpio_set_level(this->pin, 0);
        this->isOn = false;
    }
}

}

