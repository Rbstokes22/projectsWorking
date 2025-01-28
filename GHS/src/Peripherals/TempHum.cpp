#include "Peripherals/TempHum.hpp"
#include "driver/gpio.h"
#include "Peripherals/Relay.hpp"
#include "Peripherals/Alert.hpp"
#include "Network/NetCreds.hpp"

namespace Peripheral {

TempHum::TempHum(TempHumParams &params) : 

    data{0.0f, 0.0f, 0.0f, true}, averages{0, 0, 0, 0, 0}, 
    flags{false, true}, // !!!change immediate flag to true for testing
    mtx(params.msglogerr), 
    humConf{{0, ALTCOND::NONE, ALTCOND::NONE, 0, 0},
            {0, RECOND::NONE, RECOND::NONE, nullptr, 0, 0, 0, 0}},

    tempConf{{0, ALTCOND::NONE, ALTCOND::NONE, 0, 0},
            {0, RECOND::NONE, RECOND::NONE, nullptr, 0, 0, 0, 0}},

    params(params) {}

// NOTES HERE ONCE TESTED
void TempHum::handleRelay(relayConfig &conf, bool relayOn, uint32_t ct) {
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
        if (conf.relay == nullptr || !this->flags.immediate ||
        ct < TEMP_HUM_CONSECUTIVE_CTS || conf.condition == RECOND::NONE) {
            return;
        }

        conf.relay->on(conf.controlID);
        break;

        case false:
        if (!this->flags.immediate || ct < TEMP_HUM_CONSECUTIVE_CTS) {
            return;
        }
        
        conf.relay->off(conf.controlID);
        break;
    }
}

void TempHum::handleAlert(alertConfig &conf, bool alertOn, uint32_t ct) { 
    Threads::MutexLock(this->mtx);

    // Toggle that will not allow alerts to be sent several times. When an 
    // alert is sent, this is changed to false, and will not change back to
    // true until temp and/or hum, has entered back into acceptable values.
    static bool altToggle = true;
    static uint8_t attempts = 0;

    // Mirrors the same setup as the relay activity.
    if (!this->flags.immediate || ct < TEMP_HUM_CONSECUTIVE_CTS || 
        conf.condition == ALTCOND::NONE) { 
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

        // Upon success, set to false.
        altToggle = !alt->sendAlert(sms->APIkey, sms->phone, msg);
        
        if (!altToggle) {
            printf("Alert: Message Sent.\n");
            attempts = 0; // Resets value
        } else {
            printf("Alert: Message Not Sent.\n");
            attempts++; // Increase value to avoid oversending
        }

        // For an unsuccessful server response, after reaching max attempt 
        // value, set to false to prevent further trying. Will reset once 
        // the temp or hum is within its acceptable values.
        if (attempts >= ALT_MSG_ATT) {
            altToggle = false;
        }

    } else {
        // Resets the toggle when the alert is set to off meaning that
        // satisfactory conditions prevail. This will allow a message
        // to be sent if it exceeds the limitations.
        altToggle = true;
        attempts = 0;
    }
}

void TempHum::relayBounds(float value, relayConfig &conf) {
    Threads::MutexLock(this->mtx);

    float tripVal = static_cast<float>(conf.tripVal);
    float lowerBound = tripVal - TEMP_HUM_HYSTERESIS; 
    float upperBound = tripVal + TEMP_HUM_HYSTERESIS;

    // Checks to see if the relay conditions have changed. If true,
    // resets the counts and changes the previous condition to current
    // condition.
    if (conf.condition != conf.prevCondition) {
        conf.onCt = 0; conf.offCt = 0;
        conf.prevCondition = conf.condition;
    }

    switch (conf.condition) {
        case RECOND::LESS_THAN:
        if (value < tripVal) {
            conf.onCt++; 
            conf.offCt = 0;
            this->handleRelay(conf, true, conf.onCt);

        } else if (value >= upperBound) {
            conf.onCt = 0;
            conf.offCt++;
            this->handleRelay(conf, false, conf.offCt);
        }        
        break;

        case RECOND::GTR_THAN:
            if (value > tripVal) {
                conf.onCt++;
                conf.offCt = 0;
                this->handleRelay(conf, true, conf.onCt);

            } else if (value <= lowerBound) {
                conf.onCt = 0;
                conf.offCt++;
                this->handleRelay(conf, false, conf.offCt);
            }
        break;

        case RECOND::NONE: // Placeholder but required
        // If set to none, relay will be detached from the sensor.
        break;
    }
}

void TempHum::alertBounds(float value, alertConfig &conf) {
    Threads::MutexLock(this->mtx);

    float tripVal = static_cast<float>(conf.tripVal); 
    float lowerBound = tripVal - TEMP_HUM_HYSTERESIS;
    float upperBound = tripVal + TEMP_HUM_HYSTERESIS;

    // Checks to see if the alert conditions have changed. If true,
    // resets the counts and changes the previous condition to current
    // condition.
    if (conf.condition != conf.prevCondition) {
        conf.onCt = 0; conf.offCt = 0;
        conf.prevCondition = conf.condition;
    }

    switch (conf.condition) {
        case ALTCOND::LESS_THAN:
        if (value < tripVal) {
            conf.onCt++;
            conf.offCt = 0;
            this->handleAlert(conf, true, conf.onCt);

        } else if (value >= upperBound) { 
            conf.onCt = 0;
            conf.offCt++;
            this->handleAlert(conf, false, conf.offCt); 
        }        
        break;

        case ALTCOND::GTR_THAN:
        if (value > tripVal) {
            conf.onCt++;
            conf.offCt = 0;
            this->handleAlert(conf, true, conf.onCt);

        } else if (value <= lowerBound) {
            conf.onCt = 0;
            conf.offCt++;
            this->handleAlert(conf, false, conf.offCt); 
        }
        break;

        case ALTCOND::NONE: // Placeholder
        break;
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

    // Returns true of data is ok.
    return this->flags.immediate;
}

float TempHum::getHum() {
    Threads::MutexLock(this->mtx);
    return this->data.hum;
}

// Defaults to Celcius.
float TempHum::getTemp(char CorF) { // Cel or Faren
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

// Best to call this after read like "if (read) checkbounds()" due
// to the handle relay call count.
bool TempHum::checkBounds() { 
    
    if (!this->data.dataSafe) return false; // Filters bad data.

    // Checks each individual bound after confirming data is safe.
    this->relayBounds(this->data.tempC, this->tempConf.relay);
    this->relayBounds(this->data.hum, this->humConf.relay);
    this->alertBounds(this->data.tempC, this->tempConf.alt);
    this->alertBounds(this->data.hum, this->humConf.alt);

    return true;
}

isUpTH TempHum::getStatus() {
    return this->flags;
}

TH_Averages* TempHum::getAverages() { 
    Threads::MutexLock(this->mtx);
    return &this->averages;
}

// Clears the current data after copying it over to the previous values.
void TempHum::clearAverages() {
    Threads::MutexLock(this->mtx);
    this->averages.prevHum = this->averages.hum;
    this->averages.prevTemp = this->averages.temp;
    this->averages.hum = 0.0f;
    this->averages.temp = 0.0f;
    this->averages.pollCt = 0;
}

void TempHum::test(bool isTemp, float val) { // !!!COMMENT OUT WHEN NOT USED
    Threads::MutexLock(this->mtx);
    if (isTemp) {
        this->data.tempC = val;
    } else {
        this->data.hum = val;
    }
    this->checkBounds(); 
}

}