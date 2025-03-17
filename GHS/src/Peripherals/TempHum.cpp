#include "Peripherals/TempHum.hpp"
#include "driver/gpio.h"
#include "Drivers/SHT_Library.hpp"
#include "Peripherals/Relay.hpp"
#include "Peripherals/Alert.hpp"
#include "Threads/Mutex.hpp"
#include "UI/MsgLogHandler.hpp"
#include "Network/NetCreds.hpp"

namespace Peripheral {

Threads::Mutex TempHum::mtx; // define static mutex instance

// Singleton class. Pass all params upon first init.
TempHum::TempHum(TempHumParams &params) : 

    data{0.0f, 0.0f, 0.0f, true}, averages{0, 0.0f, 0.0f, 0.0f, 0.0f}, 
    flags{false, false}, 
    humConf{{0, ALTCOND::NONE, ALTCOND::NONE, 0, 0, true, 0},
        {0, RECOND::NONE, RECOND::NONE, nullptr, TEMP_HUM_NO_RELAY, 0, 0, 0}},

    tempConf{{0, ALTCOND::NONE, ALTCOND::NONE, 0, 0, true, 0},
        {0, RECOND::NONE, RECOND::NONE, nullptr, TEMP_HUM_NO_RELAY, 0, 0, 0}},

