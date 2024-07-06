#include "Peripherals/PeripheralMain.h"

namespace NVS {

const int16_t PeripheralSettings::ERR{-999};

PeripheralSettings::PeripheralSettings(
    const char* nameSpace, Messaging::MsgLogHandler &msglogerr) :

    nameSpace{nameSpace}, msglogerr(msglogerr){}

// @params key is passed in to get value
// @brief Looks key up in NVS, returns value if exists, error if not.
// @return, if error is encountered, it will be handled by the caller
int32_t PeripheralSettings::read(const char* key) { 
    int32_t retVal{PeripheralSettings::ERR};
    this->prefs.begin(this->nameSpace);
    retVal = this->prefs.getInt(key, PeripheralSettings::ERR);
    this->prefs.end();
    return retVal; 
}

bool PeripheralSettings::write(const char* key, int32_t value) {
    this->prefs.begin(this->nameSpace);
    uint8_t bytesWritten{0};
    bool noWriteReq{true};

    int16_t readVal = this->prefs.getInt(key, PeripheralSettings::ERR);
    if (readVal != value) {
        bytesWritten = this->prefs.putInt(key, value);
        noWriteReq = false;
    } 

    this->setCheckSum();
    
    return (bytesWritten == sizeof(value) || noWriteReq);
}

bool PeripheralSettings::setCheckSum() {
    


    this->prefs.end(); return false;
}

};


namespace Peripheral {

Sensors::Sensors(Messaging::MsgLogHandler &msglogerr, uint32_t checkSensorTime) : 
    msglogerr{msglogerr}, mutex{msglogerr}, clockObj{checkSensorTime} {}

void Sensors::lock() {
    this->mutex.lock();
}

void Sensors::unlock() {
    this->mutex.unlock();
}

bool Sensors::checkIfReady() {
    return this->clockObj.isReady();
}

Sensors::~Sensors(){}

}