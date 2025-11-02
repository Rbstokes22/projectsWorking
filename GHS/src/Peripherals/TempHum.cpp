#include "Peripherals/TempHum.hpp"
#include "driver/gpio.h"
#include "Drivers/SHT_Library.hpp"
#include "Peripherals/Relay.hpp"
#include "Peripherals/Alert.hpp"
#include "Threads/Mutex.hpp"
#include "UI/MsgLogHandler.hpp"
#include "Network/NetCreds.hpp"
#include "Common/FlagReg.hpp"
#include "string.h"
#include "Common/Timing.hpp"
#include "Config/config.hpp"

namespace Peripheral {

const char* TempHum::tag(TEMP_HUM_TAG);
char TempHum::log[LOG_MAX_ENTRY]{0};
Threads::Mutex TempHum::mtx(TEMP_HUM_TAG); // define static mutex instance

// Requires tag of sensor name. Ensure tag is less than 31 chars.
SensDownPkg::SensDownPkg(const char* tag) : status(false), prevStatus(false),
    counts(0), lastAlt(LAST_SENT::UP) {

    snprintf(this->sensor, sizeof(this->sensor), "%s", tag);
}

// Singleton class. Pass all params upon first init.
TempHum::TempHum(TempHumParams &params) : 

    data{0.0f, 0.0f, 0.0f, true}, sensHealth(0.0f), readErr(false), 
    humConf{{0, ALTCOND::NONE, ALTCOND::NONE, 0, 0, true, 0},
        {0, RECOND::NONE, RECOND::NONE, nullptr, TEMP_HUM_NO_RELAY, 0, 0, 0}},

    tempConf{{0, ALTCOND::NONE, ALTCOND::NONE, 0, 0, true, 0},
        {0, RECOND::NONE, RECOND::NONE, nullptr, TEMP_HUM_NO_RELAY, 0, 0, 0}},

