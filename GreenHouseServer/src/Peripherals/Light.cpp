#include "Peripherals/Light.h"

namespace Peripheral {

Light::Light(
    PERPIN photoResistorPin, 
    uint8_t maxRetries,
    Messaging::MsgLogHandler &msglogerr,
    uint32_t checkSensorTime) : 

    Sensors(msglogerr, checkSensorTime),
    photoResistorPin{photoResistorPin},
    maxRetries{maxRetries}, dataCorrupt{false}, anyDataCorrupt{false},
    ATIME(29), ASTEP(599), GAIN(AS7341_GAIN_8X) {}
    
void Light::begin() {
    if (!this->as7341.begin()) {
        Serial.println("AS7341 Not Started");
    } else {
        // Data not good since it did not start
        this->anyDataCorrupt = true; this->dataCorrupt = true;
    }

    // ADD IN NVS READING AND WRITING TO MAKE THESE SETTINGS ABLE 
    // TO BE CALIBRATED FROM THE CLIENT, IT WILL BE ADVANCED SETTINGS
    // FEATURE TO DEVIATE FROM THE DEFAULT.

    // Settings and calibration. Integration time determines how long a 
    // channel measurement takes. The forumula is:
    // t = (ATIME + 1) * (ASTEP + 1) * 2.78 micro seconds
    // Higher gain coreesponds with lower light environments

    this->as7341.setATIME(this->ATIME); // Default 29, range 0 - 255
    this->as7341.setASTEP(this->ASTEP); // Default 599, range 0 - 65534
    this->as7341.setGain(this->GAIN); // Default 8X. Binary 2^-1 -> 2^8
}

void Light::readAndSet() {
    // Had some issues where the output max count was 1000. Uploaded example
    // sketch with arduino IDE, and reuploaded this same sketch and it seemed to
    // fix the issues. Made some adjustments and everything should be good now.
    static uint8_t retries = 0;
    uint16_t readings[12]{}; // All readings from readAllChannels

    if (this->as7341.readAllChannels(readings)) {

        for (size_t i = 0; i < static_cast<size_t>(COLORS::SIZE) - 2; i++) {
            this->lightComp.data[i] = readings[i];
            this->lightComp.dataAccum[i] += this->lightComp.data[i];
        }

        this->lightComp.data[static_cast<uint8_t>(COLORS::FLICKERHZ)] = 
        this->as7341.detectFlickerHz();

        this->lightComp.dataAccum[static_cast<uint8_t>(COLORS::SAMPLES)]++;
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

void Light::setATIME(uint8_t ATIME) {
    this->ATIME = ATIME;
}

void Light::setASTEP(uint16_t ASTEP) {
    this->ASTEP = ASTEP;
}

void Light::setGAIN(as7341_gain_t GAIN) {
    this->GAIN = GAIN;
}

uint64_t Light::getColor(COLORS color) {
    return lightComp.data[static_cast<uint8_t>(color)];
}

uint64_t Light::getFlicker() {
    return this->lightComp.data[static_cast<uint8_t>(COLORS::FLICKERHZ)]; // 1 = indeterminate
}

float Light::getAccumulation(COLORS color) {
    uint64_t counts = this->lightComp.dataAccum[static_cast<uint8_t>(color)];
    uint64_t samples = this->lightComp.dataAccum[static_cast<uint8_t>(COLORS::SAMPLES)];
    if (samples == 0) samples = 1;
    return  static_cast<float>(counts) / samples;
}

void Light::clearAccumulation() {
    memset(lightComp.dataAccum, 0, static_cast<uint8_t>(COLORS::SIZE));
    this->anyDataCorrupt = false;
}

uint16_t Light::getLightIntensity() {
    return analogRead(static_cast<int>(this->photoResistorPin));
}

void Light::handleSensors() {
    this->readAndSet();
    printf(
        "Vio: %llu, Ind: %llu, Blu: %llu, Cyan: %llu, Gren: %llu, Yel: %llu, Org: %llu,"
        "Red: %llu, nir: %llu, clr: %llu, flikerHz %llu\n",
        this->getColor(COLORS::VIOLET), 
        this->getColor(COLORS::INDIGO), 
        this->getColor(COLORS::BLUE), 
        this->getColor(COLORS::CYAN), 
        this->getColor(COLORS::GREEN), 
        this->getColor(COLORS::YELLOW), 
        this->getColor(COLORS::ORANGE), 
        this->getColor(COLORS::RED), 
        this->getColor(COLORS::NIR), 
        this->getColor(COLORS::CLEAR), 
        this->getColor(COLORS::FLICKERHZ)
    );
    
    }

}