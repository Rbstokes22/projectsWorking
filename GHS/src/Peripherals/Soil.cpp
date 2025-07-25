#include "Peripherals/Soil.hpp"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_continuous.h"
#include "Peripherals/Alert.hpp"
#include "Threads/Mutex.hpp"
#include "UI/MsgLogHandler.hpp"
#include "string.h"
#include "Network/NetCreds.hpp"

namespace Peripheral {

const char* Soil::tag(SOIL_TAG);
char Soil::log[LOG_MAX_ENTRY]{0};
Threads::Mutex Soil::mtx(SOIL_TAG); // define instance of mtx

Soil::Soil(SoilParams &params) : 

    data{
        {0, false, false, 0}, {0, false, false, 0}, {0, false, false, 0}, 
        {0, false, false, 0}
    },
    conf{
        {0, ALTCOND::NONE, ALTCOND::NONE, 0, 0, true, 1, 0}, 
        {0, ALTCOND::NONE, ALTCOND::NONE, 0, 0, true, 2, 0}, 
        {0, ALTCOND::NONE, ALTCOND::NONE, 0, 0, true, 3, 0},
        {0, ALTCOND::NONE, ALTCOND::NONE, 0, 0, true, 4, 0}
    }, params(params) {

        snprintf(Soil::log, sizeof(Soil::log), "%s Ob created", Soil::tag);
        Soil::sendErr(Soil::log, Messaging::Levels::INFO);
    }

// Requires SOIL_TRIP_CONFIG data, readings data, whether to sound alert or 
// reset alert, and the count number of consecutive trips. 
void Soil::handleAlert(AlertConfigSo &conf, SoilReadings &data, 
    bool alertOn, size_t ct) {
    
    // Acts as a gate to ensure that all of this criteria is met before
    // altering alerts.
    if (!data.noErr || ct < SOIL_CONSECUTIVE_CTS || 
        conf.condition == ALTCOND::NONE) return;

    // Check to see if the alert is being called to send. If yes, ensures that
    // the toggle is set to true. If yes to both, it will send the message and
    // change the altToggle to false preventing it from sending another message
    // without being reset.
    if (alertOn) {
        if (!conf.toggle) return; // avoids repeated alerts.

        NVS::SMSreq* sms = NVS::Creds::get()->getSMSReq(); 

        // Returned val is nullptr if API key and/or phone do not meet the
        // criteria.
        if (sms == nullptr) {
            snprintf(Soil::log, sizeof(Soil::log), 
                "%s sms unable to send, missing API key and/or phone", 
                Soil::tag);

            Soil::sendErr(Soil::log);
            return;
        }

        // sms is valid.
        char msg[SOIL_ALT_MSG_SIZE] = {0}; // message to send to server
        Alert* alt = Alert::get();

        // write the message to the msg array, in preparation to send to
        // the server.
        snprintf(msg, sizeof(msg), "Alert: Soil Sensor %u at value %d",
            conf.ID, data.val);

        // Upon success, set to false. If no success, it will keep trying until
        // attempts are maxed. Sends tag because the alert takes care of 
        // logging. No logging required.
        conf.toggle = !alt->sendAlert(msg, Soil::tag);

        // Increment or clear depending on success. 
        conf.attempts = conf.toggle ? conf.attempts + 1 : 0;

        // For an unsuccessful server response, after reaching max attempt 
        // value, set to false to prevent further trying. Will reset once 
        // the soil is within its acceptable values.
        if (conf.attempts >= SOIL_ALT_MSG_ATT) {
            snprintf(Soil::log, sizeof(Soil::log), "%s max send attempts @ %u",
                Soil::tag, conf.attempts);

            Soil::sendErr(Soil::log);
            conf.toggle = false; // Indicates successful send, TRICK.
        }

    } else {
        // Resets the toggle when the alert is set to off meaning that
        // satisfactory conditions prevail. This will allow a message
        // to be sent if it exceeds the limitations.
        conf.toggle = true;
        conf.attempts = 0;
    }
}

// Requires message and messaging level. Default level set to ERROR.
void Soil::sendErr(const char* msg, Messaging::Levels lvl) {
    Messaging::MsgLogHandler::get()->handle(lvl, msg, SOIL_LOG_METHOD);
}

// Requires SoilParms* parameter.
// Default setting = nullptr. Must be init with a non nullptr to create the 
// instance, and will return a pointer to the instance upon proper completion.
Soil* Soil::get(SoilParams* parameter) {

    // Single use of mutex lock which will ensure to protect any subsequent
    // calls made after requesting this instance.
    Threads::MutexLock(Soil::mtx);

    static bool isInit{false};
    
    if (parameter == nullptr && !isInit) {
        snprintf(Soil::log, sizeof(Soil::log), 
            "%s using uninit instance, ret nullptr", Soil::tag);

        Soil::sendErr(Soil::log, Messaging::Levels::CRITICAL);
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
AlertConfigSo* Soil::getConfig(uint8_t indexNum) {
    if (indexNum >= SOIL_SENSORS) return nullptr;
    return &conf[indexNum];
}

// Reads all soil sensors and stores the data in the data variable.
void Soil::readAll() {
    esp_err_t err;
    static bool logOnce[SOIL_SENSORS] = {true, true, true, true};
    
    for (int i = 0; i < SOIL_SENSORS; i++) {
        err = adc_oneshot_read(
            this->params.handle, this->params.channels[i], &this->data[i].val
            );

        // Essentially math.floor to represents all values within the range of
        // the analog noise. If NOISE is set to 20, this means that all values 
        // from 1500 to 1519 are represented.
        this->data[i].val -= (this->data[i].val % SOIL_NOISE);

        // Sets the proper values based on read success.
        if (err == ESP_OK) {
            this->data[i].noDispErr = true;
            this->data[i].noErr = true;
            this->data[i].errCt = 0;

            if (!logOnce[i]) { // Can only be set by err below.
                snprintf(Soil::log, sizeof(Soil::log), "%s snsr %d err fixed",
                    Soil::tag, i);

                Soil::sendErr(Soil::log, Messaging::Levels::INFO);
                logOnce[i] = true; // Preven re-log, allow err logging.
            }

        } else { // Error

            this->data[i].noErr = false;
            this->data[i].errCt++;
        }

        // Sets the display flag to true if error count is below max.
        this->data[i].noDispErr = (this->data[i].errCt < SOIL_ERR_MAX);

        // If error, log once until reset by no error. Must preceed the fixed
        // err msg.
        if (!this->data[i].noDispErr && logOnce[i]) {
            snprintf(Soil::log, sizeof(Soil::log), "%s snsr %d read err",
                Soil::tag, i);

            Soil::sendErr(Soil::log);
            logOnce[i] = false; // Prevents re-log, allows fixed error log.
        }
    }
}   

// Requires the sensor number you are trying to read. Returns pointer to the
// soil reading data if correct sensor index value is used, and nullptr if not.
SoilReadings* Soil::getReadings(uint8_t indexNum) {
    if (indexNum >= SOIL_SENSORS) {
        snprintf(Soil::log, sizeof(Soil::log), 
            "%s index num %u exceeds %u ret nullptr", Soil::tag, indexNum, 
            SOIL_SENSORS);

        Soil::sendErr(Soil::log, Messaging::Levels::CRITICAL);
        return nullptr;
    }

    // IDx num is good, return pointer to data.
    return &this->data[indexNum];
}

// Requires no parameters. Checks the configuration setting for each sensor
// and compares it against the actual value to trip the alert if configured
// to do so.
void Soil::checkBounds() {
    
    // Iterates through each of the soil sensors to check its current value
    // against the value set to trip the alarm.
    for (int i = 0; i < SOIL_SENSORS; i++) {
        
        // Skips checks if the data is bad. This is done in iteration, unlike
        // temphum.cpp. This is due to having several sensors data in this 
        // singleton class, which is why this is not a bool function either.
        if (!this->data[i].noErr) continue; 
        
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

// Used for testing. Comment out when done.
// void Soil::test(int val, int sensorIdx) {
//     this->data[sensorIdx].val = val;
//     this->data[sensorIdx].noErr = true;
//     this->checkBounds();
// }

}