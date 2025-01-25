#ifndef AVERAGES_H
#define AVERAGES_H

#include "cstdint"

namespace Peripheral {

#define TIMER_OFF 99999 // random value which indicates timer off
#define SEND_ATTEMPTS 3 // Max attempts to try to send average message 0 -255

// Seconds past timer set, to attempt message sending. If 20 is passed and the
// timer is set to 82800 or 11PM(2300), that means that the message will be
// sending from seconds 82800 to 82820.
#define ATTEMPT_TIME_RANGE 20 // seconds past time set, must not exceed 60

class Averages {
    private:
    Averages();
    Averages(const Averages&) = delete; // prevent copying
    Averages &operator=(const Averages&) = delete; // prevent assignment
    uint32_t timeSet; // Defaults to 99999
    bool isSet; // Defaults to false
    const char* compileAll();

    public:
    static Averages* get();
    void setTimer(uint32_t seconds);
    void manageTimer();
};

}

#endif // AVERAGES_H