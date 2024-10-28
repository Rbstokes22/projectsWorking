#include "Peripherals/Soil.hpp"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_continuous.h"
#include "Peripherals/Relay.hpp"
#include "Threads/Mutex.hpp"
#include "UI/MsgLogHandler.hpp"
#include "string.h"

namespace Peripheral {

Soil::Soil(SoilParams &params) : 

    mtx(params.msglogerr), settings{
        {0, false, CONDITION::NONE}, 
        {0, false, CONDITION::NONE},
        {0, false, CONDITION::NONE},
        {0, false, CONDITION::NONE}
    }, params(params) {
    
    memset(this->readings, 0, SOIL_SENSORS);

}

// Requires void* parameter which will be cast to SoilParams*
// within the method. Default setting = nullptr. Must be init
// with a non nullptr to create the instance, and will return a 
// pointer to the instance upon proper completion.
Soil* Soil::get(void* parameter) {
    static bool isInit{false};
    if (parameter == nullptr && !isInit) {
        return nullptr; // Blocks instance from being created.
    } else if (parameter != nullptr) {
        isInit = true; // Opens gate after proper init
    }

    SoilParams* params = static_cast<SoilParams*>(parameter);
    static Soil instance(*params);
    
    return &instance;
}

// Requires the index number of the soil sensor, and returns a 
// pointer to its settings for modification.
SOIL_TRIP_CONFIG* Soil::getConfig(uint8_t indexNum) {
    return &settings[indexNum];
}

// Reads all sensors and stores the values in the class array
// called readings.
void Soil::readAll() {
    this->mtx.lock();
    for (int i = 0; i < SOIL_SENSORS; i++) {
        adc_oneshot_read(
            this->params.handle, 
            this->params.channels[i], 
            &this->readings[i]
            );
    }

    this->mtx.unlock();
}   

// Requires an int array and its size in bytes. This copies
// the current readings into the array. This function does 
// not return the array in order to take full advantage of 
// the mutex.
void Soil::getAll(int* readings, size_t bytes) {
    this->mtx.lock();
    memcpy(readings, this->readings, bytes);
    this->mtx.unlock();
}

// Requires no parameters. Checks the configuration setting
// for each sensor, and if tripped, handles the alert. 
void Soil::checkBounds() {

    // Checks condition to see if it trips the settings, and
    // handles the alert.
    auto chkCondition = [this](int val, SOIL_TRIP_CONFIG &conf){
 
        switch (conf.condition) {
     
            case CONDITION::LESS_THAN:
           
            if (val < conf.tripValAlert) {
                this->handleAlert(conf, true);
            } else {
                this->handleAlert(conf, false); 
            }
            break;

            case CONDITION::GTR_THAN:
           
            if (val > conf.tripValAlert) {
                this->handleAlert(conf, true);
            } else {
                this->handleAlert(conf, false); 
            }
            break;

            case CONDITION::NONE:
            break;
        }
    };

    for (int i = 0; i < SOIL_SENSORS; i++) {
        chkCondition(this->readings[i], this->settings[i]);
    }
}

// Requires SOIL_TRIP_CONFIG and whethere to employ the alert or not.
void Soil::handleAlert(SOIL_TRIP_CONFIG &config, bool alertOn) {
    // CONFIGURE EVERYTHING HERE ONCE BUILT
}



}