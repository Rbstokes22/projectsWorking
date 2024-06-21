#ifndef LIGHT_H
#define LIGHT_H

#include <Wire.h>
#include <Adafruit_AS7341.h>
#include "Peripherals/PeripheralMain.h"

namespace Peripheral {

// Used for get color by color instead of index. Example: instead of saying
// lightComp.data[1] = 33, You can use lightComp.data[INDIGO] = 33.
enum class COLORS : uint8_t { 
    VIOLET, // F1 415nm #8B00FF
    INDIGO, // F2 445nm #4B0082
    BLUE, // F3 480nm #0000FF
    CYAN, // F4 515nm #00FFFF
    GREEN, // F5 555nm #00FF00
    YELLOW, // F6 590nm #FFFF00
    ORANGE, // F7 630nm #FFA500
    RED, // F8 680nm #FF0000
    NIR, // Near InfraRed #555555
    CLEAR, // Broad Spectrum Light Measurement
    FLICKERHZ, // fligher detetion frequency
    SAMPLES, // Total amount of samples taken, for average accum
    SIZE // last enum, gives to total count
};

struct LightComposition {
    uint16_t data[static_cast<uint8_t>(COLORS::SIZE ) - 2]; // values from 0 - 65535
    uint64_t dataAccum[static_cast<uint8_t>(COLORS::SIZE)]; // accum of those values

    LightComposition() {
        memset(data, 0, static_cast<uint8_t>(COLORS::SIZE) - 1);
        memset(dataAccum, 0, static_cast<uint8_t>(COLORS::SIZE));
    }
};

class Light : public Sensors { // AS7341 & Photoresistor
    private:
    Adafruit_AS7341 as7341;
    LightComposition lightComp;
    PERPIN photoResistorPin;
    uint8_t maxRetries;
    bool dataCorrupt;
    bool anyDataCorrupt;
    uint8_t ATIME; // Default 29, range 0 - 255
    uint16_t ASTEP; // Default 599, range 0 - 65534
    as7341_gain_t GAIN; // Default 8X. Binary 2^-1 -> 2^8

    public:
    Light(
        PERPIN photoResistorPin, 
        uint8_t maxRetries,
        Messaging::MsgLogHandler &msglogerr);
        
    void begin();
    void readAndSet();
    void setATIME(uint8_t ATIME);
    void setASTEP(uint16_t ASTEP);
    void setGAIN(as7341_gain_t GAIN);
    uint64_t getColor(COLORS color);
    uint64_t getFlicker();
    float getAccumulation(COLORS color);
    void clearAccumulation();
    uint16_t getLightIntensity();
    void handleSensors() override;
};

}

#endif // LIGHT_H