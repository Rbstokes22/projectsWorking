#include "Peripherals.h"

namespace Devices {

Sensors::~Sensors(){}; // define here, doesnt really matter where this is.

const float TempHum::ERR = 999;

TempHum::TempHum(uint8_t pin, uint8_t type, uint8_t maxRetries) : 
    dht{pin, type}, maxRetries{maxRetries}
{
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
}

}