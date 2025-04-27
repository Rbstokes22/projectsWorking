#include "Peripherals/Report.hpp"
#include "cstdint"
#include "stddef.h"
#include "Common/Timing.hpp"
#include "Peripherals/TempHum.hpp"
#include "Peripherals/Soil.hpp"
#include "Peripherals/Light.hpp"
#include "UI/MsgLogHandler.hpp"
#include "Network/Handlers/socketHandler.hpp"

// No mutex required, Accessed from a single thread

namespace Peripheral {

Report::Report() : tag("(REPORT)"), clrTimeSet(MAX_SET_TIME) {

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
    Messaging::MsgLogHandler::get()->handle(lvl, msg, REPORT_LOG_METHOD);
}

// Requires the seconds past midnight to clear the averages. If the value 
// exceeds 86340, or 23:59:00, it will be set to that default time. 
void Report::setTimer(uint32_t seconds) {

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

    Clock::TIME* dtg = Clock::DateTime::get()->getTime();
    uint8_t hour = dtg->hour;
    static uint8_t lastHour = hour; // Set static upon init.
    static uint8_t attempts = 0; // Sending attempts

    // Raw systime gives the seconds past midnight from 0 - 86399.
    uint32_t sysTime = dtg->raw;

    // clear toggle, day toggle.
    static bool clrT = true, dayT = true;

    bool inRange = (sysTime >= this->clrTimeSet) && 
        (sysTime <= (this->clrTimeSet + REPORT_TIME_PADDING));

    // Clear all timer
    if (clrT && inRange) { // Not cleared, systime is in range.
        clrT = false;
        this->clearAll();

    } else if (!clrT && !inRange) { // Has cleared and systime is out of range.
        clrT = true; // Reset
    }

    // daily log timer. Logs between 23:59:55 and midnight.
    inRange = (sysTime >= 86390) && (sysTime <= 86400);

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

        // Attempt to send report via alert class.
        bool sent = Alert::get()->sendReport(Comms::SOCKHAND::getMasterBuf());

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

// gets the current set time and returns.
uint32_t Report::getTime() {return this->clrTimeSet;}
}