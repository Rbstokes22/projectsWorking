#include "Peripherals/TempHum.hpp"
#include "driver/gpio.h"
#include "Peripherals/Relay.hpp"

namespace Peripheral {

TempHum::TempHum(TempHumParams &params) : 

    temp(0.0f), hum(0.0f), averages{0, 0, 0}, 
    flags{false, false}, mtx(params.msglogerr),
    humConf{0, 0, false, CONDITION::NONE, nullptr, 0, 0},
    tempConf{0, 0, false, CONDITION::NONE, nullptr, 0, 0},
    params(params) {}

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
    const size_t errCtMax{5};
    bool read{false};

    this->mtx.lock();
    try {
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
            this->flags.immediate = false;
            errCt++;
        }

        if (errCt >= errCtMax) {
            this->flags.display = false;
            errCt = 0;
        }

    } catch(...) {
        this->mtx.unlock();
        throw; // rethrow exception.
    }   
    
    this->mtx.unlock();
    
    return this->flags.immediate;
}

void TempHum::getHum(float &hum) {
    this->mtx.lock();
    try {
        hum = this->hum;
    } catch(...) {
        this->mtx.unlock();
        throw; // rethrow exception.
    }
    
    this->mtx.unlock();
}

void TempHum::getTemp(float &temp) {
    this->mtx.lock();
    try {
        temp = this->temp;
    } catch(...) {
        this->mtx.unlock();
        throw; // rethrow exception.
    }
    
    this->mtx.unlock();
}

TH_TRIP_CONFIG* TempHum::getHumConf() {
    return &this->humConf;
}

TH_TRIP_CONFIG* TempHum::getTempConf() {
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

        switch (conf.condition) {
     
            case CONDITION::LESS_THAN:
            if (val < tripValueRelay) {
                this->handleRelay(conf, true);
            } else if (val >= upperBound) {
                this->handleRelay(conf, false);
            }

            if (val < tripValueAlert) {
                this->handleAlert(conf, true);
            } else {
                this->handleAlert(conf, false); 
            }
            break;

            case CONDITION::GTR_THAN:
            if (val > tripValueRelay) {
                this->handleRelay(conf, true);
            } else if (val <= lowerBound) {
                this->handleRelay(conf, false);
            }

            if (val > tripValueAlert) {
                this->handleAlert(conf, true);
            } else {
                this->handleAlert(conf, false); 
            }
            break;

            case CONDITION::NONE:
            break;
        }
    };

    chkCondition(this->temp, this->tempConf);
    chkCondition(this->hum, this->humConf);
}

void TempHum::handleRelay(TH_TRIP_CONFIG &config, bool relayOn) {
    // This is used because If the relay is set to none from the client,
    // nullptr will be chosen, this is by design and doesnt need err handling.
    // This also returns if the immediate flag is false, which prevent relay
    // from activating if data is garbage due to read error.
    if (config.relay == nullptr || !this->flags.immediate) {
        return;
    }

    if (relayOn) {
        config.relay->on(config.relayControlID);
    } else {
        config.relay->off(config.relayControlID);
    }
}

void TempHum::handleAlert(TH_TRIP_CONFIG &config, bool alertOn) { 
    // Build this once the alert.hpp/cpp is built.
}

isUpTH TempHum::getStatus() {
    return this->flags;
}

TH_Averages* TempHum::getAverages(bool reset) {
    TH_Averages avs;

    this->mtx.lock();
    try {
        avs = this->averages;
    } catch(...) {
        this->mtx.unlock();
        throw; // rethrow exception
    }

    this->mtx.unlock();

    if (reset) { // used to get daily average and clear.
        this->averages.hum = 0.0f;
        this->averages.temp = 0.0f;
        this->averages.pollCt = 0;
    }

    return &avs;
}

}