    params(params) {

        memset(&this->averages, 0, sizeof(this->averages));
        memset(&this->trends, 0.0f, sizeof(this->trends));

        snprintf(TempHum::log, sizeof(TempHum::log), "%s Ob Created", 
            TempHum::tag);

        TempHum::sendErr(TempHum::log, Messaging::Levels::INFO);
    }

// Requires relay configuration, relayOn boolean, true to turn on, false to
// turn off, and counts of exceeding boundaries. If 5 consecutive counts are 
// required, and a temp limit of 50, if the temp value exceeds 50 for 5 
// consecutive readings, the relay will either energize, or de-endergize 
// depending on settings.
void TempHum::handleRelay(relayConfigTH &conf, bool relayOn, size_t ct) {

    // Primary checks to ensure usability for relay. Special conditions are
    // with the appropriate case, if they exist.
    if (conf.relay == nullptr || this->readErr ||
        ct < TEMP_HUM_CONSECUTIVE_CTS) return; // Block 
    
    // Checks if the relay is set to be energized or de-energized. 
    switch (relayOn) {
        case true:
        if (conf.condition == RECOND::NONE) return; // Block
        conf.relay->on(conf.controlID);
        break;

        // If false, the only gate is to ensure that it is not an error and
        // that the counts are above the required counts. If so, the relay
        // will signal that it is no longer being held in an energized position
        // by the SHT.
        case false:
        conf.relay->off(conf.controlID);
        break;
    }
}

// Requires alert configuration, alert on or (to send), and the count of 
// consecutive value trips. Works like the relay; however, the alerts will be
// reset once the values are back within range.
void TempHum::handleAlert(alertConfigTH &conf, bool alertOn, size_t ct) { 

    // Mirrors the same setup as the relay activity.
    if (this->readErr || ct < TEMP_HUM_CONSECUTIVE_CTS || 
        conf.condition == ALTCOND::NONE) { 

        return; // Block.
    }

    // Check to see if the alert is being called to send. If yes, ensures that
    // the toggle is set to true. If yes to both, it will send the message and
    // change the altToggle to false preventing it from sending another message
    // without being reset. 
    if (alertOn) {
        if (!conf.toggle) return; // avoids repeated alerts.

        char msg[TEMP_HUM_ALT_MSG_SIZE] = {0}; // message to send to server
        Alert* alt = Alert::get(); 

        if (alt == nullptr) return; // Not required, will never return nullptr

        // write the message to the msg array, in preparation to send to
        // the server.
        snprintf(msg, sizeof(msg), 
            "Alert: Temp at %0.2fC/%0.2fF, Humidity at %0.2f%%",
            this->data.tempC, this->data.tempF, this->data.hum);

        // Upon success, set to false. If no success, it will keep trying until
        // attempts are maxed.
        conf.toggle = !alt->sendAlert(msg, TempHum::tag); 

        // Increment or clear, depending on success.
        conf.attempts = conf.toggle ? conf.attempts + 1 : 0;
        
        if (conf.attempts >= TEMP_HUM_ALT_MSG_ATT) {
            snprintf(TempHum::log, sizeof(TempHum::log), 
                "%s max send attempts @ %u", TempHum::tag, conf.attempts);

            TempHum::sendErr(TempHum::log);
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

// Requires the value, relay config that lists the trip value and relay 
// condition being LT, GT, or NONE, and if it is a temp. If it is a temp, this
// indicates a float that is multiplied by 100 in int form (10.2 = 1020). 
// This allows the client to have precision over the precise temperature rather
// than steps of celcius degrees. Once the value exceeds the bound, it is
// handled appropriately to energize or de-energize the attached relay. 
// Hysteresis is applied to dampen the osciallations around the trip val.
void TempHum::relayBounds(float value, relayConfigTH &conf, bool isTemp) {

    float tripVal = static_cast<float>(conf.tripVal);

    if (isTemp) tripVal /= 100.0f; // div by 100 if a temp.

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

// Requires the value, alert config that lists the trip value and alert 
// condition being LT, GT, or NONE, and if it is a temp. If it is a temp, this
// indicates a float that is multiplied by 100 in int form (10.2 = 1020). 
// This allows the client to have precision over the precise temperature rather
// than steps of celcius degrees. Once the value exceeds the bound, it is
// handled appropriately to energize or de-energize the attached relay. 
// Hysteresis is applied to dampen the osciallations around the trip val.
void TempHum::alertBounds(float value, alertConfigTH &conf, bool isTemp) {

    float tripVal = static_cast<float>(conf.tripVal);

    if (isTemp) tripVal /= 100.0f; // div by 100 if a temp.

    float lowerBound = conf.tripVal - TEMP_HUM_HYSTERESIS;
    float upperBound = conf.tripVal + TEMP_HUM_HYSTERESIS;

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

// Requires no params. Gets the current hour. When the hour switches, hours
// 0 to n - 1 will be captured and moved to position 1 to n, position 0 will
// have the new value written into it after the move. 
void TempHum::computeTrends() {

    // Get current hour. Set static last hour equal to. This should init at
    // 0, 0.
    uint8_t hour = Clock::DateTime::get()->getTime()->hour;
    static uint8_t lastHour = hour; 

    if (hour != lastHour) { // Change detected.

        // Move (idx 0 to n-1) to (idx 1 to n). Opens idx 0 for new val.
        memmove(&this->trends.temp[1], this->trends.temp, 
            sizeof(this->trends.temp) - sizeof(this->trends.temp[0]));

        this->trends.temp[0] = this->data.tempC; // Insert current reading.

        // Same as above.
        memmove(&this->trends.hum[1], this->trends.hum, 
            sizeof(this->trends.hum) - sizeof(this->trends.hum[0]));

        this->trends.hum[0] = this->data.hum; // same.

        lastHour = hour; // Update time for next change.
    }
}


// Requires messand and messaging level. Level default to ERROR.
void TempHum::sendErr(const char* msg, Messaging::Levels lvl) {
    Messaging::MsgLogHandler::get()->handle(lvl, msg, TEMP_HUM_LOG_METHOD);
}

// Singleton class object, requires temphum parameters for first init. Once
// init, will return a pointer to the class instance.
TempHum* TempHum::get(TempHumParams* parameter) {

    // Single use of mutex lock which will ensure to protect any subsequent
    // calls made after requesting this instance.
    Threads::MutexLock(TempHum::mtx);

    static bool isInit{false};

    if (parameter == nullptr && !isInit) {
        snprintf(TempHum::log, sizeof(TempHum::log), 
            "%s using uninit instance, ret nullptr", TempHum::tag);

        TempHum::sendErr(TempHum::log, Messaging::Levels::CRITICAL);
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

    SHT_DRVR::SHT_RET read;
    static bool logOnce = true; // Used to log errors once, and log fixed once.

    // Used to handle alerts for the sensor being down/impacted long term.
    static SensDownPkg pkg(this->tag);

    Alert* alt = Alert::get();

    SHT_DRVR::SHT_VALS tempVal;

    // boolean return. SHT driver reads data and populates the SHT_VALS
    // struct carrier.
    read = this->params.sht.readAll(SHT_DRVR::START_CMD::NSTRETCH_HIGH_REP, 
        tempVal);

    // upon success, updates averages/trends and changes both flags to true.
    // If unsuccessful, will change the noErr flag to false indicating an
    // immediate error, which means the data is garbage. Upon a pre-set 
    // consecutive error read, display flag is set to false allowing the 
    // clients display to show the temp/hum reading to be down.
    if (read == SHT_DRVR::SHT_RET::READ_OK) {
        this->data = tempVal; // Set actual value to temp val if success.
        this->readErr = false;
        this->computeAvgs();
        this->computeTrends();
        this->sensHealth *= HEALTH_EXP_DECAY; // Decay unit per good read.

        if (!logOnce) {
            snprintf(TempHum::log, sizeof(TempHum::log), "%s err fixed",
                TempHum::tag);

            TempHum::sendErr(TempHum::log, Messaging::Levels::INFO);
            logOnce = true; // Prevent re-log, allow err logging.
        }

    } else { // If error, set flag to false. No err handling necessary here.
        this->sensHealth += HEALTH_ERR_UNIT; // Adds one unit perbad read.
        if (this->sensHealth > HEALTH_ERR_MAX) { // Sets max value.
            this->sensHealth = HEALTH_ERR_MAX;
        }

        this->readErr = true;
    }

    // Handle sensor checks and alerts if sensor is broken or fixed.
    alt->monitorSens(pkg, this->sensHealth); 

    // Logs if sensor becomes unresponsive.
    if (this->sensHealth > HEALTH_ERR_BAD) {

        if (logOnce) {
            snprintf(TempHum::log, sizeof(TempHum::log), "%s read err",
                TempHum::tag);

            TempHum::sendErr(TempHum::log);
            logOnce = false; // Prevents re-log, allows fixed error log.
        }
    } 
    // Returns true of data is ok.
    return !this->readErr;
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

    if (this->readErr) return false; // Prevent bad data from being used.

    // Checks each individual bound after confirming data is safe.
    this->relayBounds(this->data.tempC, this->tempConf.relay, true);
    this->relayBounds(this->data.hum, this->humConf.relay, false);
    this->alertBounds(this->data.tempC, this->tempConf.alt, true);
    this->alertBounds(this->data.hum, this->humConf.alt, false);

    return true;
}

// Require no params. Returns current sensor health.
float TempHum::getHealth() {return this->sensHealth;}

// Returns current, and previous averages structure for modification or viewing.
TH_Averages* TempHum::getAverages() { 
    return &this->averages;
}

// Requires no params. Returns the current temp/hum trend struct.
TH_Trends* TempHum::getTrends() {
    return &this->trends;
}

// Clears the current data after copying it over to the previous values.
void TempHum::clearAverages() {

    // Write averages into log. Prep log, do not send until complete. First
    // in order to capture current averages before resetting.
    snprintf(TempHum::log, sizeof(TempHum::log),
        "%s Averages Cleared. TempC: %.1f, TempF: %.1f,"
        " Hum: %.1f. Count %zu", 
        TempHum::tag,
        this->averages.temp, (this->averages.temp * 1.8 + 32),
        this->averages.hum, this->averages.pollCt
    );

    // Copy averages over
    this->averages.prevHum = this->averages.hum;
    this->averages.prevTemp = this->averages.temp;

    // Reset current values.
    this->averages.hum = 0.0f;
    this->averages.temp = 0.0f;
    this->averages.pollCt = 0; 

    // Send log
    Messaging::MsgLogHandler::get()->handle(Messaging::Levels::INFO,
        TempHum::log, TEMP_HUM_LOG_METHOD, true, false);
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