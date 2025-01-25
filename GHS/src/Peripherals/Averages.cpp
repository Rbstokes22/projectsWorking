#include "Peripherals/Averages.h"
#include "cstdint"
#include "Common/Timing.hpp"
#include "Peripherals/TempHum.hpp"

namespace Peripheral {

Averages::Averages() : timeSet(TIMER_OFF), isSet(false) {}

Averages* Averages::get() {
    static Averages instance;
    return &instance;
}

const char* Averages::compileAll() { // Create JSON compilation of all periph.
    // START TO BUILD HERE.
}

// Requires the seconds past midnight to send message. If 99999 is passed,
// the service will be disabled. Only values of 0 - 86340 can be passed which
// is from midnight to 2359 the previous day. This allows appropriate padding
// to attempt if beginning attempts are unsuccessful.
void Averages::setTimer(uint32_t seconds) {
    // Filter to ensure that the passed seconds is within the constraints
    // of 0 to 86399 (seconds per day).
    if (seconds == TIMER_OFF || seconds >= 86340) {
        this->isSet = false;
        this->timeSet = TIMER_OFF;
        return;
    } 

    // If OK, sets the appropriate seconds and registers that the time
    // is set. 
    this->timeSet = seconds;
    this->isSet = true;
}

// Requires no parameters. Should be called at a 1 Hz frequency to ensure
// proper capture. Will ensure that the set time is within the perscribed range
// and that all averages are sent to the Alerts class to send to the server.
void Averages::manageTimer() { 
    uint32_t sysTime = Clock::DateTime::get()->getTime()->raw;
    uint32_t maxTime = sysTime + ATTEMPT_TIME_RANGE;
    static uint8_t attempts = 0;
    bool sent = false;
    static bool toggle = true; // Used to block code after successful attempt

    // If the system time is within the range of the time set and the max
    if ((sysTime >= this->timeSet) && (sysTime <= maxTime) && toggle) {

        // Compile all average data into JSON, and send it to the Alerts class.
        const char* data = compileAll();
        sent = Alert::get()->sendAverages(data);

        // Once sent, toggle is set to false to prevent code from re-running,
        // as well as everything is reset.
        if (sent) {attempts = 0; toggle = false;} else {attempts++;}

    } else if (sysTime > maxTime) { // Resets after alloted time expired.
        attempts = 0;
        toggle = true;
    }
}

}