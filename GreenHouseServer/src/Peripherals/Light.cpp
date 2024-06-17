#include "Peripherals.h"

namespace Devices {

// Map used for iteration of the as7341 channels to avoid a verbose
// definition and to allow easy modification. You can pass a buffer of 
// uint16_t arrays to the readAllChannels function, but I prefer explicity
// getting the channel from this map.
as7341_color_channel_t channelMap[static_cast<uint8_t>(COLORINDEX::SIZE) - 2]{
    AS7341_CHANNEL_415nm_F1, AS7341_CHANNEL_445nm_F2,
    AS7341_CHANNEL_480nm_F3, AS7341_CHANNEL_515nm_F4,
    AS7341_CHANNEL_555nm_F5, AS7341_CHANNEL_590nm_F6,
    AS7341_CHANNEL_630nm_F7, AS7341_CHANNEL_680nm_F8,
    AS7341_CHANNEL_NIR, AS7341_CHANNEL_CLEAR
};

const int Light::ERR{65536}; // Error for light values

Light::Light(DEVPIN photoResistorPin, uint8_t maxRetries) : 
    photoResistorPin{photoResistorPin},
    maxRetries{maxRetries},
    dataCorrupt{false},
    anyDataCorrupt{false}{}
    
void Light::begin() {
    this->as7341.begin();
}

void Light::readAndSet() {
    static uint8_t retries = 0;
    if (this->as7341.readAllChannels()) {
        
        for (size_t i = 0; i < static_cast<size_t>(COLORINDEX::SIZE) - 2; i++) {
            this->lightComp.data[i] = this->as7341.getChannel(channelMap[i]);
            this->lightComp.dataAccum[i] += this->lightComp.data[i];
        }

        this->lightComp.data[static_cast<uint8_t>(COLORINDEX::FLICKERHZ)] = 
        this->as7341.detectFlickerHz();

        this->lightComp.dataAccum[static_cast<uint8_t>(COLORINDEX::SAMPLES)]++;
        retries = 0; this->dataCorrupt = false;
    } else {
        retries++;
    }

    if (retries > this->maxRetries) {
        retries = 0; 
        this->dataCorrupt = true; 
        // Triggered once to see if any data is corrupt. Resets in clear Accum.
        this->anyDataCorrupt = true; 
    }
}

uint64_t Light::getColor(COLORINDEX color) {
    return lightComp.data[static_cast<uint8_t>(color)];
}

uint64_t Light::getFlicker() {
    return this->lightComp.data[static_cast<uint8_t>(COLORINDEX::FLICKERHZ)]; // 1 = indeterminate
}

float Light::getAccumulation(COLORINDEX color) {
    uint64_t counts = this->lightComp.dataAccum[static_cast<uint8_t>(color)];
    uint64_t samples = this->lightComp.dataAccum[static_cast<uint8_t>(COLORINDEX::SAMPLES)];
    if (samples == 0) samples = 1;
    return  static_cast<float>(counts) / samples;
}

void Light::clearAccumulation() {
    memset(lightComp.dataAccum, 0, static_cast<uint8_t>(COLORINDEX::SIZE));
    this->anyDataCorrupt = false;
}

uint16_t Light::getLightIntensity() {
    return analogRead(static_cast<int>(this->photoResistorPin));
}

void Light::handleSensors() {this->readAndSet();}

}