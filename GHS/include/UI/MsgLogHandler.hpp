#ifndef MSGLOGHANDLER_HPP
#define MSGLOGHANDLER_HPP

#include <cstdint>
#include "UI/IDisplay.hpp"
#include "Threads/Mutex.hpp"

namespace Messaging {

#define LOG_SIZE 16384 // bytes of log information. 16 KB.
#define LOG_MAX_ENTRY 128 // max entry size per log.
#define MLH_DELIM ';' // delimiter used in log entries
#define MLH_DELIM_REP ':' // Replaces delimiter with : if contained in msg.
#define SRL_RED "\033[31m" // Red text
#define SRL_GRN "\033[32m" // Green text
#define SRL_YEL "\033[33m" // Yellow text
#define SRL_CYN "\033[36m" // Cyan text
#define SRL_MAG "\033[35m" // Magenta text
#define SRL_RST "\033[0m" // Reset text

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

extern const char LevelsMap[5][11]; // used for verbosity with enum Levels
extern const char LevelsColors[5][10]; // Used for serial colors

class MsgLogHandler {
    private:
    uint32_t msgClearTime;
    uint8_t msgClearInterval; // Seconds to clear messages from OLED display.
    UI::IDisplay* OLED; // Needs to be added, default to nullptr.
    bool serialOn; // Enables serial printing.
    bool clrMsgOLED;
    char log[LOG_SIZE];
    bool newLogEntry; // New log message available to client.
    static Threads::Mutex mtx; // Removed <atomic> with mtx implementation.
    MsgLogHandler(); 
    MsgLogHandler(const MsgLogHandler&) = delete; // prevent copying
    MsgLogHandler &operator=(const MsgLogHandler&) = delete; // prevent assgnmt
    void writeSerial(Levels level, const char* message);
    void writeOLED(Levels level, const char* message);
    void writeLog(Levels level, const char* message); 
    size_t stripLogMsg(size_t newMsgLen);
    
    public:
    static MsgLogHandler* get();
    void OLEDMessageCheck();
    void handle(Levels level, const char* message, Method method);
    bool getNewLogEntry();
    void resetNewLogFlag();
    const char* getLog();
    bool addOLED(UI::IDisplay &OLED);
};

}

#endif // MSGLOGHANDLER_HPP