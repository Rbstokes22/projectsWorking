#include "Peripherals/TempHum.hpp"
#include "driver/gpio.h"
#include "Peripherals/Relay.hpp"
#include "Peripherals/Alert.hpp"
#include "Network/NetCreds.hpp"

namespace Peripheral {

TempHum::TempHum(TempHumParams &params) : 

    data{0.0f, 0.0f, 0.0f, false}, averages{0, 0, 0}, 
    flags{false, false}, mtx(params.msglogerr),

    humConf{0, 0, false, CONDITION::NONE, CONDITION::NONE, 
        nullptr, 0, 0, 0, 0, 0, 0},

    tempConf{0, 0, false, CONDITION::NONE, CONDITION::NONE, 
        nullptr, 0, 0, 0, 0, 0, 0},

    params(params) {}


// RELAY AND ALERT CONSIDERATIONS. THINGS NEED BE BE CHANGED HERE IN ORDER TO ENSURE THE RELAY
// WILL SOMEHOW BE SHUT OFF EVEN IF THE CONFIGURATION CHANGES. MAYBE MEET ALL CONDITIONS TO TURN
// RELAY ON, AND ONLY COUNTS TO SHUT IT OFF?
void TempHum::handleRelay(TH_TRIP_CONFIG &config, bool relayOn, uint32_t ct) {
    Threads::MutexLock(this->mtx);
    // Gate starts with relayOn (true) or relayOn (false), which shuts it off.
    // Returns to prevent relay activity if:
    // RELAY ON:
    //  1. Relay is set to none, which sets it to nullptr and turns it off.
    //  2. Immediate flag is false, indicating an SHT read error preventing
    //     false triggers.
    //  3. Count of consecutive trigger criteria being met is less than setting.
    //  4. The condition to trigger the relay is set to none.
    // RELAY OFF:
    //  1. Immediate flag is false, indiciating an SHT read error preventing
    //     false triggers.
    //  2. Count of consecutive trigger criteria being met is less than setting.
    // This is to prevent an active relay from being unable to shut off if its
    // conditions were changed, which means that if the data is good
    switch (relayOn) {
        case true:
        if (config.relay == nullptr || !this->flags.immediate ||
        ct < TEMP_HUM_CONSECUTIVE_CTS || config.condition == CONDITION::NONE) {
            return;
        }

        config.relay->on(config.relayControlID);
        break;

        case false:
        if (!this->flags.immediate || ct < TEMP_HUM_CONSECUTIVE_CTS) {
            return;
        }

        config.relay->off(config.relayControlID);
        break;
    }
}

void TempHum::handleAlert(TH_TRIP_CONFIG &config, bool alertOn, uint32_t ct) { 
    Threads::MutexLock(this->mtx);

    // Toggle that will not allow alerts to be sent several times. When an 
    // alert is sent, this is changed to false, and will not change back to
    // true until temp and/or hum, has entered back into acceptable values.
    static bool altToggle = true;

    // Mirrors the same setup as the relay activity.
    if (!config.alertsEn || !this->flags.immediate ||
        ct < TEMP_HUM_CONSECUTIVE_CTS || config.condition == CONDITION::NONE) { 
        return;
    }

    // Check to see if the alert is being called to send. If yes, ensures that
    // the toggle is set to true. If yes to both, it will send the message and
    // change the altToggle to false preventing it from sending another message
    // without being reset. 
    if (alertOn) {
        if (!altToggle) return; // return if toggle hasnt been reset.

        NVS::SMSreq* sms = NVS::Creds::get()->getSMSReq();

        // Returned val is nullptr if API key and/or phone
        // do not meet the criteria.
        if (sms == nullptr) { 
            printf("Temp Hum Alerts: Not able to send, "
            "missing API key and/or phone\n");
            return;
        }

        char msg[75](0);
        Alert* alt = Alert::get(); 

        snprintf(msg, sizeof(msg), 
                "Alert: Temp at %0.2fC/%0.2fF, Humidity at %0.2f%%",
                this->data.tempC, this->data.tempF, this->data.hum
                );

        altToggle = alt->sendMessage(sms->APIkey, sms->phone, msg);

    } else {
        // Resets the toggle when the alert is set to off meaning that
        // satisfactory conditions prevail. This will allow a message
        // to be sent if it exceeds the limitations.
        altToggle = false;
    }
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
    SHT_DRVR::SHT_RET read;

    Threads::MutexLock(this->mtx);

    read = this->params.sht.readAll(
        SHT_DRVR::START_CMD::NSTRETCH_HIGH_REP, this->data
        );

    if (read == SHT_DRVR::SHT_RET::READ_OK) {
        this->averages.pollCt++;
        this->averages.temp += this->data.tempC; // accumulate
        this->averages.temp /= this->averages.pollCt; // average
        this->averages.hum += this->data.hum;
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
    return this->data.hum;
}

// Defaults to Celcius.
float TempHum::getTemp(char CorF) {
    Threads::MutexLock(this->mtx);
    if (CorF == 'F' || CorF == 'f') return this->data.tempF;
    return this->data.tempC;
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
    
    if (!this->data.dataSafe) return; // Filters bad data.
    Threads::MutexLock(this->mtx);
    // Turns relay on when the trip value and condition is met.
    // When the padded bound is reached, turns the relay off.
    auto chkCondition = [this](float val, TH_TRIP_CONFIG &conf){

        // Relay bounds. Will reverse action once appropriate bound
        // is met.
        float tripValueRelay = static_cast<float>(conf.tripValRelay);
        float lowerBoundRelay = tripValueRelay - TEMP_HUM_HYSTERESIS; 
        float upperBoundRelay = tripValueRelay + TEMP_HUM_HYSTERESIS;

        // Float bounds. same as relay.
        float tripValueAlert = static_cast<float>(conf.tripValAlert); 
        float lowerBoundAlert = tripValueAlert - TEMP_HUM_HYSTERESIS;
        float upperBoundAlert = tripValueAlert + TEMP_HUM_HYSTERESIS;
        

        // Zeros out all counts if the condition is changed.
        if (conf.condition != conf.prevCondition) {
            conf.relayOnCt = 0; conf.relayOffCt = 0;
            conf.alertOnCt = 0; conf.alertOffCt = 0;
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

                } else if (val >= upperBoundRelay) {
                    conf.relayOnCt = 0;
                    conf.relayOffCt++;
                    this->handleRelay(conf, false, conf.relayOffCt);
                }

                if (val < tripValueAlert) {
                    conf.alertOnCt++;
                    conf.alertOffCt = 0;
                    this->handleAlert(conf, true, conf.alertOnCt);

                } else if (val >= upperBoundAlert) { 
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

                } else if (val <= lowerBoundRelay) {
                    conf.relayOnCt = 0;
                    conf.relayOffCt++;
                    this->handleRelay(conf, false, conf.relayOffCt);
                }

                if (val > tripValueAlert) {
                    conf.alertOnCt++;
                    conf.alertOffCt = 0;
                    this->handleAlert(conf, true, conf.alertOnCt);

                } else if (val <= lowerBoundAlert) {
                    conf.alertOnCt = 0;
                    conf.alertOffCt++;
                    this->handleAlert(conf, false, conf.alertOffCt); 
                }

                break;

            case CONDITION::NONE:
            // If condition is changed to none, The relay will be detached
            // from the client.
                break;
        }
    };

    chkCondition(this->data.tempC, this->tempConf);
    chkCondition(this->data.hum, this->humConf);
}

isUpTH TempHum::getStatus() {
    return this->flags;
}

TH_Averages* TempHum::getAverages() { 
    Threads::MutexLock(this->mtx);
    return &this->averages;
}

void TempHum::clearAverages() {
    Threads::MutexLock(this->mtx);
    this->averages.hum = 0.0f;
    this->averages.temp = 0.0f;
    this->averages.pollCt = 0;
}

}