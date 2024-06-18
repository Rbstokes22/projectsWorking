#include "Peripherals/Soil.h"
#include <Arduino.h>

namespace Peripheral {

Soil::Soil(PERPIN soilPin, uint8_t maxRetries) : 
    soilPin(soilPin), maxRetries(maxRetries),
    dryValue(2500), wetValue(1000) {}

void Soil::init() {
    
}

uint16_t Soil::getMoisture() {
    return analogRead(static_cast<int>(this->soilPin));
}

void Soil::handleSensors() {
    printf("Soil Reading: %d\n", this->getMoisture());
}
    
}

