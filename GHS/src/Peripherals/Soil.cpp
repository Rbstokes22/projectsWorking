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
    memset(this->flags, false, sizeof(this->flags));
}

// Requires void* parameter which will be cast to SoilParams*
// within the method. Default setting = nullptr. Must be init
// with a non nullptr to create the instance, and will return a 
// pointer to the instance upon proper completion.
Soil* Soil::get(SoilParams* parameter) {
    static bool isInit{false};
    
    if (parameter == nullptr && !isInit) {
        return nullptr; // Blocks instance from being created.
    } else if (parameter != nullptr) {
        isInit = true; // Opens gate after proper init
    }

    static Soil instance(*parameter);
    
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
    esp_err_t err;
    static size_t errCt[SOIL_SENSORS]{0};
    const size_t errCtMax{5};

    this->mtx.lock();
    try {
        for (int i = 0; i < SOIL_SENSORS; i++) {
            err = adc_oneshot_read(
                this->params.handle, 
                this->params.channels[i], 
                &this->readings[i]
                );

            if (err == ESP_OK) {
                this->flags[i].display = true;
                this->flags[i].immediate = true;
                errCt[i] = 0;

            } else {
                this->flags[i].immediate = false;
                errCt[i]++;
            }

            if (errCt[i] >= errCtMax) {
                this->flags[i].display = false;
                errCt[i] = 0;
            }
        }
    } catch(...) {
        this->mtx.unlock();
        throw; // rethrow exception
    }
    
    this->mtx.unlock();
}   

// Requires an int array and its size in bytes. This copies
// the current readings into the array. This function does 
// not return the array in order to take full advantage of 
// the mutex.
void Soil::getAll(int* readings, size_t bytes) {
    this->mtx.lock();
    try {
        memcpy(readings, this->readings, bytes);
    } catch(...) {
        this->mtx.unlock();
        throw; // rethrow exception
    }
    
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
    // CONFIGURE EVERYTHING HERE ONCE BUILT, check for flags.immediate
}

isUpSoil* Soil::getStatus(uint8_t indexNum) { // pointer return to handle index err.
    if (indexNum > (SOIL_SENSORS - 1)) return nullptr;
    return &this->flags[indexNum];
}



}