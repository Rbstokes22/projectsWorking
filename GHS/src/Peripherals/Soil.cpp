#include "Peripherals/Soil.hpp"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_continuous.h"
#include "Peripherals/Alert.hpp"
#include "Threads/Mutex.hpp"
#include "UI/MsgLogHandler.hpp"
#include "string.h"
#include "Network/NetCreds.hpp"

namespace Peripheral {

Soil::Soil(SoilParams &params) : 

    data{
        {0, false, false, 0}, {0, false, false, 0}, {0, false, false, 0}, 
        {0, false, false, 0}
    },
    mtx(params.msglogerr), conf{
        {0, ALTCOND::NONE, ALTCOND::NONE, 0, 0, false, 1, 0}, 
        {0, ALTCOND::NONE, ALTCOND::NONE, 0, 0, false, 2, 0}, 
        {0, ALTCOND::NONE, ALTCOND::NONE, 0, 0, false, 3, 0},
        {0, ALTCOND::NONE, ALTCOND::NONE, 0, 0, false, 4, 0}
    }, params(params) {}

// Requires SOIL_TRIP_CONFIG and whethere to employ the alert or not.
void Soil::handleAlert(SOIL_TRIP_CONFIG &conf, SoilReadings &data, 
    bool alertOn, uint32_t ct) {
    // DOES NOT REQUIRE MUTEX LOCK, LOCK IS APPLIED ON CHECK BOUNDS FUNCTION

    if (!data.immediate || ct < SOIL_CONSECUTIVE_CTS || 
        conf.condition == ALTCOND::NONE) return;

    if (alertOn) {
        if (!conf.toggle) return; // avoids repeated alerts.

        NVS::SMSreq* sms = NVS::Creds::get()->getSMSReq();

        if (sms == nullptr) {
            printf("Soil Alerts: Not able to send, missing API key and/or "
            "phone\n");
            return;
        }

        char msg[SOIL_ALT_MSG_SIZE] = {0}; // message to send to server
        Alert* alt = Alert::get();

        // write the message to the msg array, in preparation to send to
        // the server.
        snprintf(msg, sizeof(msg), 
                "Alert: Soil Sensor %u at value %d",conf.ID, data.val
                );

        // Upon success, set to false. If no success, it will keep trying until
        // attempts are maxed.
        conf.toggle = !alt->sendAlert(sms->APIkey, sms->phone, msg);

        if (!conf.toggle) {
            printf("Alert: Message Sent.\n");
            conf.attempts = 0; // Resets value
        } else {
            printf("Alert: Message Not Sent.\n");
            conf.attempts++; // Increase value to avoid oversending
        }

        // For an unsuccessful server response, after reaching max attempt 
        // value, set to false to prevent further trying. Will reset once 
        // the soil is within its acceptable values.
        if (conf.attempts >= SOIL_ALT_MSG_ATT) {
            conf.toggle = false;
        }

    } else {
        // Resets the toggle when the alert is set to off meaning that
        // satisfactory conditions prevail. This will allow a message
        // to be sent if it exceeds the limitations.
        conf.toggle = true;
        conf.attempts = 0;
    }
}

// Requires SoilParms* parameter.
// Default setting = nullptr. Must be init
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
// pointer to its conf for modification. Will return a nullptr if the 
// correct index value is not reached, and the configuration if it is correct.
SOIL_TRIP_CONFIG* Soil::getConfig(uint8_t indexNum) {
    if (indexNum >= (SOIL_SENSORS)) return nullptr;
    Threads::MutexLock(this->mtx);
    return &conf[indexNum];
}

// Reads all soil sensors and stores the data in the data variable.
void Soil::readAll() {
    esp_err_t err;
    Threads::MutexLock(this->mtx);

    for (int i = 0; i < SOIL_SENSORS; i++) {
        err = adc_oneshot_read(
            this->params.handle, this->params.channels[i], &this->data[i].val
            );

        // Sets the proper values based on read success.
        if (err == ESP_OK) {
            this->data[i].display = true;
            this->data[i].immediate = true;
            this->data[i].errCt = 0;

        } else {
            this->data[i].immediate = false;
            this->data[i].errCt++;
        }

        // Sets the display flag to true if error count is below max.
        this->data[i].display = (this->data[i].errCt < SOIL_ERR_MAX);
    }
}   

// Requires the sensor number you are trying to read. Returns pointer to the
// soil reading data if correct sensor index value is used, and nullptr if not.
SoilReadings* Soil::getAll(uint8_t indexNum) {
    if (indexNum >= (SOIL_SENSORS)) return nullptr;
    Threads::MutexLock(this->mtx);
    return &this->data[indexNum];
}

// Requires no parameters. Checks the configuration setting for each sensor
// and compares it against the actual value to trip the alert if configured
// to do so.
void Soil::checkBounds() {
    Threads::MutexLock(this->mtx);

    // Iterates through each of the soil sensors to check its current value
    // against the value set to trip the alarm.
    for (int i = 0; i < SOIL_SENSORS; i++) {
        int lowerBound = this->conf[i].tripVal - SOIL_HYSTERESIS;
        int upperBound = this->conf[i].tripVal + SOIL_HYSTERESIS;

        // Checks the configuration condition to check for a change. If changed
        // resets the on and off counts to 0, and changes the current condition
        // to the previous condition.
        if (this->conf[i].condition != this->conf[i].prevCondition) {
            this->conf[i].onCt = 0; 
            this->conf[i].offCt = 0;
            this->conf[i].prevCondition = this->conf[i].condition;
        }

        // One the criteria has been met, the counts are incremeneted for each
        // consecutive count only. These counts are passed to the alert handler
        // and once conditions are met, will send and alert or reset the toggle
        // allowing follow-on alerts for the same criteria being met.
        switch (this->conf[i].condition) {

            case ALTCOND::LESS_THAN:
            if (this->data[i].val < this->conf[i].tripVal) {
                this->conf[i].onCt++;
                this->conf[i].offCt = 0;
                this->handleAlert(this->conf[i], this->data[i], true, 
                    this->conf[i].onCt);

            } else if (this->data[i].val >= upperBound) {
                this->conf[i].onCt = 0;
                this->conf[i].offCt++;
                this->handleAlert(this->conf[i], this->data[i], false,
                this->conf[i].offCt); 
            }

            break;

            case ALTCOND::GTR_THAN:
            if (this->data[i].val > this->conf[i].tripVal) {
                this->conf[i].onCt++;
                this->conf[i].offCt = 0;
                this->handleAlert(this->conf[i], this->data[i], true, 
                this->conf[i].onCt);

            } else if (this->data[i].val <= lowerBound) {
                this->conf[i].onCt = 0;
                this->conf[i].offCt++;
                this->handleAlert(this->conf[i], this->data[i], false,
                this->conf[i].offCt); 
            }

            break;

            default: // Placeholder
            break;
        }
    }
}

}