#include <Arduino.h>
#include "espCom.h"
#include "sensors.h"

// Values and commands pasted from nodeMCU master
#define CHECK_ALL 0

#define TEMP_HUM 1
#define GET_TEMP_HUM 2

#define SOIL_1 3
#define GET_SOIL_1 4

#define ACTIVATE_RELAY_1 5
#define DEACTIVATE_RELAY_1 6

// Pins


class Timer {
    private:
    unsigned long previousMillis;
    unsigned long interval;

    public:
    Timer(unsigned long interval) : previousMillis(millis()), interval(interval){};

    bool isOverdue(){
        unsigned long int currentMillis = millis();
        if (currentMillis - this->previousMillis >= this->interval) {
            this->previousMillis = currentMillis;
            return true;
        } else {
            return false;
        }
    }

    // Allows dynamic change to allow client to speed or slow data stream
    void changeInterval(unsigned long interval) {
        this->interval = interval;
    }
};

Timer nextResponse(60000); // Use to setup LED that goes on if delayed past expected

void setup() {
    
  
}

void loop() {
    
}



