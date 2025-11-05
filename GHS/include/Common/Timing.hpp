#ifndef TIMING_H
#define TIMING_H

#include <cstdint>
#include "Threads/Mutex.hpp"

// All timed data and functions go here for preceise timing throughout
// the program.
namespace Clock {

#define SEC_PER_DAY 86400 // Seconds per day
#define CALIB_BLOCK_TIME 10 // Radius of time from midnight where calibrations
                            // will be blocked in order to prevent reporting
                            // and log errors.

struct TIME {
    uint8_t hour; 
    uint8_t minute;
    uint8_t second;
    uint32_t raw; // raw seconds exclusive format.
    uint8_t day; // Day number, mon = 0, sun = 6.
};

class DateTime { // Singleton class
    private:
    TIME time; // Time structure in hhmmss and raw seconds.
    uint32_t timeCalibrated; // Actual time in seconds past midnight.
    uint8_t dayCalibrated; // Actual day number calibrated.
    uint32_t calibratedAt; // system run time in seconds when cal occured.
    bool calibrated; // Has this been calibrated.
    static Threads::Mutex mtx; // mutex.
    void adjustTime();
    void setHHMMSS(uint32_t seconds);
    DateTime(); 
    DateTime(const DateTime&) = delete; // prevent copying
    DateTime &operator=(const DateTime&) = delete; // prevent assignment

    public:
    static DateTime* get();
    void calibrate(int secsPastMid, uint8_t day); 
    TIME* getTime();
    bool isCalibrated();
    int64_t micros();
    int64_t millis();
    uint32_t seconds();
};

}
#endif // TIMING_H