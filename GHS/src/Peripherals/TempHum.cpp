#include "Peripherals/TempHum.hpp"
#include "driver/gpio.h"
#include "Peripherals/Relay.hpp"

namespace Peripheral {

// STATIC SETUP
float TempHum::temp = 0.0f;
float TempHum::hum = 0.0f;
BOUNDARY_CONFIG TempHum::humConf = {0, 0, false, CONDITION::NONE, nullptr, 0, 0};
BOUNDARY_CONFIG TempHum::tempConf = {0, 0, false, CONDITION::NONE, nullptr, 0, 0};
bool TempHum::isUp = false;

float TempHum::getHum() {
    return TempHum::hum;
}

float TempHum::getTemp() {
    return TempHum::temp;
}

void TempHum::setHum(float val) {
    TempHum::hum = val;
}

void TempHum::setTemp(float val) {
    TempHum::temp = val;
}

BOUNDARY_CONFIG* TempHum::getHumConf() {
    return &TempHum::humConf;
}

BOUNDARY_CONFIG* TempHum::getTempConf() {
    return &TempHum::tempConf;
}

void TempHum::checkBounds() {

    // Turns relay on when the trip value and condition is met.
    // When the padded bound is reached, turns the relay off.
    auto chkCondition = [this](float val, BOUNDARY_CONFIG &conf){
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
                this->handleAlert();
            } else {
                this->handleAlert(); // Add a boolean or something to reset
            }
            break;

            case CONDITION::GTR_THAN:
            if (val > tripValueRelay) {
                this->handleRelay(conf, true);
            } else if (val <= lowerBound) {
                this->handleRelay(conf, false);
            }

            if (val > tripValueAlert) {
                this->handleAlert();
            } else {
                this->handleAlert(); // Add a boolean or something to reset
            }
            break;

            case CONDITION::NONE:
            break;
        }
    };

    chkCondition(TempHum::temp, TempHum::tempConf);
    chkCondition(TempHum::hum, TempHum::humConf);
}

void TempHum::handleRelay(BOUNDARY_CONFIG &config, bool relayOn) {
    // This is used because If the relay is set to none from the client,
    // nullptr will be chosen, this is by design and doesnt need err handling.
    if (config.relay == nullptr) {
        return;
    }

    if (relayOn) {
        config.relay->on(config.relayControlID);
    } else {
        config.relay->off(config.relayControlID);
    }
}

void TempHum::handleAlert() { // if alerts enable == true logic here
    // Build this once the alert.hpp/cpp is built.
}

void TempHum::setStatus(bool isUp) {
    TempHum::isUp = isUp;
}

bool TempHum::getStatus() {
    return TempHum::isUp;
}

}