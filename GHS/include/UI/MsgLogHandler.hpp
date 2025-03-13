#ifndef MSGLOGHANDLER_HPP
#define MSGLOGHANDLER_HPP

#include <cstdint>
#include "UI/IDisplay.hpp"
#include "Threads/Mutex.hpp"

namespace Messaging {

#define LOG_SIZE 16384 // bytes of log information. 16 KB.
#define LOG_MAX_ENTRY 128 // max entry size per log.
#define LOG_MAX_ENTRY_PAD 40 // Used to add time and loc data to log entry.
#define LOG_PREV_MSGS 5 // Used in comparison to current message to avoid repeat
#define MLH_DELIM ';' // delimiter used in log entries
#define MLH_DELIM_REP ':' // Replaces delimiter with : if contained in msg.
#define OLED_QUEUE_QTY 5 // Quantity of messages in queue
#define OLED_MSG_EXPIRATION_SECONDS 2 // Will stop or display next message then


// COLOR CODING escape chars, these can be interjected anywhere.
// Color	Code	Example Escape Code
// Black	30	    \033[30m
// Red	    31	    \033[31m
// Green	32	    \033[32m
// Yellow	33	    \033[33m
// Blue	    34	    \033[34m
// Magenta	35	    \033[35m
// Cyan	    36	    \033[36m
// White	37	    \033[37m
// Reset	0	    \033[0m

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

// Total size of the max entry and pad, is below 200 chars which is the limit
// of the OLED per the datasheet. Set to micros to prevent issues with
// concurrent messaging.
struct Queue {
    char msg[LOG_MAX_ENTRY + LOG_MAX_ENTRY_PAD];
    bool used; // Is spot currently used.
    bool sending; // has message been sent.
    int64_t expiration; // system micros, message will be expired.
};

class MsgLogHandler {
    private:
    const char* tag;
    char errLog[LOG_MAX_ENTRY]; // Error log for local handling.
    Queue OLEDqueue[OLED_QUEUE_QTY]; // queue used to send messages to OLED.
    UI::IDisplay* OLED; // Needs to be added, default to nullptr.
    bool serialOn; // Enables serial printing.
    char log[LOG_SIZE]; // Main log, very large.
    bool newLogEntry; // New log message available to client.
    static Threads::Mutex mtx; // Removed <atomic> with mtx implementation.
    MsgLogHandler(); 
    MsgLogHandler(const MsgLogHandler&) = delete; // prevent copying
    MsgLogHandler &operator=(const MsgLogHandler&) = delete; // prevent assgnmt
    void writeSerial(Levels level, const char* message, uint32_t seconds);
    bool writeOLED(Levels level, const char* message, uint32_t seconds);
    void writeLog(Levels level, const char* message, uint32_t seconds, 
        bool ignoreRepeat); 
    size_t stripLogMsg(size_t newMsgLen);
    bool analyzeLogEntry(const char* message, uint32_t seconds);
    bool OLEDcheck();
    bool OLEDQueueSend();
    bool OLEDQueueRemove();
   
    public:
    static MsgLogHandler* get();
    void OLEDMessageMgr();
    void handle(Levels level, const char* message, Method method, 
        bool ignoreRepeat = false);
    bool newLogAvail();
    void resetNewLogFlag();
    const char* getLog();
    bool addOLED(UI::IDisplay &OLED);
};

}

#endif // MSGLOGHANDLER_HPP