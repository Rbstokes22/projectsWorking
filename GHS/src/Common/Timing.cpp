#include "Common/Timing.hpp"
#include <cstdint>
#include "Threads/Mutex.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

// NO logging due to circular dependancy issue. Not an issue throughout the
// program but only here. This is because when this object is created, it 
// logs its creation. It cant log if the msglogerr isnt init, so it cant be
// created first. The way these singletons are designed, if no params are req,
// then the first get() will init that static instance and it is returned. So
// the msglogerr will log its creation, which will then init this DateTime
// singleton. Then this singleton attempts to log using the msglogerr, but 
// the msglogerr is only partially init, since the creation of the DT singleton
// is in the init steps.

namespace Clock {

Threads::Mutex DateTime::mtx("(DATETIME)"); 

// Requires no parameters. Adjusts the current time to the correct 
// hours, minutes, seconds, and raw time.
void DateTime::adjustTime() { 

    // Difference between now and when calibrated, in seconds.
    // Equivalent to setting the system clock to zero.
    uint32_t timeSinceCal = this->seconds() - this->calibratedAt;
    uint32_t currentTime = timeSinceCal + this->timeCalibrated;

    // Total whole days that has been running since calibrated. Max 65535
    uint16_t days = currentTime / SEC_PER_DAY;

    // Computes offset which will yield a value between 0 - 86400.
    this->time.raw = currentTime - (days * SEC_PER_DAY);

    // Computes day current day. 0 = monday, 6 = sunday. 
    this->time.day = (this->dayCalibrated + days) % 7; 

    // calibrates the current time 
    this->setHHMMSS(this->time.raw);
}

// Requires the seconds past midnight. Converts to hour, minute, and second.
void DateTime::setHHMMSS(uint32_t seconds) {

    this->time.hour = seconds / 3600;
    this->time.minute = (seconds % 3600) / 60;
    this->time.second = seconds % 60;
}

DateTime::DateTime() : 
    
    time{0, 0, 0, 0, 0}, timeCalibrated(0), dayCalibrated(0), calibratedAt(0), 
    calibrated(false) {}

// Singleton class, returns instance a pointer instance of this class.
DateTime* DateTime::get() {

    // Single use of mutex lock which will ensure to protect any subsequent
    // calls made after requesting this instance.
    Threads::MutexLock(DateTime::mtx);
    
    static DateTime instance;
    return &instance;
}

// Requires an int of the seconds past midnight. Calibrates the hhmmss,
// and sets baseline for future computations.
void DateTime::calibrate(int secsPastMid, uint8_t day) {

    // Prevents time calibration during reporting and log events at the 
    // midnight mark. 
    if (secsPastMid <= CALIB_BLOCK_TIME || 
            secsPastMid >= (SEC_PER_DAY - CALIB_BLOCK_TIME)) return;

    this->time.raw = static_cast<uint32_t>(secsPastMid);
    this->setHHMMSS(secsPastMid);
    this->timeCalibrated = secsPastMid; // Time this was calibrated
    this->dayCalibrated = day;
    this->time.day = day; // Set day once calibrated.
    this->calibrated = true;
    this->calibratedAt = this->seconds(); // calibrated at this sys runtime
}

// Returns TIME struct with hour, minute, second.
TIME* DateTime::getTime() {
    this->adjustTime(); // Adjusts before returning time.
    return &this->time;  
}

// Returns if the clock has been calibrated.
bool DateTime::isCalibrated() {
    return this->calibrated;
}

// Returns int64_t system runtime in micros. Can run for 
// 2924 centuries before rollover at this level of precision.
int64_t DateTime::micros() {
    return esp_timer_get_time();
}

// Returns int64_t system runtime in millis. WARNING: there is some precision 
// lost due to division operation, meaning the result will be always be floored.
// The precison is ranged from 0, dead accurate, to 999 micros being floored to
// zero, resulting in an accurace range of 0 - 999 micros, or 0 to 0.999 millis.
int64_t DateTime::millis() {
    return micros() / 1000;
}

// Returns uint32_t system runtime in seconds. WARNING: there is some precision
// lost due to division operation, meaning the result will always be floored.
// The precision is ranged from 0, dead accurate, to 999 millis beging floored
// to zero, resulting in an accuracy range of 0 - 999 millis, or 0 - 0.999 sec.
// Combined with the millis() accuracy range, seconds will be anywhere from 0
// to 0.999999 seconds off.
uint32_t DateTime::seconds() {
    return millis() / 1000;
}

}