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

#include <DHT.h>
#include <Wire.h>
#include <Adafruit_AS7341.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "Timing.h"

#define Relay_1_PIN 15
#define DHT_PIN 25 
#define Soil_1_PIN 34
#define Photoresistor_PIN 35

namespace Threads {

class SensorThread {
    private:
    TaskHandle_t taskHandle;
    Clock::Timer &checkSensors;
    bool isThreadSuspended;
    
    public:
    SensorThread(Clock::Timer &checkSensors);
    void setupThread();
    static void sensorTask(void* parameter);
    void suspendTask();
    void resumeTask();
};

}

namespace Devices {

struct LightComposition { // hange to uint64_t
    int violet; //F1 415nm
    int indigo; //F2 445nm
    int blue; // F3 480nm
    int cyan; // F4 515nm
    int green; // F5 555nm
    int yellow; // F6 590nm
    int orange; // F7 630nm
    int red; // F8 680nm
    float flicker; // flicker frequency in hz
    int nir; // near infrared
    int clear; // broad spectrum light measurement
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
    LightComposition getCurrent();
    LightComposition getAccumulation();
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



}


void handleSensors();




#endif // PERIPHERALS_H