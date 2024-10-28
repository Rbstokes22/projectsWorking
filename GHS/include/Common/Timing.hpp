#ifndef TIMING_H
#define TIMING_H

#include <cstdint>

// All timed data and functions go here for preceise timing throughout
// the program.
namespace Clock {

class Timer {
    private:
    uint32_t previousMillis;
    uint32_t interval;
    uint32_t millis();

    public:
    Timer(uint32_t interval); 
    bool isReady();
    void changeInterval(uint32_t newInterval);
};

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