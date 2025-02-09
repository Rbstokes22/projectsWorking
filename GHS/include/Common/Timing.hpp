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
    uint32_t raw;
};

class DateTime { // Singleton class
    private:
    const uint32_t secPerDay = 86400;
    TIME time;
    uint32_t timeCalibrated;
    uint32_t calibratedAt;
    bool calibrated;
    static Threads::Mutex mtx;
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