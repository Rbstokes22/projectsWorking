#ifndef MSGLOGHANDLER_H
#define MSGLOGHANDLER_H

#include <Arduino.h>
#include "UI/IDisplay.h"
#include "Common/Timing.h"



namespace Messaging {

enum class Levels : uint8_t {

    // Detailed information for diagnosing problems. This can help
    // trace execution flow and see how data is being manipulated.
    DEBUG, 

    // Confirmation things are working as expected. 
    INFO, 

    // Something unexpected happened and the software is still running
    // but there could be an issue in the future.
    WARNING, 

    // Due to a more serious problem, the software has not been able to 
    // perform a certain function. 
    ERROR, 

    // A serious issue that requires immediate attention. The program
    // may be unable to continue.
    CRITICAL 
};

enum class Method : uint8_t {
    SRL, // output to serial if running.
    OLED, // output to OLED if running.
    LOG, // Log to NVS
    NONE // No method, default argument for methods 2 and 3
};

extern const char LevelsMap[5][10]; // used for verbosity with enum Levels

class MsgLogHandler {
    private:
    UI::IDisplay &OLED;
    Clock::Timer &OLEDclear;
    bool serialOn;
    uint32_t msgClearTime;
    uint8_t msgClearSeconds;
    bool isMsgReset;
    
    public:
    MsgLogHandler(
        UI::IDisplay &OLED, uint8_t msgClearInterval = 5, bool serialOn = false);
    void writeSerial(Levels level, const char* message);
    void writeOLED(Levels level, const char* message);
    void writeLog(Levels level, const char* message);
    void prepMessage(Method method, Levels level, const char* message);
    void OLEDMessageCheck();
    void handle(
        Levels level, const char* message,
        Method method1, 
        Method method2 = Method::NONE, 
        Method method3 = Method::NONE);
};

}

#endif // MSGLOGHANDLER_H