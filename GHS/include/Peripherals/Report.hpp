#ifndef REPORT_HPP
#define REPORT_HPP

#include "cstdint"
#include "stddef.h"
#include "UI/MsgLogHandler.hpp"
#include "Threads/Mutex.hpp"

namespace Peripheral {

#define REPORT_LOG_METHOD Messaging::Method::SRL_LOG
#define REPORT_TIME_PADDING 50 // 50 seconds range, do not exceed 60;
#define REPORT_TAG "(REPORT)"

#define SEND_ATTEMPTS 3 // Max attempts to try to send average message 0 - 255

// Serves as the average clearing if disabled.
#define MAX_SET_TIME 86340  // 23:59:00

class Report {
    private:
    const char* tag;
    char log[LOG_MAX_ENTRY];
    static Threads::Mutex mtx;
    Report();
    Report(const Report&) = delete; // prevent copying
    Report &operator=(const Report&) = delete; // prevent assignment
    uint32_t clrTimeSet;
    bool compileAll(char* jsonRep, size_t bytes);
    void clearAll();
    void sendErr(const char* msg, Messaging::Levels lvl = 
        Messaging::Levels::ERROR);

    public:
    static Report* get();
    void setTimer(uint32_t seconds);
    void manageTimer();
    uint32_t getTime();
};

}

#endif // REPORT_HPP