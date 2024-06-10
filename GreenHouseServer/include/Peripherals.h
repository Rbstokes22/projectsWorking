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

#define DHT_ADDRESS 0x5C

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


extern DHT dht;
extern Adafruit_AS7341 as7341;
void handleSensors();




#endif // PERIPHERALS_H