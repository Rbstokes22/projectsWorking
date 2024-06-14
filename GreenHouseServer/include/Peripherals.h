#ifndef PERIPHERALS_H
#define PERIPHERALS_H

// Included peripherals
// DHT-12 (temp & humidity)
// Soil sensors (soil moisture content)
// AS7341 light spectrum analyzer (rate and accumulation of wavelengths)
// Relays (logic based operation of external electronics)
// Photoresistor (To determine if it is day or night)

// NOTES:
// Set light threshold 400 for dark. make this adjustable. no calib needed
// Soil sensor also needs a calibrator built in for dry and wet
// Use preferences to store sensor data

// BREAK INTO SEVERAL HEADERS AND SOURCE FILES IN DIR PERIPHERALS

#include <DHT.h>
#include <Wire.h>
#include <Adafruit_AS7341.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "Timing.h"

enum PeripheralPins {
    Relay_1_PIN = 15,
    DHT_PIN = 25,
    Soil_1_PIN = 34,
    Photoresistor_PIN = 35
};

namespace Devices {

// Measured in counts
struct LightComposition { // hange to uint64_t
    uint64_t violet; //F1 415nm
    uint64_t indigo; //F2 445nm
    uint64_t blue; // F3 480nm
    uint64_t cyan; // F4 515nm
    uint64_t green; // F5 555nm
    uint64_t yellow; // F6 590nm
    uint64_t orange; // F7 630nm
    uint64_t red; // F8 680nm
    uint64_t nir; // near infrared
    uint64_t clear; // broad spectrum light measurement
    uint64_t flickerHz; // flicker frequency
    uint64_t sampleQuantity;
};

class TempHum { // DHT-22
    private:
    DHT dht;

    public:
    TempHum(uint8_t pin, uint8_t type);
    float getTemp();
    float getHum();
};

class Light { // AS7341 & Photoresistor
    private:
    Adafruit_AS7341 as7341;
    LightComposition currentLight;
    LightComposition lightAccumulation;
    uint8_t photoResistorPin;

    public:
    Light(uint8_t photoResistorPin);
    void begin();
    void readAndSet();
    uint64_t getColor(const char* color);
    uint64_t getFlicker();
    void getAccumulation();
    void clearAccumulation();
    uint16_t getLightIntensity();
};

class Soil {
    private:

    public:

};

class Relay {
    private:

    public:
};

struct SensorObjects {
  Devices::TempHum &tempHum;
  Devices::Light &light;
  Clock::Timer &checkSensors;

  SensorObjects(
    Devices::TempHum &tempHum, 
    Devices::Light &light,
    Clock::Timer &checkSensors);
};

}


void handleSensors(
    Devices::TempHum &tempHum,
    Devices::Light &light
    );

#endif // PERIPHERALS_H