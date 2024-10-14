#include "Peripherals/TempHum.hpp"
#include "driver/gpio.h"
#include "Peripherals/Relay.hpp"

namespace Peripheral {

// STATIC SETUP
float TempHum::temp = 0.0f;
float TempHum::hum = 0.0f;
RELAY_CONFIG TempHum::humConf = {0, nullptr, CONDITION::LESS_THAN};
RELAY_CONFIG TempHum::tempConf = {0, nullptr, CONDITION::LESS_THAN};
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

void TempHum::setHumConf(RELAY_CONFIG &config) {
    TempHum::humConf.tripVal = config.tripVal;
    TempHum::humConf.relay = config.relay;
    TempHum::humConf.condition = config.condition;
}

void TempHum::setTempConf(RELAY_CONFIG &config) {
    TempHum::tempConf.tripVal = config.tripVal;
    TempHum::tempConf.relay = config.relay;
    TempHum::tempConf.condition = config.condition;
}

void TempHum::checkBounds() {

    if (humConf.relay == nullptr) {
        printf("Humidity not configured\n");
        return;
    }

    if (tempConf.relay == nullptr) {
        printf("Temperature not configured\n");
        return;
    }

    // Truns relay on when the trip value and condition is met.
    // When the padded bound is reached, turns the relay off.
    auto chkCondition = [this](float val, RELAY_CONFIG &conf){
        float tripValue = static_cast<float>(conf.tripVal);
        float lowerBound = tripValue - TEMP_HUM_PADDING;
        float upperBound = tripValue + TEMP_HUM_PADDING;

        switch (conf.condition) {
     
            case CONDITION::LESS_THAN:
            if (val < tripValue) {
                this->handleRelay(conf, true);
            } else if (val >= upperBound) {
                this->handleRelay(conf, false);
            }
            break;

            case CONDITION::GTR_THAN:
            if (val > tripValue) {
                this->handleRelay(conf, true);
            } else if (val <= lowerBound) {
                this->handleRelay(conf, false);
            }
            break;
        }
    };

    chkCondition(TempHum::temp, TempHum::tempConf);
    chkCondition(TempHum::hum, TempHum::humConf);
}

void TempHum::handleRelay(RELAY_CONFIG &config, bool relayOn) {
    if (config.relay == nullptr) {
        printf("Config must not equal nullptr\n");
        return;
    }

    if (relayOn) {
        config.relay->on(config.relayControlID);
    } else {
        config.relay->off(config.relayControlID);
    }
}

void TempHum::setStatus(bool isUp) {
    TempHum::isUp = isUp;
}

bool TempHum::getStatus() {
    return TempHum::isUp;
}

}