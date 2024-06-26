#ifndef PERIPHERALMAIN_H
#define PERIPHERALMAIN_H

#include "Threads/Mutex.h"
#include <Preferences.h>
#include "UI/MsgLogHandler.h"

enum class PERPIN : uint8_t { // Peripheral Pin
    RE1 = 15, // Relay 1
    DHT = 25, // DHT 22
    SOIL1 = 34, // Soil 1
    PHOTO = 35 // Photo Resistor
};

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
    int16_t read(const char* key);
    bool write(const char* key, int16_t value);

};

}

namespace Peripheral {

class Sensors {
    protected:
    Threads::Mutex mutex;
    Messaging::MsgLogHandler &msglogerr;

    public:
    Sensors(Messaging::MsgLogHandler &msglogerr);
    virtual void handleSensors() = 0;
    void lock();
    void unlock();
    virtual ~Sensors();
};

}

#endif // PERIPHERALMAIN_H