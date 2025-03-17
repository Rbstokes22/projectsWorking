#include "Peripherals/Report.hpp"
#include "cstdint"
#include "stddef.h"
#include "Common/Timing.hpp"
#include "Peripherals/TempHum.hpp"
#include "Peripherals/Soil.hpp"
#include "Peripherals/Light.hpp"
#include "UI/MsgLogHandler.hpp"

// No mutex required, Accessed from a single thread

namespace Peripheral {

Report::Report() : tag("(REPORT)") ,timer{TIMER_OFF, false} {

    snprintf(this->log, sizeof(this->log), "%s Ob created", this->tag);
    this->sendErr(this->log, Messaging::Levels::INFO);
}

// Returns pointer to that static instance.
Report* Report::get() {
    static Report instance;
    return &instance; 
}

// Requires pointer to json report, and its size in bytes. Populates the
// report of the average temp/hum and spectral/photo readings over duration, 
// the duration of being light, and current soil readings. Returns true if 
// populated, and false if not.
bool Report::compileAll(char* jsonRep, size_t bytes) { 
    
    TH_Averages* th = TempHum::get()->getAverages();
    Soil* soil = Soil::get();
    Light_Averages* light = Light::get()->getAverages();
    int written = -1;
    bool isErr = false; // Used to log if error occurs.

    // All colors will be the average accumulation over the previous capture
    // time. The counts should always be a float between 0 and 65535 or 
    // 16-bit max. Soil values are smooth, so averages will not be computed,
    // and current values will be populated.
    written = snprintf(jsonRep, bytes, 
    "{\"temp\":%0.2f,\"hum\":%0.2f,"
    "\"violet\":%0.2f,\"indigo\":%0.2f,\"blue\":%0.2f,\"cyan\":%0.2f,"
    "\"green\":%0.2f,\"yellow\":%0.2f,\"orange\":%0.2f,\"red\":%0.2f,"
    "\"nir\":%0.2f,\"clear\":%0.2f,\"photo\":%0.2f,\"lightDur\":%lu,"
    "\"soil1\":%d,\"soil2\":%d,\"soil3\":%d,\"soil4\":%d}",
    th->temp, th->hum,
    light->color.violet, light->color.indigo,
    light->color.blue, light->color.cyan,
    light->color.green, light->color.yellow,
    light->color.orange, light->color.red,
    light->color.nir, light->color.clear,
    light->photoResistor, Light::get()->getDuration(),
    soil->getReadings(0)->val, soil->getReadings(1)->val,
    soil->getReadings(2)->val, soil->getReadings(3)->val
    );

    if (written == -1) { // Not written
        snprintf(this->log, sizeof(this->log), "%s write err", this->tag);
        isErr = true;

    } else if (written == 0) { // Nothing written
        snprintf(this->log, sizeof(this->log), "%s zero write", this->tag);
        isErr = true;

    } else if (written > bytes) { // Too much data.
        snprintf(this->log, sizeof(this->log), "%s write exceeds length @ %d", 
            this->tag, written);
        isErr = true;
    }

    if (isErr) this->sendErr(this->log);

    return (written > 0 && written <= bytes); // false if bad write
}

// Clears all averages. Called after sending the data to the server or 
// once per day if timer is unset.
void Report::clearAll() {
    TempHum::get()->clearAverages();
    Light::get()->clearAverages(); 
}

// Requires message and messaging level. Level set default to ERROR.
void Report::sendErr(const char* msg, Messaging::Levels lvl) {
    Messaging::MsgLogHandler::get()->handle(lvl, msg,
        Messaging::Method::SRL_LOG);
}

// Requires the seconds past midnight to send message. If 99999 is passed,
// the service will be disabled. Only values of 0 - 86340 can be passed which
// is from midnight to 23:59:00. This allows appropriate padding to attempt 
// if beginning attempts are unsuccessful.
void Report::setTimer(uint32_t seconds) {

    // Filter to ensure that the passed seconds is within the constraints
    // of 0 to 86340 (seconds per day - 60).
    if (seconds == TIMER_OFF || seconds >= MAX_SET_TIME) {
        this->timer.isSet = false; // Nulls out settings
        this->timer.timeSet = TIMER_OFF;

        snprintf(this->log, sizeof(this->log), "%s Timer off", this->tag);
        this->sendErr(this->log, Messaging::Levels::INFO);
        return; // Block
    } 

    // If OK, sets the appropriate seconds and registers that the time
    // is set. 
    this->timer.timeSet = seconds;
    this->timer.isSet = true;

    snprintf(this->log, sizeof(this->log), "%s timer set to %lu seconds",
        this->tag, seconds);

    this->sendErr(this->log, Messaging::Levels::INFO);
}

// Requires no params. Checks that the system time is within the time range
// to send the report. WARNING. Timing is critical, and this should be called
// at a 1 Hz frequency in order to capture the important time raises. If a 
// frequency of > 10 Hz is called, will potentially miss the toggle handling
// which will cause further issues. This sends all averages to the Alerts 
// class that will be sent to the client.
void Report::manageTimer() { 
    
    // System time, raw format gives you seconds past midnight. Max Time is
    // the top of the range from the start of the time set. If the time is set
    // at 22:30:00, the maxTime would be 22:30:20 if the ATTEMPT_TIME_RANGE
    // is set to 20.
    uint32_t sysTime = Clock::DateTime::get()->getTime()->raw;
    uint32_t maxTime = this->timer.timeSet + ATTEMPT_TIME_RANGE;

    static uint8_t attempts = 0; // Sending attempts
    bool sent = false; // Is message sent
    static bool toggle = true; // Used to block code after successful attempt

    // If the range exceeds the total seconds per day - 10, ot 86390, will 
    // default to 86390 to ensure success.
    if (maxTime >= MAX_TOTAL_SET_TIME) maxTime = MAX_TOTAL_SET_TIME;

    // MANIPULATOR. Will manipulate time setting to ensure that all averages
    // are cleared at 23:59 each day if not specified. A report will not be
    // sent, but typically clearing is when the report is complete.
    if (!this->timer.isSet) {
        this->timer.timeSet = MAX_SET_TIME;
    }

    // If the system time is within the range of the time set and the max,
    // Attempt to send.
    if ((sysTime >= this->timer.timeSet) && (sysTime <= maxTime) && toggle) {

        // If no time is set for averages, will clear at least once per day
        // at 23:59PM. This will only run this specific part and not attempt
        // to send to server.
        if (!this->timer.isSet) {
            this->clearAll();
            attempts = 0;
            toggle = false;
            this->timer.timeSet = TIMER_OFF;
            return; // block the rest of the code from running.
        }

        char JSONrep[REP_JSON_DATA_SIZE] = {0}; // init JSON report for SMS

        // Compile all average data into JSON, and send it to the Alerts class
        // to be sent to the server for SMS processing. Block if error.
        if (!compileAll(JSONrep, sizeof(JSONrep))) return; // Block

        sent = Alert::get()->sendReport(JSONrep);

        // Once sent, toggle is set to false to prevent code from re-running,
        // as well as everything is reset.
        if (sent) {
            attempts = 0; 
            toggle = false;
            this->clearAll();
            } else {attempts++;} // Incremenet attempt if not sent.

        // Block another attempt if maxed out by setting toggle to false. This
        // will be reset when the system time exceed the max time. That is
        // the reason that the report time is capped at 23:59:00, to allow for
        // this resetting.
        if (attempts >= SEND_ATTEMPTS) toggle = false;

    // Resets after alloted time expired. 10 Second working window in worst
    // case scenario. Timing is crucial to capture since the sysTime will 
    // reset to 0 at midnigh, which will not clear until a 24 hour period.
    } else if (sysTime > maxTime) { 
        attempts = 0;
        toggle = true;
    }
}

// Gets the time data to send back to the client to show status.
TimerData* Report::getTimeData() {
    return &this->timer;
}

}