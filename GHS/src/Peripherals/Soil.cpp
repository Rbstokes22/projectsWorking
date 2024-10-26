#include "Peripherals/Soil.hpp"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_continuous.h"
#include "Peripherals/Relay.hpp"
#include "Threads/Mutex.hpp"
#include "UI/MsgLogHandler.hpp"
#include "string.h"

namespace Peripheral {

Soil::Soil(Messaging::MsgLogHandler* msglogerr) : 

    flags{false, false, false}, mtx(*msglogerr),
    msglogerr(msglogerr), settings{
        {0, 0, false, CONDITION::NONE, nullptr, 0, 0}, 
        {0, 0, false, CONDITION::NONE, nullptr, 0, 0}, 
        {0, 0, false, CONDITION::NONE, nullptr, 0, 0}, 
        {0, 0, false, CONDITION::NONE, nullptr, 0, 0}
    }, channels(nullptr) {
    
    memset(this->readings, 0, SOIL_SENSORS);
    if (msglogerr != nullptr) this->flags.msgLogErr = true;
}

// Requires reference to msglogerr. Defaults to nullptr, and the first call
// must contain the appropriate reference to the msglogerr. Returns pointer
// of the instance.
Soil* Soil::getInstance(Messaging::MsgLogHandler* msglogerr) {
    static Soil instance(msglogerr);
    return &instance;
}

void Soil::setHandle(adc_oneshot_unit_handle_t handle) {
    this->handle = handle;
    this->flags.handle = true;
}

// Must pass array of adc channels that do not go out of scope.
void Soil::setChannels(adc_channel_t* channels) {
    if (channels == nullptr) {
        printf("Soil.cpp Channels cannot = nullptr\n");
        return;
    }

    this->channels = channels;
}

BOUNDARY_CONFIG* Soil::getConfig(uint8_t indexNum) {
    return &settings[indexNum];
}

void Soil::readAll() {
    this->mtx.lock();
    for (int i = 0; i < SOIL_SENSORS; i++) {
        adc_oneshot_read(this->handle, this->channels[i], &this->readings[i]);
    }
    this->mtx.unlock();
}   

// Must equal the quantity of readings expected
void Soil::getAll(int* readings, size_t size) {
    this->mtx.lock();
    memcpy(readings, this->readings, size);
    this->mtx.unlock();
}

void Soil::checkBounds() {
    // Turns relay on when the trip value and condition is met.
    // When the padded bound is reached, turns the relay off.
    // auto chkCondition = [this](float val, BOUNDARY_CONFIG &conf){
    //     float tripValueRelay = static_cast<float>(conf.tripValRelay);
    //     float lowerBound = tripValueRelay - TEMP_HUM_PADDING;
    //     float upperBound = tripValueRelay + TEMP_HUM_PADDING;
    //     float tripValueAlert = static_cast<float>(conf.tripValAlert);

    //     switch (conf.condition) {
     
    //         case CONDITION::LESS_THAN:
    //         if (val < tripValueRelay) {
    //             this->handleRelay(conf, true);
    //         } else if (val >= upperBound) {
    //             this->handleRelay(conf, false);
    //         }

    //         if (val < tripValueAlert) {
    //             this->handleAlert();
    //         } else {
    //             this->handleAlert(); // Add a boolean or something to reset
    //         }
    //         break;

    //         case CONDITION::GTR_THAN:
    //         if (val > tripValueRelay) {
    //             this->handleRelay(conf, true);
    //         } else if (val <= lowerBound) {
    //             this->handleRelay(conf, false);
    //         }

    //         if (val > tripValueAlert) {
    //             this->handleAlert();
    //         } else {
    //             this->handleAlert(); // Add a boolean or something to reset
    //         }
    //         break;

    //         case CONDITION::NONE:
    //         break;
    //     }
    // };

    // chkCondition(TempHum::temp, TempHum::tempConf);
    // chkCondition(TempHum::hum, TempHum::humConf);
}

void Soil::handleRelay() {

}

void Soil::handleAlert() {

}

bool Soil::isInit() {
    uint8_t totalReq{3};

    bool required[totalReq] = {
        this->flags.msgLogErr, this->flags.handle, this->flags.channels
        };

    for (uint8_t i = 0; i < totalReq; i++) {
        if (required[i] == false) return false;
    }

    return true;
}

}