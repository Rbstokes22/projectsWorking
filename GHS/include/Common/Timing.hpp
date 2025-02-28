#ifndef TIMING_H
#define TIMING_H

#include <cstdint>
#include "Threads/Mutex.hpp"

// All timed data and functions go here for preceise timing throughout
// the program.
namespace Clock {

struct TIME {
    uint8_t hour; 
    uint8_t minute;
    uint8_t second;
    uint32_t raw; // raw seconds exclusive format.
};

class DateTime { // Singleton class
    private:
    const char* tag;
    const uint32_t secPerDay = 86400; // Total seconds per day.
    TIME time; // Time structure in hhmmss and raw seconds.
    uint32_t timeCalibrated; // Actual time in seconds past midnight.
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
    void calibrate(int secsPastMid); 
    TIME* getTime();
    bool isCalibrated();
    int64_t micros();
    int64_t millis();
    uint32_t seconds();
};

}
#endif // TIMING_H