    params(params) {}

// Requires relay configuration, relayOn boolean, true to turn on, false to
// turn off, and counts of exceeding boundaries. If 5 consecutive counts are 
// required, and a temp limit of 50, if the temp value exceeds 50 for 5 
// consecutive readings, the relay will either energize, or de-endergize 
// depending on settings.
void TempHum::handleRelay(relayConfigTH &conf, bool relayOn, size_t ct) {
    
    // Checks if the relay is set to be energized or de-energized. If true,
    // checks to ensure the relay is attached, there are no immediate flags
    // indicating SHT read error, the counts are above the required counts,
    // and that the condition is not set to NONE. If so, the relay will 
    // energize once params are met.
    switch (relayOn) {
        case true:
        if (conf.relay == nullptr || !this->flags.noErr ||
        ct < TEMP_HUM_CONSECUTIVE_CTS || conf.condition == RECOND::NONE) {
            return;
        }

        conf.relay->on(conf.controlID);
        break;

        // If false, the only gate is to ensure that it is not an error and
        // that the counts are above the required counts. If so, the relay
        // will signal that it is no longer being held in an energized position
        // by the SHT.
        case false:
        if (!this->flags.noErr || ct < TEMP_HUM_CONSECUTIVE_CTS) {
            return;
        }
        
        conf.relay->off(conf.controlID);
        break;
    }
}

// Requires alert configuration, alert on to be sent, and the count of 
// consecutive value trips. Works like the relay; however, the alerts will be
// reset once the values are back within range.
void TempHum::handleAlert(alertConfigTH &conf, bool alertOn, size_t ct) { 

    // Mirrors the same setup as the relay activity.
    if (!this->flags.noErr || ct < TEMP_HUM_CONSECUTIVE_CTS || 
        conf.condition == ALTCOND::NONE) { 
        return;
    }

    // Check to see if the alert is being called to send. If yes, ensures that
    // the toggle is set to true. If yes to both, it will send the message and
    // change the altToggle to false preventing it from sending another message
    // without being reset. 
    if (alertOn) {
        if (!conf.toggle) return; // avoids repeated alerts.

        NVS::SMSreq* sms = NVS::Creds::get()->getSMSReq();

        // Returned val is nullptr if API key and/or phone
        // do not meet the criteria.
        if (sms == nullptr) { 
            printf("Temp Hum Alerts: Not able to send, "
            "missing API key and/or phone\n");
            return;
        }

        char msg[TEMP_HUM_ALT_MSG_SIZE] = {0}; // message to send to server
        Alert* alt = Alert::get(); 

        // write the message to the msg array, in preparation to send to
        // the server.
        snprintf(msg, sizeof(msg), 
                "Alert: Temp at %0.2fC/%0.2fF, Humidity at %0.2f%%",
                this->data.tempC, this->data.tempF, this->data.hum
                );

        // Upon success, set to false. If no success, it will keep trying until
        // attempts are maxed.
        conf.toggle = !alt->sendAlert(sms->APIkey, sms->phone, msg, "temphum"); // When edit add tag
        
        if (!conf.toggle) {
            printf("Alert: Message Sent.\n");
            conf.attempts = 0; // Resets value
        } else {
            printf("Alert: Message Not Sent.\n");
            conf.attempts++; // Increase value to avoid oversending
        }

        // For an unsuccessful server response, after reaching max attempt 
        // value, set to false to prevent further trying. Will reset once 
        // the temp or hum is within its acceptable values.
        if (conf.attempts >= TEMP_HUM_ALT_MSG_ATT) {
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

// Requires the value and relay configuration that lists the trip value and 
// the relay condition, being less than, gtr than, or none. Once the value
// exceeds the bound, it is handled appropriately to energize or de-energize
// the attached relay. Hysteresis is applied to avoid oscillations around the
// trip value.
void TempHum::relayBounds(float value, relayConfigTH &conf, bool isTemp) {

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

    // Once the criteria has been met, the counts are incremented for each
    // consecutive count only. These counts are passed to the relay handler
    // and once conditions are met, will energize or de-energize the relays.
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

        default: // Empty but required using enum class
        break;
    }
}

// Requires the value and alert configuration that lists the trip value and 
// the alert condition, being less than, gtr than, or none. Once the value
// exceeds the bound, it is handled appropriately to send an alert to the
// server, or reset the toggle once the value is within prescribed bounds.
// Hysteresis is applied to avoid oscillations around the trip value.
void TempHum::alertBounds(float value, alertConfigTH &conf, bool isTemp) {

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

    // Once the criteria has been met, the counts are incremented for each
    // consecutive count only. These counts are passed to the alert handler
    // and once conditions are met, will send alert or reset toggle allowing
    // follow-on alerts for the same criteria being met.
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

        default: // Placeholder
        break;
    }
}

// Requires no parameters, and when called, computes the new average temp
// and humidity.
void TempHum::computeAvgs() {

    // The deltas are the change between the current and averages. Then the
    // new addition is added to the current running averages in a small amount.
    // The actual formula is:
    // NewAv = (Av * pollct + new temp)/(new poll ct)
    float deltaT = this->data.tempC - this->averages.temp;
    float deltaH = this->data.hum - this->averages.hum;
    this->averages.pollCt++;
    this->averages.temp += (deltaT / this->averages.pollCt); 
    this->averages.hum += (deltaH / this->averages.pollCt);
}

// Singleton class object, requires temphum parameters for first init. Once
// init, will return a pointer to the class instance.
TempHum* TempHum::get(TempHumParams* parameter) {

    // Single use of mutex lock which will ensure to protect any subsequent
    // calls made after requesting this instance.
    Threads::MutexLock(TempHum::mtx);

    static bool isInit{false};

    if (parameter == nullptr && !isInit) {
        return nullptr; // Blocks instance from being created.
    } else if (parameter != nullptr) {
        isInit = true; // Opens gate after proper init
    }

    static TempHum instance(*parameter);
    
    return &instance;
}

// Requires no parameters. Reads the temperature and humidity from the SHT
// driver. Upon successful reading, data is available to include temp, hum,
// and their averages. Returns true if read is successful, or false if not.
bool TempHum::read() {
    static size_t errCt{0};
    SHT_DRVR::SHT_RET read;

    // boolean return. SHT driver reads data and populates the SHT_VALS
    // struct carrier.
    read = this->params.sht.readAll(
        SHT_DRVR::START_CMD::NSTRETCH_HIGH_REP, this->data
        );

    // upon success, updates averages and changes both flags to true.
    // If unsuccessful, will change the noErr flag to false indicating an
    // immediate error, which means the data is garbage. Upon a pre-set 
    // consecutive error read, display flag is set to false allowing the 
    // clients display to show the temp/hum reading to be down.
    if (read == SHT_DRVR::SHT_RET::READ_OK) {
        this->computeAvgs();
        this->flags.noDispErr = true;
        this->flags.noErr = true;
        errCt = 0; // resets count.
    } else {
        this->flags.noErr = false; // Indicates error
        errCt++; // inc count by one.
    }

    // Sets the display to true if error ct is less than max allowed.
    this->flags.noDispErr = (errCt < TEMP_HUM_ERR_CT_MAX);

    // Returns true of data is ok.
    return this->flags.noErr;
}

// Returns humidity value float.
float TempHum::getHum() {
    return this->data.hum;
}

// Defaults to Celcius. Celcius is the standard for this device, and
// F is built in but not used, for potential future employment. Returns
// the temperature in requested value.
float TempHum::getTemp(char CorF) { // Cel or Faren
    if (CorF == 'F' || CorF == 'f') return this->data.tempF;
    return this->data.tempC;
}

// Returns the humidity configuation for modification or viewing.
TH_TRIP_CONFIG* TempHum::getHumConf() {
    return &this->humConf;
}

// Returns the temperature configuration for modification or viewing.
TH_TRIP_CONFIG* TempHum::getTempConf() {
    return &this->tempConf;
}

// Requires no paramenter. Best used if called sequentially with a successful
// read, such is if (read) checkBounds(); Checks the current values of temp
// and hum against the trip values prescribed in their configuration. Returns
// true if successful, and false if the data is marked as being corrupt by 
// the SHT driver.
bool TempHum::checkBounds() { 

    if (!this->flags.noErr) return false; // Filters bad data.

    // Checks each individual bound after confirming data is safe.
    this->relayBounds(this->data.tempC, this->tempConf.relay, true);
    this->relayBounds(this->data.hum, this->humConf.relay, false);
    this->alertBounds(this->data.tempC, this->tempConf.alt, true);
    this->alertBounds(this->data.hum, this->humConf.alt, false);

    return true;
}

// returns the current bool flags, specifically immediate and display in this.
isUpTH TempHum::getStatus() {
    return this->flags;
}

// Returns current, and previous averages structure for modification or viewing.
TH_Averages* TempHum::getAverages() { 
    return &this->averages;
}

// Clears the current data after copying it over to the previous values.
void TempHum::clearAverages() {
    this->averages.prevHum = this->averages.hum;
    this->averages.prevTemp = this->averages.temp;
    this->averages.hum = 0.0f;
    this->averages.temp = 0.0f;
    this->averages.pollCt = 0; 
}

// void TempHum::test(bool isTemp, float val) { // !!!COMMENT OUT WHEN NOT TEST
//     if (isTemp) {
//         this->data.tempC = val;
//     } else {
//         this->data.hum = val;
//     }
//     this->flags.noErr = true;
//     this->checkBounds(); 
// }

}