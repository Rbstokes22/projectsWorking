#include "Peripherals/TempHum.hpp"
#include "driver/gpio.h"
#include "Peripherals/Relay.hpp"
#include "Peripherals/Alert.hpp"

namespace Peripheral {

TempHum::TempHum(TempHumParams &params) : 

    temp(0.0f), hum(0.0f), averages{0, 0, 0}, 
    flags{false, false}, mtx(params.msglogerr),

    humConf{0, 0, false, CONDITION::NONE, CONDITION::NONE, 
        nullptr, 0, 0, 0, 0, 0, 0},

    tempConf{0, 0, false, CONDITION::NONE, CONDITION::NONE, 
        nullptr, 0, 0, 0, 0, 0, 0},

    params(params) {}

void TempHum::handleRelay(TH_TRIP_CONFIG &config, bool relayOn, uint32_t ct) {
    // Returns to prevent relay activity if:
    //  1. relay is set to none, which will set it to nullptr.
    //  2. immediate flag is false, indicating in a DHT read error
    //  preventing false triggers.
    //  3. count of consecutive triggers is less than setting.
    if (
        config.relay == nullptr || 
        !this->flags.immediate || 
        ct < TEMP_HUM_CONSECUTIVE_CTS) {
        return;
    }

    if (relayOn) {
        config.relay->on(config.relayControlID);
    } else {
        config.relay->off(config.relayControlID);
    }
}

void TempHum::handleAlert(TH_TRIP_CONFIG &config, bool alertOn, uint32_t ct) { 
    // Build this once the alert.hpp/cpp is built.
    // This is going to ping webpage instead of twilio so use http
    // client to build.
}

TempHum* TempHum::get(TempHumParams* parameter) {
    static bool isInit{false};

    if (parameter == nullptr && !isInit) {
        return nullptr; // Blocks instance from being created.
    } else if (parameter != nullptr) {
        isInit = true; // Opens gate after proper init
    }

    static TempHum instance(*parameter);
    
    return &instance;
}

bool TempHum::read() {
    static size_t errCt{0};
    bool read{false};

    Threads::MutexLock(this->mtx);

    read = this->params.dht.read(this->temp, this->hum);

    if (read) {
            this->averages.pollCt++;
            this->averages.temp += this->temp; // accumulate
            this->averages.temp /= this->averages.pollCt; // average
            this->averages.hum += this->hum;
            this->averages.hum /= this->averages.pollCt;
            this->flags.display = true;
            this->flags.immediate = true;
            errCt = 0;
        } else {
            this->flags.immediate = false; // Indicates error
            errCt++;
        }

        if (errCt >= TEMP_HUM_ERR_CT_MAX) {
            this->flags.display = false; // Indicates error on display
            errCt = 0;
        }

    return this->flags.immediate;
}

float TempHum::getHum() {
    Threads::MutexLock(this->mtx);
    return this->hum;
}

float TempHum::getTemp() {
    Threads::MutexLock(this->mtx);
    return this->temp; 
}

TH_TRIP_CONFIG* TempHum::getHumConf() {
    Threads::MutexLock(this->mtx);
    return &this->humConf;
}

TH_TRIP_CONFIG* TempHum::getTempConf() {
    Threads::MutexLock(this->mtx);
    return &this->tempConf;
}

void TempHum::checkBounds() {

    // Turns relay on when the trip value and condition is met.
    // When the padded bound is reached, turns the relay off.
    auto chkCondition = [this](float val, TH_TRIP_CONFIG &conf){
        float tripValueRelay = static_cast<float>(conf.tripValRelay);
        float lowerBound = tripValueRelay - TEMP_HUM_PADDING;
        float upperBound = tripValueRelay + TEMP_HUM_PADDING;
        float tripValueAlert = static_cast<float>(conf.tripValAlert);

        // Zeros out all counts if the condition is changed.
        if (conf.condition != conf.prevCondition) {
            conf.relayOnCt = 0;
            conf.relayOffCt = 0;
            conf.alertOnCt = 0;
            conf.alertOffCt = 0;
        }

        conf.prevCondition = conf.condition; // set prev to current

        switch (conf.condition) { 

            // The configuration relay/alert on and off counts accumulate
            // and/or zero, and send the applicable count to the 
            // handleRelays function. This is to prevent relay or alert
            // activity from a single value, and ensures consecutive
            // triggers are met.

            case CONDITION::LESS_THAN: 

                if (val < tripValueRelay) {
                    conf.relayOnCt++;
                    conf.relayOffCt = 0;
                    this->handleRelay(conf, true, conf.relayOnCt);
                } else if (val >= upperBound) {
                    conf.relayOnCt = 0;
                    conf.relayOffCt++;
                    this->handleRelay(conf, false, conf.relayOffCt);
                }

                if (val < tripValueAlert) {
                    conf.alertOnCt++;
                    conf.alertOffCt = 0;
                    this->handleAlert(conf, true, conf.alertOnCt);
                } else { // Alerts turning off will reset to allow follow on alerts
                    conf.alertOnCt = 0;
                    conf.alertOffCt++;
                    this->handleAlert(conf, false, conf.alertOffCt); 
                }

                break;
            
            
            case CONDITION::GTR_THAN: 

                if (val > tripValueRelay) {
                    conf.relayOnCt++;
                    conf.relayOffCt = 0;
                    this->handleRelay(conf, true, conf.relayOnCt);
                } else if (val <= lowerBound) {
                    conf.relayOnCt = 0;
                    conf.relayOffCt++;
                    this->handleRelay(conf, false, conf.relayOffCt);
                }

                if (val > tripValueAlert) {
                    conf.alertOnCt++;
                    conf.alertOffCt = 0;
                    this->handleAlert(conf, true, conf.alertOnCt);
                } else {;
                conf.alertOnCt = 0;
                    conf.alertOnCt = 0;
                    conf.alertOffCt++;
                    this->handleAlert(conf, false, conf.alertOffCt); 
                }

                break;

            case CONDITION::NONE:
                break;
        }
    };

    chkCondition(this->temp, this->tempConf);
    chkCondition(this->hum, this->humConf);
}

isUpTH TempHum::getStatus() {
    return this->flags;
}

TH_Averages* TempHum::getAverages() { 
    Threads::MutexLock(this->mtx);
    return &this->averages;
}

void TempHum::clearAverages() {
    this->averages.hum = 0.0f;
    this->averages.temp = 0.0f;
    this->averages.pollCt = 0;
}

void TempHum::test() {
    Alert* alt = Alert::get();
    bool testing = alt->sendMessage(
        "FF022286",
        "7346740543",
        "Temperature exceeding 90 Degrees"
    );

    printf("Message Sent %d\n", testing);
}

}