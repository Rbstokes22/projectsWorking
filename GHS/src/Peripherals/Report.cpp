#include "Peripherals/Report.hpp"
#include "cstdint"
#include "stddef.h"
#include "Common/Timing.hpp"
#include "Peripherals/TempHum.hpp"
#include "Peripherals/Soil.hpp"

// No mutex required, Accessed from a single thread

namespace Peripheral {

Report::Report() : timer{TIMER_OFF, false} {}

Report* Report::get() {
    static Report instance;
    return &instance; 
}

// Requires the json char array, and its size. Populates a report of 
// the average temp and humidity, the proportionality of the visible
// and NIR light, the average of the clear light channel, and the 
// current soil readings. Returns true if populated, and false if not.
bool Report::compileAll(char* jsonRep, size_t bytes) { 
    
    uint16_t PH = 55; // !!! Placeholder delete after remainder is built.

    // For colors, all visible and NIR are going to be proportionality, so
    // its percentage of the total. Clear is going to be average intensity
    // in counts with 65535 being the most intense. Temp and hum will be
    // the actual averages. Soil will be current values since not alot 
    // of fluctuation is expected.
    int written = snprintf(jsonRep, bytes, 
    "{\"temp\":%.2f,\"hum\":%.2f,"
    "\"violet\":%u,\"indigo\":%u,\"blue\":%u,\"cyan\":%u,"
    "\"green\":%u,\"yellow\":%u,\"orange\":%u,\"red\":%u,"
    "\"nir\":%u,\"clear\":%u,"
    "\"soil1\":%d,\"soil2\":%d,\"soil3\":%d,\"soil4\":%d}",
    TempHum::get()->getAverages()->temp,
    TempHum::get()->getAverages()->hum,
    PH, PH, PH, PH, PH, PH, PH, PH, PH, PH,
    Soil::get()->getReadings(0)->val,
    Soil::get()->getReadings(1)->val,
    Soil::get()->getReadings(2)->val,
    Soil::get()->getReadings(3)->val
    );

    return (written >= 0 && written <= bytes); // false if bad write
}

// Clears all averages. Called after sending the data to the server or 
// once per day if timer is unset.
void Report::clearAll() {
    TempHum::get()->clearAverages();
    // ADD OTHERS HERE!!!
}

// Requires the seconds past midnight to send message. If 99999 is passed,
// the service will be disabled. Only values of 0 - 86340 can be passed which
// is from midnight to 2359 the previous day. This allows appropriate padding
// to attempt if beginning attempts are unsuccessful.
void Report::setTimer(uint32_t seconds) {
    // Filter to ensure that the passed seconds is within the constraints
    // of 0 to 86399 (seconds per day).
    if (seconds == TIMER_OFF || seconds >= MAX_SET_TIME) {
        this->timer.isSet = false;
        this->timer.timeSet = TIMER_OFF;
        return;
    } 

    // If OK, sets the appropriate seconds and registers that the time
    // is set. 
    this->timer.timeSet = seconds;
    this->timer.isSet = true;
}

// Requires no parameters. Should be called at a 1 Hz frequency to ensure
// proper capture. Will ensure that the set time is within the perscribed range
// and that all averages are sent to the Alerts class to send to the server.
void Report::manageTimer() { 
    
    uint32_t sysTime = Clock::DateTime::get()->getTime()->raw;
    uint32_t maxTime = this->timer.timeSet + ATTEMPT_TIME_RANGE;
    static uint8_t attempts = 0;
    bool sent = false;
    static bool toggle = true; // Used to block code after successful attempt

    // If the range exceeds the total seconds per day - 10, ot 86390, will 
    // default to 86390 to ensure success.
    if (maxTime >= MAX_TOTAL_SET_TIME) maxTime = MAX_TOTAL_SET_TIME;

    // MANIPULATOR. Will manipulate time setting to ensure that all averages
    // are cleared at 23:59 each day if not specified.
    if (!this->timer.isSet) {
        this->timer.timeSet = MAX_SET_TIME;
    }

    // If the system time is within the range of the time set and the max
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

        char JSONrep[REP_JSON_DATA_SIZE] = {0}; // init JSON report

        // Compile all average data into JSON, and send it to the Alerts class
        // to be sent to the server for SMS processing.
        compileAll(JSONrep, sizeof(JSONrep));
        sent = Alert::get()->sendReport(JSONrep);

        // Once sent, toggle is set to false to prevent code from re-running,
        // as well as everything is reset.
        if (sent) {
            attempts = 0; 
            toggle = false;
            this->clearAll();
            } else {attempts++;}

        // Block another attempt if maxed out.
        if (attempts >= SEND_ATTEMPTS) toggle = false;

    } else if (sysTime > maxTime) { // Resets after alloted time expired.
        attempts = 0;
        toggle = true;
        this->clearAll(); // resets averages.
    }
}

// Gets the time data to send back to the client to show status.
TimerData* Report::getTimeData() {
    return &this->timer;
}

}