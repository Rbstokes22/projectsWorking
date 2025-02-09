#include "Common/Timing.hpp"
#include <cstdint>
#include "Threads/Mutex.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

namespace Clock {

Threads::Mutex DateTime::mtx; // Define static mutex

// Requires no parameters. Adjusts the current time to the correct 
// hours, minutes, seconds, and raw time.
void DateTime::adjustTime() { 

    // Difference between now and when calibrated, in seconds.
    // Equivalent to setting the system clock to zero.
    uint32_t timeSinceCal = this->seconds() - this->calibratedAt;
    uint32_t currentTime = timeSinceCal + this->timeCalibrated;

    // Total whole days that has been running since calibrated.
    uint16_t days = currentTime / this->secPerDay;

    // Computes offset which will yield a value between 0 - 86400.
    this->time.raw = currentTime - (days * this->secPerDay);

    // calibrates the current time 
    this->setHHMMSS(this->time.raw);
}

// Requires the seconds past midnight. Converts to hour, minute, and second.
void DateTime::setHHMMSS(uint32_t seconds) {

    this->time.hour = seconds / 3600;
    this->time.minute = (seconds - (this->time.hour * 3600)) / 60;
    this->time.second = 
        seconds - (this->time.hour * 3600) - (this->time.minute * 60);
}

DateTime::DateTime() : 
    
    time{0, 0, 0, 0}, timeCalibrated(0), calibratedAt(0), calibrated(false) {}

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
void DateTime::calibrate(int secsPastMid) {
    this->time.raw = static_cast<uint32_t>(secsPastMid);
    this->setHHMMSS(secsPastMid);
    this->timeCalibrated = secsPastMid; // Time this was calibrated
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

// Returns int64_t system runtime in millis.
int64_t DateTime::millis() {
    return micros() / 1000;
}

// Returns uint32_t system runtime in seconds.
uint32_t DateTime::seconds() {
    return millis() / 1000;
}

}