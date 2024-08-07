#include "Peripherals/TempHum.h"

namespace Peripheral {

const float TempHum::ERR = 999;

TempHum::TempHum(
    PERPIN pin, 
    uint8_t type, 
    uint8_t maxRetries,
    Messaging::MsgLogHandler &msglogerr,uint32_t checkSensorTime) : 

    Sensors(msglogerr, checkSensorTime),
    dht{static_cast<uint8_t>(pin), type}, maxRetries{maxRetries}{}

void TempHum::begin() {
    this->dht.begin();
}

void TempHum::setTemp() {
    static uint8_t retries = 0;
    float buffer = this->dht.readTemperature();
    if (isnan(buffer)) {
        if (retries < this->maxRetries) {
            retries++;
        } else {
            this->Temp = TempHum::ERR;
            retries = 0;
        }
        
    } else {
        retries = 0;
        this->Temp = buffer;
    }
}

void TempHum::setHum() {
    static uint8_t retries = 0;
    float buffer = this->dht.readHumidity();
    if (isnan(buffer)) {
        if (retries < this->maxRetries) {
            retries++;
        } else {
            this->Hum = TempHum::ERR;
            retries = 0;
        }
        
    } else {
        retries = 0;
        this->Hum = buffer;
    }
}

float TempHum::getTemp(char tempUnits) {
    if (tempUnits == 'F' || tempUnits == 'f') {
        return (this->Temp * 1.8) + 32;
    } else {return this->Temp;}
}

float TempHum::getHum() {
    return this->Hum;
}

void TempHum::handleSensors() {
    this->setTemp(); this->setHum();
    
    printf("Temp: %f, Hum: %f\n", this->getTemp(), this->getHum());
}

}