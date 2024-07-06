#ifndef PERIPHERALMAIN_H
#define PERIPHERALMAIN_H

#include "Threads/Mutex.h"
#include <Preferences.h>
#include "UI/MsgLogHandler.h"
#include "Common/Timing.h"

// ALL SENSOR SETTINGS HERE USE DEFAULT FOR NVS ISSUES. 
namespace NVS {

class PeripheralSettings {
    private:
    Preferences prefs;
    Messaging::MsgLogHandler &msglogerr;
    const char* nameSpace;
    static const int16_t ERR;

    public:
    PeripheralSettings(const char* nameSpace, Messaging::MsgLogHandler &msglogerr);
    int32_t read(const char* key);
    bool write(const char* key, int32_t value);
    bool setCheckSum();

};

}

namespace Peripheral {

const uint8_t PeripheralQty = 3;

enum class PERPIN : uint8_t { // Peripheral Pin
    RE1 = 15, // Relay 1
    DHT = 25, // DHT 22
    SOIL1 = 34, // Soil 1
    PHOTO = 35 // Photo Resistor
};

class Sensors {
    protected:
    Threads::Mutex mutex;
    Messaging::MsgLogHandler &msglogerr;
    Clock::Timer clockObj;

    public:
    Sensors(Messaging::MsgLogHandler &msglogerr, uint32_t checkSensorTime);
    virtual void handleSensors() = 0;
    void lock();
    void unlock();
    bool checkIfReady();
    virtual ~Sensors();
};

}

#endif // PERIPHERALMAIN_H