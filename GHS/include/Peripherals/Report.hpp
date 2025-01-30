#ifndef REPORT_HPP
#define REPORT_HPP

#include "cstdint"
#include "stddef.h"

namespace Peripheral {

#define TIMER_OFF 99999 // random value which indicates timer off
#define SEND_ATTEMPTS 3 // Max attempts to try to send average message 0 - 255

// Serves as the average clearing if disabled.
#define MAX_SET_TIME 86340  // 23:59:00
#define MAX_TOTAL_SET_TIME 86390 // 23:59:50

// Seconds past timer set, to attempt message sending. If 20 is passed and the
// timer is set to 82800 or 11PM(2300), that means that the message will be
// sending from seconds 82800 to 82820.
#define ATTEMPT_TIME_RANGE 20 // seconds past time set, must not exceed 50.

struct TimerData {
    uint32_t timeSet; // Default to 99999
    bool isSet; // Defaults to false
};

class Report {
    private:
    Report();
    Report(const Report&) = delete; // prevent copying
    Report &operator=(const Report&) = delete; // prevent assignment
    TimerData timer;
    bool compileAll(char* jsonRep, size_t bytes);
    void clearAll();

    public:
    static Report* get();
    void setTimer(uint32_t seconds);
    void manageTimer();
    TimerData* getTimeData();
};

}

#endif // REPORT_HPP