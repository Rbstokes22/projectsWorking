#ifndef PERIPHERALMAIN_H
#define PERIPHERALMAIN_H

#include "Mutex.h"
#include "Preferences.h"

// ALL SENSOR SETTINGS HERE USE DEFAULT FOR NVS ISSUES. 
namespace FlashWrite {

class PeripheralSettings {
    private:

    public:

};

}

enum class PERPIN : uint8_t { // Peripheral Pin
    RE1 = 15, // Relay 1
    DHT = 25, // DHT 22
    SOIL1 = 34, // Soil 1
    PHOTO = 35 // Photo Resistor
};

namespace Peripheral {

class Sensors {
    protected:
    Threads::Mutex mutex;

    public:
    virtual void handleSensors() = 0;
    void lock();
    void unlock();
    virtual ~Sensors();
};

}

#endif // PERIPHERALMAIN_H