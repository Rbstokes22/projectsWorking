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
void MsgLogHandler::writeSerial(Levels level, const char* message, 
    uint32_t seconds) {

    if (this->serialOn) {
        uint8_t lvl = static_cast<uint8_t>(level);

        printf("%s%s (%lu): %s%s\n", LevelsColors[lvl], LevelsMap[lvl], 
            seconds, message, SRL_RST);
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
void MsgLogHandler::writeLog(Levels level, const char* message, 
    uint32_t seconds, bool ignoreRepeat) {

    if (!ignoreRepeat) { // Prevents the analysis from running and forces log.

        // Check for block. If true, allows a write, if false, it does not. This
        // analyzes repeating entries ensuring logbook pollution control and will
        // write repeat entries only when set conditions are met.
        if (!this->analyzeLogEntry(message, seconds)) return;
    }

    // Max log size throughout the program is limited to 128 bytes. The max
    // entry adds a padding to that in order to allow the level of message,
    // calling function tag, and time.
    const size_t maxEntrySize = LOG_MAX_ENTRY + LOG_MAX_ENTRY_PAD;

    char entry[maxEntrySize] = {0};
    size_t remaining = LOG_SIZE - strlen(this->log) - 1;  // - 1 for null term

    // Write the data to the entry to be concat to the log array. Used sprintf
    // in order to avoid null termination by snprintf, this must be terminated
    // by the delimiter. EXPLICITYLY DID NOT CHECK TO SEE IF WRITTEN EXCEEDS
    // ENTRY SIZE, IT OVER, IT WILL BE TRUNCATED. Format is INFO (seconds): msg
    int written = snprintf(entry, maxEntrySize, "%s (%lu): %s%c", 
        LevelsMap[static_cast<uint8_t>(level)], seconds, message, MLH_DELIM);

    // If written is greater than max entry size, ensures it has a closing
    // delimiter before the null terminator.
    if (written > maxEntrySize) {
        printf("MLH: Message size exceeds max entry\n");
        entry[maxEntrySize - 2] = MLH_DELIM;
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

// Requires the message and current time in seconds. Compares the new entry
// with the previous n entries. If it is the same, returns false unless either
// the consecutive entry timeout is met, or the quantity of consecutive 
// entires is met. Returns true if the new entry does not equal the previous.
bool MsgLogHandler::analyzeLogEntry(const char* message, uint32_t time) {

    // Purpose: This is to prevent an error, such as an error with a peripheral
    // device, to continue logging the same error over and over, polluting the
    // log. If a limit of 3 entries is set, it will allow 3 entries, and then
    // block any additional until the time is greater than or equal to the 
    // reset time. An array of previous messages is used to prevent message
    // clusters from defying this logic. For example, if the SHT driver fails
    // and logs the failure, and it also causes the temphum read to fail, which
    // also logs the failure, it will keep resetting itself preventing the
    // intent of this function.

    static char prevMsgs[LOG_PREV_MSGS][LOG_MAX_ENTRY] = {""};
    static size_t index = 0; // Used with prevMsgs to index new entries.

    // Adds the timeout to the passed time. When the time meets this value,
    // a repeating log entry can be sent.
    static uint32_t resetTime = 0; 
    static size_t consecutive = 0;

    for (int i = 0; i < LOG_PREV_MSGS; i++) { // Iterate all prev messages.

        // Compares current message with each previous message. Action is only
        // taken here if there is a match. If matched, and below the consec
        // entries limit, returns true which will log. If below the limit, will
        // log when the reset time is reached.
        if (strcmp(message, prevMsgs[i]) == 0) {
            consecutive++;

            if (consecutive <= CONSECUTIVE_ENTRIES) {
                resetTime = time + CONSECUTIVE_ENTRY_TIMEOUT; // Establish
                return true; // Allow log
            } else if (time >= resetTime) {
                resetTime = time + CONSECUTIVE_ENTRY_TIMEOUT; // Re-establish
                return true; // Allow log
            }

            return false; // Prevent log.
        }
    }

    // No match to any previous entry. New entry is added into the array.
    snprintf(prevMsgs[index++], LOG_MAX_ENTRY, "%s", message);
    index %= LOG_PREV_MSGS; // Runs 0 to max index, and repeates.
    consecutive = 0;
    return true; // ALlow write, new entry.
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

// Requires the level of error, message, sending method, and option to ignore
// repeated logs which is default to false. Prints the messaging in the 
// applicable method. NOTE: Log max entry is 128 bytes, OLED is 200 bytes, and
// Serial in unrestricted.
void MsgLogHandler::handle(Levels level, const char* message, Method method,
    bool ignoreRepeat) {
   
    uint32_t seconds = Clock::DateTime::get()->seconds();

    switch (method) {
        case Method::SRL: 
        this->writeSerial(level, message, seconds);
        break;

        case Method::SRL_OLED:
        this->writeSerial(level, message, seconds);
        this->writeOLED(level, message);
        break;

        case Method::SRL_LOG:
        this->writeSerial(level, message, seconds);
        this->writeLog(level, message, seconds, ignoreRepeat);
        break;

        case Method::OLED:
        this->writeOLED(level, message);
        break;

        case Method::OLED_LOG:
        this->writeOLED(level, message);
        this->writeLog(level, message, seconds, ignoreRepeat);
        break;

        case Method::LOG:
        this->writeLog(level, message, seconds, ignoreRepeat);
        break;

        case Method::SRL_OLED_LOG:
        this->writeSerial(level, message, seconds);
        this->writeOLED(level, message);
        this->writeLog(level, message, seconds, ignoreRepeat);
        break;

        default:;
    }
}

// Requires no parameters. Returns true if new log available. Used to
// alert client that a new log entry is available to avoid constant polling
// of data.
bool MsgLogHandler::newLogAvail() {
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