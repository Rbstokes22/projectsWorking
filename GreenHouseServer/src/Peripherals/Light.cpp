#include "Peripherals.h"

namespace Devices {

Light::Light(uint8_t photoResistorPin) : 

currentLight{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
lightAccumulation{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
photoResistorPin{photoResistorPin}{}

void Light::begin() {
    this->as7341.begin();
}

void Light::readAndSet() {
    this->as7341.readAllChannels();
    this->currentLight.violet = 
        this->as7341.getChannel(AS7341_CHANNEL_415nm_F1);
    this->currentLight.indigo = 
        this->as7341.getChannel(AS7341_CHANNEL_445nm_F2);
    this->currentLight.blue = 
        this->as7341.getChannel(AS7341_CHANNEL_480nm_F3);
    this->currentLight.cyan = 
        this->as7341.getChannel(AS7341_CHANNEL_515nm_F4);
    this->currentLight.green = 
        this->as7341.getChannel(AS7341_CHANNEL_555nm_F5);
    this->currentLight.yellow = 
        this->as7341.getChannel(AS7341_CHANNEL_590nm_F6);
    this->currentLight.orange = 
        this->as7341.getChannel(AS7341_CHANNEL_630nm_F7);
    this->currentLight.red = 
        this->as7341.getChannel(AS7341_CHANNEL_680nm_F8);
    this->currentLight.nir = 
        this->as7341.getChannel(AS7341_CHANNEL_NIR);
    this->currentLight.clear = 
        this->as7341.getChannel(AS7341_CHANNEL_CLEAR);
    this->currentLight.flickerHz = 
        this->as7341.detectFlickerHz();
}

uint64_t Light::getColor(const char* color) {
    if (strcmp(color, "violet") == 0) return this->currentLight.violet;
    if (strcmp(color, "indigo") == 0) return this->currentLight.indigo;
    if (strcmp(color, "blue") == 0) return this->currentLight.blue;
    if (strcmp(color, "cyan") == 0) return this->currentLight.cyan;
    if (strcmp(color, "green") == 0) return this->currentLight.green;
    if (strcmp(color, "yellow") == 0) return this->currentLight.yellow;
    if (strcmp(color, "orange") == 0) return this->currentLight.orange;
    if (strcmp(color, "red") == 0) return this->currentLight.red;
    if (strcmp(color, "nir") == 0) return this->currentLight.nir;
    if (strcmp(color, "clear") == 0) return this->currentLight.clear;
    return 65536; // shows that the color passed was not in the scope of this function
}

uint64_t Light::getFlicker() {
    return this->currentLight.flickerHz;
}

void Light::getAccumulation() {

}

void Light::clearAccumulation() {
    lightAccumulation = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
}

uint16_t Light::getLightIntensity() {
    return analogRead(this->photoResistorPin);
}

void Light::handleSensors() {
    
}


}