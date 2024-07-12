#ifndef TIMING_H
#define TIMING_H

#include "config.hpp"

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

}
#endif // TIMING_H