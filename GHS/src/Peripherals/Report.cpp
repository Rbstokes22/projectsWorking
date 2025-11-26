#include "Peripherals/Report.hpp"
#include "cstdint"
#include "stddef.h"
#include "Common/Timing.hpp"
#include "Peripherals/TempHum.hpp"
#include "Peripherals/Soil.hpp"
#include "Peripherals/Light.hpp"
#include "UI/MsgLogHandler.hpp"
#include "Network/Handlers/socketHandler.hpp"
#include "Threads/Mutex.hpp"
#include "xtensa/hal.h"

// No mutex required, Accessed from a single thread

namespace Peripheral {

Threads::Mutex Report::mtx(REPORT_TAG);

Report::Report() : tag(REPORT_TAG), clrTimeSet(MAX_SET_TIME) {

    snprintf(this->log, sizeof(this->log), "%s Ob created", this->tag);
    this->sendErr(this->log, Messaging::Levels::INFO);
}

// Clears all averages. Called after sending the data to the server or 
// once per day if timer is unset.
void Report::clearAll() { // Mtx is suspended before this call.
    TempHum::get()->clearAverages();
    Light::get()->clearAverages(); 
}

// Requires message and messaging level. Level set default to ERROR.
void Report::sendErr(const char* msg, Messaging::Levels lvl) {
    Messaging::MsgLogHandler::get()->handle(lvl, msg, REPORT_LOG_METHOD);
}

// Returns pointer to that static instance.
Report* Report::get() {

    static Report instance;
    return &instance; 
}

// Requires the seconds past midnight to clear the averages. If the value 
// exceeds 86340, or 23:59:00, it will be set to that default time. 
void Report::setTimer(uint32_t seconds) {

    Threads::MutexLock guard(Report::mtx);
    if (!guard.LOCK()) {
        return; // Block if locked.
    }

    // Filter to ensure that MAX set time is not exceeded.
    this->clrTimeSet = (seconds >= MAX_SET_TIME) ? MAX_SET_TIME : seconds;

    snprintf(this->log, sizeof(this->log), 
        "%s Average clear timer set to %lu seconds", this->tag, 
        this->clrTimeSet);
    
    this->sendErr(this->log, Messaging::Levels::INFO);
}

// Requires no params. Checks that the system time is within the time range
// to send the report. WARNING. Timing is critical, and this should be called
// at a 1 Hz frequency in order to capture the important time raises. If a 
// frequency of > 10 Hz is called, will potentially miss the toggle handling
// which will cause further issues. This sends all averages to the Alerts 
// class that will be sent to the client. Also handles secondary control of
// logging a new day in the 10 secs before midnight.
void Report::manageTimer() { 

    Clock::TIME dtg;
    Clock::DateTime::get()->getTime(&dtg);

    Threads::MutexLock guard(Report::mtx);
    if (!guard.LOCK()) {
        return; // Block if locked.
    }

    uint8_t hour = dtg.hour;
    static uint8_t lastHour = hour; // Set static upon init.
    static uint8_t attempts = 0; // Sending attempts

    // Raw systime gives the seconds past midnight from 0 - 86399.
    uint32_t sysTime = dtg.raw;

    // clear toggle, day toggle.
    static bool clrT = true, dayT = true;

    bool inRange = (sysTime >= this->clrTimeSet) && 
        (sysTime <= (this->clrTimeSet + REPORT_TIME_PADDING));

    // Clear all timer
    if (clrT && inRange) { // Not cleared, systime is in range.
        clrT = false;

        if (!guard.UNLOCK()) return;
        this->clearAll(); // Clears external class variables.
        if (!guard.LOCK()) return;

    } else if (!clrT && !inRange) { // Has cleared and systime is out of range.
        clrT = true; // Reset
    }

    // daily log timer. Logs between midnight and 00:00::50
    inRange = (sysTime <= REPORT_TIME_PADDING);

    if (dayT && inRange) { // Not logged yet and in range.
        dayT = false;
        snprintf(this->log, sizeof(this->log), "%s NEW DAY", this->tag);
        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::INFO,
            this->log, Messaging::Method::SRL_OLED_LOG);

    } else if (!dayT && !inRange) { // logged, and no longer in range.
        dayT = true; // Reset.
    }

    // Send report at hour change, will also trigger at first calibration 
    // assuming it is not between midnight and 0100.
    if (hour != lastHour) {

        // Attempt to send report via alert class. Potential big call.
        if (!guard.UNLOCK()) return;
        bool sent = Alert::get()->sendReport(Comms::SOCKHAND::getReportBuf());
        if (!guard.LOCK()) return; 

        // Set the attempts to 0 if successful, or increment if failed.
        attempts = (sent) ? 0 : (attempts + 1);

        // Set the last hour to the current hour if successful.
        lastHour = (sent) ? hour : lastHour;

        // Blocks another attempt if maxed out for the remaining hour by 
        // pretending it was successful.
        if (attempts >= SEND_ATTEMPTS) {
            lastHour = hour; 
            attempts = 0;
        }
    }
}

// Param data ptr def to nullptr. If mtx not locked, sets data to and returns
// 0. If locked, sets data to and returns clear time setting.
uint32_t Report::getTime(uint32_t* data) {

    Threads::MutexLock guard(Report::mtx);

    if (!guard.LOCK()) {
        if (data != nullptr) *data = 0;
        return 0;
    }

    // Locked
    if (data != nullptr) *data = this->clrTimeSet;

    return this->clrTimeSet;
}
}