#include "Peripherals/PeripheralMain.h"

namespace FlashWrite {

const int16_t PeripheralSettings::ERR{-999};

PeripheralSettings::PeripheralSettings(
    const char* nameSpace, Messaging::MsgLogHandler &msglogerr
    ) :

    nameSpace{nameSpace}, msglogerr(msglogerr){}

// @params key is passed in to get value
// @brief Looks key up in NVS, returns value if exists, error if not.
// @return, if error is encountered, it will be handled by the caller
int16_t PeripheralSettings::read(const char* key) { 
    int16_t retVal{PeripheralSettings::ERR};
    this->prefs.begin(this->nameSpace);
    retVal = this->prefs.getShort(key, PeripheralSettings::ERR);
    this->prefs.end();
    return retVal; 
}

bool PeripheralSettings::write(const char* key, int16_t value) {
    this->prefs.begin(this->nameSpace);
    uint16_t bytesWritten{0};
    bool noWriteReq{true};

    int16_t readVal = this->prefs.getShort(key, PeripheralSettings::ERR);
    if (readVal != value) {
        bytesWritten = this->prefs.putShort(key, value);
        noWriteReq = false;
    } 

    this->prefs.end();
    return (bytesWritten == sizeof(value) || noWriteReq);
}

};

namespace Peripheral {

Sensors::Sensors(Messaging::MsgLogHandler &msglogerr) : 
    msglogerr{msglogerr}, mutex{msglogerr}{}

void Sensors::lock() {
    this->mutex.lock();
}

void Sensors::unlock() {
    this->mutex.unlock();
}

}