#ifndef MSGLOGHANDLER_HPP
#define MSGLOGHANDLER_HPP

#include <cstdint>
#include "UI/IDisplay.hpp"
#include "Threads/Mutex.hpp"

namespace Messaging {

#define LOG_SIZE 8192 // bytes of log information.
#define LOG_MAX_ENTRY 128 // max entry size per log.
#define MLH_DELIM ';' // delimiter used in log entries
#define MLH_DELIM_REP ':' // Replaces delimiter with : if contained in msg.

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
    SRL, // Serial
    SRL_OLED, // Serial and OLED
    SRL_LOG, // Serial and log
    OLED, // OLED
    OLED_LOG, // OLED and log
    LOG, // LOG
    SRL_OLED_LOG // Serial, OLED, and log.
};

extern const char LevelsMap[5][10]; // used for verbosity with enum Levels

// Parameters to control the Messaging, Logging, and Error capability.
struct MsgLogHandlerParams {
    UI::IDisplay &OLED; // Address of OLED object.
    uint8_t msgClearInterval; // Seconds to clear messages from OLED display
    bool serialOn; // Enables serial printing.
};

class MsgLogHandler {
    private:
    MsgLogHandlerParams &params;
    uint32_t msgClearTime;
    bool clrMsgOLED;
    char log[LOG_SIZE];
    bool newLogEntry; // New log message available to client.
    static Threads::Mutex mtx; // Removed <atomic> with mtx implementation.
    MsgLogHandler(MsgLogHandlerParams &params); 
    MsgLogHandler(const MsgLogHandler&) = delete; // prevent copying
    MsgLogHandler &operator=(const MsgLogHandler&) = delete; // prevent assgnmt
    void writeSerial(Levels level, const char* message);
    void writeOLED(Levels level, const char* message);
    void writeLog(Levels level, const char* message); 
    size_t stripLogMsg(size_t newMsgLen);
    
    public:
    static MsgLogHandler* get(MsgLogHandlerParams* params = nullptr);
    void OLEDMessageCheck();
    void handle(Levels level, const char* message, Method method);
    bool getNewLogEntry();
    void resetNewLogFlag();
    const char* getLog();
};

}

#endif // MSGLOGHANDLER_HPP