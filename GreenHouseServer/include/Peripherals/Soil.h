#ifndef SOIL_H
#define SOIL_H

#include "Peripherals/PeripheralMain.h"

namespace Peripheral {

class Soil : public Sensors {
    private:
    PERPIN soilPin;
    uint8_t maxRetries;
    uint16_t dryValue; // Higher value
    uint16_t wetValue; // Lower value

    public:
    Soil(PERPIN soilPin, uint8_t maxRetries);
    void init();
    uint16_t getMoisture();
    void handleSensors();
};

}


#endif // SOIL_H