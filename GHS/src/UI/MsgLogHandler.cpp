#include "UI/MsgLogHandler.hpp"
#include <cstdint>
#include "UI/IDisplay.hpp"
#include "Threads/Mutex.hpp"
#include "string.h"
#include "Common/Timing.hpp"
#include "freertos/FreeRTOS.h"
#include "Config/config.hpp"

namespace Messaging {

Threads::Mutex MsgLogHandler::mtx; // Create static instance

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

// Maps to levels. 
const char LevelsMap[5][11]{"[DEBUG]", "[INFO]", "[WARNING]", "[ERROR]",
    "[CRITICAL]"};

const char LevelsColors[5][10]{SRL_CYN, SRL_GRN, SRL_YEL, SRL_RED, SRL_MAG};

// Requires parameters, which should be globally created. 
MsgLogHandler::MsgLogHandler() : 

    msgClearTime(0), msgClearInterval(MSG_CLEAR_SECS), OLED(nullptr),
    serialOn(SERIAL_ON), clrMsgOLED{false}, newLogEntry{false} {

        memset(this->log, 0, LOG_SIZE);
        this->handle(Messaging::Levels::INFO, "(MSGLOGERR) init",
            Messaging::Method::SRL_LOG);
    } 

// Requires level of message, and the message. If serial writing is enabled,
// writes the message to serial output.
void MsgLogHandler::writeSerial(Levels level, const char* message) {
    if (this->serialOn) {
        uint8_t lvl = static_cast<uint8_t>(level);

        printf("%s%s %s%s\n", LevelsColors[lvl], LevelsMap[lvl], message, 
            SRL_RST);
    }
}

// Requires the level of message, and the message. Writes message to the OLED
// by blocking typical OLED display briefly allowing the message to be read.
void MsgLogHandler::writeOLED(Levels level, const char* message) {
    if (this->OLED == nullptr) {
        this->handle(Levels::ERROR, "(MSGLOGERR) OLED not added", Method::SRL);
        return;
    }

    // OLED has 200 char capacity @ 5x7, and is set max for the
    // buffer size. Errors will always display in 5x7 font.
    char msg[static_cast<int>(UI::UIvals::OLEDCapacity)]{}; 

    int writtenChars = snprintf(msg, sizeof(msg), "%s: %s",
    LevelsMap[static_cast<uint8_t>(level)], 
    message);

    if (writtenChars < 0 || writtenChars >= sizeof(msg)) {
        strncpy(msg, "ERROR: Message too long", sizeof(msg) - 1);
        msg[sizeof(msg) -1] = '\0';
    }

    // Displays the message and sets timer to clear the last message.
    this->OLED->displayMsg(msg); 

    // Once called, sets the message clear time and allows the OLED message
    // check function to clear the OLED message when conditions are met.
    uint32_t curSeconds = Clock::DateTime::get()->seconds();
    this->msgClearTime = (curSeconds + this->msgClearInterval);
    this->clrMsgOLED = false; // Allows OLED display.
}

// LOG HANDLING. Keep message below limit or it will be chopped. The format is
// the following entry1;entry2;...;entryn;
void MsgLogHandler::writeLog(Levels level, const char* message) {

    char entry[LOG_MAX_ENTRY] = {0};
    size_t remaining = LOG_SIZE - strlen(this->log) - 1;  // - 1 for null term

    // Write the data to the entry to be concat to the log array. Used sprintf
    // in order to avoid null termination by snprintf, this must be terminated
    // by the delimiter. EXPLICITYLY DID NOT CHECK TO SEE IF WRITTEN EXCEEDS
    // ENTRY SIZE, IT OVER, IT WILL BE TRUNCATED. Format is INFO (seconds): msg
    int written = snprintf(entry, LOG_MAX_ENTRY, "%s (%lu): %s%c", 
        LevelsMap[static_cast<uint8_t>(level)],
        Clock::DateTime::get()->seconds(), message, MLH_DELIM);

    // If written is greater than max entry size, ensures it has a closing
    // delimiter before the null terminator.
    if (written > LOG_MAX_ENTRY) {
        printf("MLH: Message size exceeds max entry\n");
        entry[LOG_MAX_ENTRY - 2] = MLH_DELIM;
    }

    // Iterates the entry - 1, to avoid replacing the closing delimiter. If a
    // delimiter is found, it is replaced with another CHAR.
    for (int i = 0; i < strlen(entry) - 1; i++) {
        if (entry[i] == MLH_DELIM) entry[i] = MLH_DELIM_REP;
    }

    // Checks to see if the entry is larger than the remaining space left in
    // the log. If yes, this will strip the first message off, potentially 
    // several times until the remaining space is large enough to accomodate
    // the new entry.
    if (strlen(entry) >= remaining) { // Purge beginning
        while(true) {
            remaining = this->stripLogMsg(written);
            if (remaining >= written) break;
        }
    }

    // Concatenate entry to log. Automatically null terminates.
    strncat(this->log, entry, written); 
    this->newLogEntry = true; // Used for client to know new entry avialable.
}

// Requires no parameters. Strips the first message from the LOG when a new
// entry is added, but the data space is full. Returns remaining size after
// stripping entry.
size_t MsgLogHandler::stripLogMsg(size_t newMsgLen) {
    int delimPos = -1; // Indicates no delimiter exists

    // Iterates the log to find the first delimiter. Sets position to idx val.
    for (int i = 0; i < LOG_SIZE; i++) {
        if (this->log[i] == MLH_DELIM) {
            delimPos = i;
            break;
        }
    }

    // If -1, means no delimiter and the log will be reset to 0. If not -1,
    // removes the first entry, and moves the remaining log to index 0.
    if (delimPos != -1) {
        size_t start = delimPos + 1; // Account for start of 2nd entry
        size_t moveSize = strlen(this->log) - start; // How many bytes to move

        memmove(this->log, this->log + start, moveSize);
        this->log[moveSize] = '\0'; // Null terminate the new log
    } else { // No messages exist, zero everything out
        memset(this->log, 0, LOG_SIZE);
    }

    // Returns the remaining size
    return LOG_SIZE - strlen(this->log) - 1; // -1 for the null term
}

// Requires no params. Returns static instance to class.
MsgLogHandler* MsgLogHandler::get() {

    // Single use of mutex lock which will ensure to protect any subsequent
    // calls made after requesting this instance.
    Threads::MutexLock(MsgLogHandler::mtx);
    static MsgLogHandler instance;
    
    return &instance;
}

// Monitors the current system time in seconds and compares it with the time
// that the message sent to the OLED is set to clear. Once called, changes the
// override status back to false to allow normal OLED functionality.
void MsgLogHandler::OLEDMessageCheck() {
    if (this->OLED == nullptr) {
        this->handle(Levels::ERROR, "(MSGLOGERR) OLED not added", Method::SRL);
        return;
    }

    uint32_t curSeconds = Clock::DateTime::get()->seconds();

    // Checks the current seconds against the clear time specified in the
    // write OLED function. Once true, sends the OLED override to false to
    // allow normal operation. Sets clearMsgOLED to true to allow a single
    // call when conditions are met.
    if ((curSeconds >= this->msgClearTime) && !this->clrMsgOLED) {
        this->OLED->setOverrideStat(false);
        this->clrMsgOLED = true;
    }
}

// Requires the level of error, message, and methods for sending. Prints the
// message in the applicable method passed. NOTE: Log max entry is 128 bytes,
// OLED is 200 bytes, and serial is unrestricted.
void MsgLogHandler::handle(Levels level, const char* message, Method method) {
   
    switch (method) {
        case Method::SRL: 
        this->writeSerial(level, message);
        break;

        case Method::SRL_OLED:
        this->writeSerial(level, message);
        this->writeOLED(level, message);
        break;

        case Method::SRL_LOG:
        this->writeSerial(level, message);
        this->writeLog(level, message);
        break;

        case Method::OLED:
        this->writeOLED(level, message);
        break;

        case Method::OLED_LOG:
        this->writeOLED(level, message);
        this->writeLog(level, message);
        break;

        case Method::LOG:
        this->writeLog(level, message);
        break;

        case Method::SRL_OLED_LOG:
        this->writeSerial(level, message);
        this->writeOLED(level, message);
        this->writeLog(level, message);
        break;

        default:;
    }
}

// Requires no parameters. Returns true if new log available. Used to
// alert client that a new log entry is available to avoid constant polling
// of data.
bool MsgLogHandler::getNewLogEntry() {
    return this->newLogEntry; 
}

// Sets the value of new log entry to the param. Used in conjuntion with 
// client command to reset the new log entry back to false after receiving
// msg.
void MsgLogHandler::resetNewLogFlag() {
    this->newLogEntry = false;
}

// Requires no parameters. Returns log.
const char* MsgLogHandler::getLog() {
    return this->log;
}

// Requires reference to OLED object. This function is to allow additon of 
// OLED to features, since this class must be init immediately since all other
// classes depend on it.
bool MsgLogHandler::addOLED(UI::IDisplay &OLED) {
    this->OLED = &OLED;

    if (this->OLED == nullptr) {
        this->handle(Levels::WARNING, "(MSGLOGERR) OLED not added",
            Method::SRL_LOG);
            
        return false;
    }

    this->handle(Levels::INFO, "(MSGLOGERR) OLED added",Method::SRL_LOG);

    return true;
}

}