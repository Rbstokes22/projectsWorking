#ifndef PERIPHERALS_H
#define PERIPHERALS_H

// Included peripherals
// DHT-12 (temp & humidity)
// Soil sensors (soil moisture content)
// AS7341 light spectrum analyzer (rate and accumulation of wavelengths)
// Relays (logic based operation of external electronics)
// Photoresistor (To determine if it is day or night)

// NOTES:
// For calibrations, show analog readings 0 to 4095. The clients can then
// choose the value that they want for alerts and or/relay. This doesnt 
// have to show a physical value, but rather a range graph where they can 
// drag a slider to.

// Create an alerts headers, like threads, that will send alerts on these
// values. The alerts can be configured near the reading of the sensor,
// unlike the relays which will be below and have modifiable conditions.

#include <DHT.h>
#include <Wire.h>
#include <Adafruit_AS7341.h>
#include "Timing.h"

enum class DEVPIN : uint8_t { // Device pin
    RE1 = 15, // Relay 1
    DHT = 25, // DHT 22
    S1 = 34, // Soil 1
    PHOTO = 35 // Photo Resistor
};

namespace Devices {

// Used for get color by color instead of index. Example: instead of saying
// lightComp.data[1] = 33, You can use lightComp.data[INDIGO] = 33.
enum class COLORINDEX : uint8_t { 
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

// Channel map for iteration of the AS7341 channels.
extern as7341_color_channel_t channelMap[
    static_cast<uint8_t>(COLORINDEX::SIZE) - 2
    ];

struct LightComposition {
    uint16_t data[
        static_cast<uint8_t>(COLORINDEX::SIZE ) - 2
        ]; // values from 0 - 65535
    uint64_t dataAccum[static_cast<uint8_t>(COLORINDEX::SIZE)]; // accum of those values

    LightComposition() {
        memset(data, 0, static_cast<uint8_t>(COLORINDEX::SIZE) - 1);
        memset(dataAccum, 0, static_cast<uint8_t>(COLORINDEX::SIZE));
    }
};

class Sensors {
    public:
    virtual void handleSensors() = 0;
    virtual ~Sensors();
};

class Light : public Sensors { // AS7341 & Photoresistor
    private:
    Adafruit_AS7341 as7341;
    LightComposition lightComp;
    DEVPIN photoResistorPin;
    static const int ERR;
    uint8_t maxRetries;
    bool dataCorrupt;
    bool anyDataCorrupt;

    public:
    Light(DEVPIN photoResistorPin, uint8_t maxRetries);
    void begin();
    void readAndSet();
    uint64_t getColor(COLORINDEX color);
    uint64_t getFlicker();
    float getAccumulation(COLORINDEX color);
    void clearAccumulation();
    uint16_t getLightIntensity();
    void handleSensors() override;
};

class TempHum : public Sensors { // DHT-22
    private:
    DHT dht;
    float Temp;
    float Hum;
    static const float ERR;
    uint8_t maxRetries; // used for error handling in temp/hum

    public:
    TempHum(DEVPIN pin, uint8_t type, uint8_t maxRetries);
    void begin();
    void setTemp();
    void setHum();
    float getTemp(char tempUnits = 'C');
    float getHum();
    void handleSensors() override;
};

class Soil : public Sensors { // Capacitive soil sensor
    private:

    public:

};

class Relay {
    private:

    public:
};


}

#endif // PERIPHERALS_H