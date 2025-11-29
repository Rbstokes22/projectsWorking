#include "UI/MsgLogHandler.hpp"
#include <cstdint>
#include "UI/IDisplay.hpp"
#include "Threads/Mutex.hpp"
#include "string.h"
#include "Common/Timing.hpp"
#include "freertos/FreeRTOS.h"
#include "Config/config.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Network/NetManager.hpp"

namespace Messaging {

Threads::Mutex MsgLogHandler::mtx(MLH_TAG); // Create static instance

// Maps to levels. Used in printing.
const char LevelsMap[5][11]{"[DEBUG]", "[INFO]", "[WARNING]", "[ERROR]",
    "[CRITICAL]"};

// Used for serial display, allows different colors for error types.
const char LevelsColors[5][10]{SRL_CYN, SRL_GRN, SRL_YEL, SRL_RED, SRL_MAG};

// Requires no params allowing immediate use. Relies on timing, and will likely
// init the dateTime singleton.
MsgLogHandler::MsgLogHandler() : 

    tag(MLH_TAG), OLED(nullptr), serialOn(SERIAL_ON), newLogEntry{false} {

        memset(this->errLog, 0, sizeof(this->errLog));
        memset(this->log, 0, LOG_SIZE);
        memset(this->OLEDqueue, 0, sizeof(this->OLEDqueue));
        
        snprintf(this->errLog, sizeof(this->errLog), "%s init", this->tag);
        this->handle(Levels::INFO, this->errLog, Method::SRL_LOG);
    } 

// Requires level of message, and the message. If serial writing is enabled,
// writes the message to serial output.

// Requires the message level, the message, and the system seconds invoked.
// If serial writing is enabled, writes message to serial output with proper
// coloring.
void MsgLogHandler::writeSerial(Levels level, const char* message, 
    uint32_t seconds) {

    if (this->serialOn) {
        uint8_t lvl = static_cast<uint8_t>(level);

        printf("%s%s (%lu): %s%s\n", LevelsColors[lvl], LevelsMap[lvl], 
            seconds, message, SRL_RST);
    }
}

// Requires level, message, and system time in seconds. Writes message to
// first available queue spot to be handled by the OLED message manager.
// Returns true upon sucessful add, and false if not.
bool MsgLogHandler::writeOLED(Levels level, const char* message, 
    uint32_t seconds) {

    if (!this->OLEDcheck()) return false; // Block.

    // When invoking this method, this will always send something to the
    // queue to be sent if a spot is available. When this method exits,
    // there will always be something in the queue, unlike the manager.

    int written = -1; // chars written to buffer. -1 for err handling.
    uint8_t lvl = static_cast<uint8_t>(level);

    Queue* queue = nullptr; // Will be reset upon queue availability.

    // Iterate the queue to set the the queue pointer to the queue of interest.
    for (int i = 0; i < OLED_QUEUE_QTY; i++) {
        if (!OLEDqueue[i].used) { // spot open.
            queue = &OLEDqueue[i]; // Set pointer to point at message.
            break; // Exit after first success.
        }
    }

    if (queue == nullptr) { // No spots available.
        snprintf(this->errLog, sizeof(this->errLog), "%s No OLED msg avail", 
            this->tag);

        this->handle(Levels::ERROR, this->errLog, Method::SRL_LOG);
        return false; // Block if no availabilities.
    }

    // Success. Write to the OLED queue, which will be managed by the OLED 
    // message manager. Set the expiration to current time to allow manager
    // to find the first message in queue.
    written = snprintf(queue->msg, sizeof(queue->msg), "%s (%lu): %s",
        LevelsMap[lvl], seconds, message);
    
    if (written < 0) {
        return false;

    } else if (written > LOG_MAX_ENTRY) {

        snprintf(this->errLog, sizeof(this->errLog), 
            "%s OLED msg exceeds max size, truncated", this->tag);
        
        this->handle(Levels::ERROR, this->errLog, Method::SRL_LOG);
    }
    
    // Set queue variables if successful message write. Sets expiration to
    // the time message was set. This is to allow the manager to see the oldest
    // message in the queue and handle it first.
    queue->expiration = Clock::DateTime::get()->micros(); 
    queue->used = true;
    queue->sending = false; // Will set to true once actually sent.

    return true; // Added to queue.
}

// Requires level, message, time in seconds, ignore repeating entries, and
// big log. Repeating entries are filtered to prevent log pollution, for ex:
// in the event of a disconnected sensor wire. Those will display at a set
// interval. IF ignore repeat is set to true, it will repeat chosen entries,
// such as a class logging the creation of several objects at once. The big log
// will allow up to 512 bytes vs 128 bytes. This must be set to true to allow
// the increased size of up to 512 bytes. The log is sent via http in the 
// format: entry1;entry2;...;entryn; with the delimiter being a semicolon.
void MsgLogHandler::writeLog(Levels level, const char* message, 
    uint32_t seconds, bool ignoreRepeat, bool bigLog) {

    // Prevents the analysis from running if set to ignore repeat is set to
    // false or if sending a big log entry.
    if (!ignoreRepeat || !bigLog) { 

        // Check for block. If true, allows a write, if false, it does not. This
        // analyzes repeating entries ensuring logbook pollution control and 
        // will write repeat entries only when set conditions are met.
        if (!this->analyzeLogEntry(message, seconds)) return;
    }

    // Max log size throughout the program is limited to 128 bytes. The max
    // entry adds a padding to that in order to allow the level of message,
    // calling function tag, and time. If using a big log, the max entry will
    // be set to 512 bytes, plus this padding.
    const size_t maxEntrySize = bigLog ? BIGLOG_MAX_ENTRY + LOG_MAX_ENTRY_PAD :
                                         LOG_MAX_ENTRY + LOG_MAX_ENTRY_PAD;
    
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

// Requires the size of the new message length. This strips the first message
// from the log when a new entry is added, but the log is full. Returns the
// remaining size after the beginning entry(ies) is/are stripped.
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

    // These arrays will be responsible to hold the previous n entries. This
    // allows for specific control for each entry, rather than other entries
    // affecting other entries.
    static char prevMsgs[LOG_PREV_MSGS][LOG_MAX_ENTRY] = {""};
    static uint32_t resetTimes[LOG_PREV_MSGS]{0}; // Allows log to be re-ran
    static size_t consecutiveCts[LOG_PREV_MSGS]{0}; // Allows n consec logs.

    static size_t index = 0; // Used with prevMsgs to index new entries.

    for (int i = 0; i < LOG_PREV_MSGS; i++) { // Iterate all prev messages.

        // Compares current message with each previous message. Action is only
        // taken here if there is a match. If matched, and below the consec
        // entries limit, returns true which will log. If below the limit, will
        // log when the reset time is reached.
        if (strcmp(message, prevMsgs[i]) == 0) {
            consecutiveCts[i]++; 

            // First check entries.
            if (consecutiveCts[i] <= CONSECUTIVE_ENTRIES) {
                resetTimes[i] = time + CONSECUTIVE_ENTRY_TIMEOUT; // Establish
                return true; // Allow log

            // Then times if all consecutive entries have been logged.
            } else if (time >= resetTimes[i]) {
                resetTimes[i] = time + CONSECUTIVE_ENTRY_TIMEOUT; // Re-estab
                return true; // Allow log
            }

            return false; // Prevent log.
        }
    }

    // No match to any previous entry. New entry is added into the array.
    snprintf(prevMsgs[index], LOG_MAX_ENTRY, "%s", message);
    consecutiveCts[index] = 0; // Set count to 0.
    index++; // Increment after adding values and adjusting times.

    // Runs between 0 and max. This ensures that the log is cyclic 0 - n, 0 - n,
    // always clearing the oldest entires next. 
    index %= LOG_PREV_MSGS; 
    
    return true; // ALlow write, new entry.
}

// Requires no params. Returns true if OLED != nullptr and can proceed.
bool MsgLogHandler::OLEDcheck() {
    if (this->OLED == nullptr) {
        snprintf(this->errLog, sizeof(this->errLog), "%s OLED req adding",
            this->tag);

        this->handle(Levels::WARNING, this->log, Method::SRL_LOG);
    }

    return (this->OLED != nullptr);
}

// Requires no params. Checks the queue for the message with the longest time
// inside. Changes to status and sends that message if and only if another
// message isnt currently being displayed. Works hand-in-hand with 
// OLEDQueueRemove().
bool MsgLogHandler::OLEDQueueSend() {
    // Unlike the writeOLED(), there may not always be something in the queue
    // which means that special iterations are required to ensure proper
    // management.

    int64_t maxExp = INT64_MAX; // Allows proper setting in queue iteration.
    Queue* queue = nullptr;

    // First gather the required metadata. This will return the index of the
    // message with the first expiration time, that is currently being used
    // and not in the sending process.
    for (int i = 0; i < OLED_QUEUE_QTY; i++) {

        // If any messages are sending, this will return until removed from
        // sending status by the OLEDQueueRemove function.
        if (this->OLEDqueue[i].used == true) { // Message is in queue.
            if (this->OLEDqueue[i].sending) return false; // Block if sending
         
            // If not sending, check expiration time. If it is lower than
            // the previous, will set the first expiration time to the exp
            // time from the queue. Sets queue to the one with the most
            // time in the queue.
            if (this->OLEDqueue[i].expiration < maxExp) {
                maxExp = this->OLEDqueue[i].expiration;
                queue = &this->OLEDqueue[i]; 
            }
        }
    }

    if (queue == nullptr) return false; // No messages in queue.

    // If good, adjust queue parameters and send message.
    queue->sending = true;
    queue->expiration = Clock::DateTime::get()->micros() +
        (1000000 * OLED_MSG_EXPIRATION_SECONDS);

    this->OLED->setOverrideStat(true); // Block updates until expired.
    this->OLED->displayMsg(queue->msg);
    return true;
}

// Requires no parameters. Iterates the queue to ensure that any messages that
// were sent, and their expiration time is met, are deleted and their params
// are reset.
bool MsgLogHandler::OLEDQueueRemove() {
    
    Queue* queue = nullptr;
    size_t remaining = 0;

    // Iterate the queue. Only one message can be set to sending and this will
    // set the queue to point at the first sending message.
    for (int i = 0; i < OLED_QUEUE_QTY; i++) {

        remaining += this->OLEDqueue[i].used; // Incremement to total usage.

        if (this->OLEDqueue[i].sending) { 
            queue = &this->OLEDqueue[i];
            // No need to break, only one sending status at a time.
        }
    }

    if (queue == nullptr) return false; // Nothing to remove.

    // If expired, reset params to allow re-use. Will not reset to override
    // status to false until the queue is clear. Uses 1, since 1 accounts
    // for the current message being deleted.
    if (Clock::DateTime::get()->micros() >= queue->expiration) {
        memset(queue, 0, sizeof(*queue));
        if (remaining <= 1) this->OLED->setOverrideStat(false);
        return true;
    }

    return false; // Indicates no removal occured.
}

// Requires level, message, and time in seconds. This is prepared by the 
// handle function.
void MsgLogHandler::sendUDPLog(Levels level, const char* msg, 
    uint32_t seconds) {

    char mDNS[64];

    // Only allow UDP if conn to LAN. If successful return, mDNS is updated.
    if (!Comms::NetManager::onLAN(mDNS, sizeof(mDNS))) return; 
   
    // ATTENTION. All logs will attempt to be sent by UDP, which will only work
    // after device connected to LAN. Until then, they will just fire into the
    // abyss. Upon the receipt of a log, it is the responsibility of the client
    // to get the total log, inspect it, and aggregate the log entries together.

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP); // Create socket.

    if (sock < 0) return;

    char log[BIGLOG_MAX_ENTRY + 64] = {0}; // Some padding for mdns.

    // Before processing and mtx, send log entry. WARNING. Logs will be sent
    // to the UDP abyss, and will only be received once this is connected to 
    // the LAN.
    int written = snprintf(log, sizeof(log), 
        "{\"mdns\": \"%s\", \"entry\": \"%s (%lu): %s%c\"}", mDNS,
        LevelsMap[static_cast<uint8_t>(level)], seconds, msg, MLH_DELIM);

    if (written <= 0) return; // Indicates unsuccessful write.

    struct sockaddr_in dest; 
    dest.sin_family = AF_INET;
    dest.sin_port = htons(LOG_UDP_PORT);
    dest.sin_addr.s_addr = inet_addr(UDP_URL);

    sendto(sock, log, strlen(log), 0, (struct sockaddr *)&dest, sizeof(dest));

    close(sock);
}

// Requires no params. Returns static instance to class.
MsgLogHandler* MsgLogHandler::get() {

    static MsgLogHandler instance;

    return &instance;
}

// Requires no params. Checks for new queue messages to send, and sent message
// to remove once expired. WARNING: This must be called externally on an 
// interval, in order to properly execute. If not called, messages will never
// send or be removed. Recommend calling this function at 1 Hz.
void MsgLogHandler::OLEDMessageMgr() {

    Threads::MutexLock guard(MsgLogHandler::mtx);
    if (!guard.LOCK()) {
        return; // Block if locked.
    }

    if (!this->OLEDcheck()) return; // BLOCK

    this->OLEDQueueSend();
    this->OLEDQueueRemove();
}

// Requires the level of error, message, sending method, option to ignore 
// repeated logs (def false), and if bigLog (def false), which exceeds the 
// typlical log entry max of 128 bytes, at 512 bytes. OLED is restricted to 200 
// bytes, and serial is unrestricted.
void MsgLogHandler::handle(Levels level, const char* message, Method method,
    bool ignoreRepeat, bool bigLog) {

    uint32_t seconds = Clock::DateTime::get()->seconds();

    this->sendUDPLog(level, message, seconds); // Sends UDP upon every msg.

    Threads::MutexLock guard(MsgLogHandler::mtx);
    if (!guard.LOCK()) {
        return; // Block if locked.
    }
   
    switch (method) {
        case Method::SRL: 
        this->writeSerial(level, message, seconds);
        break;

        case Method::SRL_OLED:
        this->writeSerial(level, message, seconds);
        this->writeOLED(level, message, seconds);
        break;

        case Method::SRL_LOG:
        this->writeSerial(level, message, seconds);
        this->writeLog(level, message, seconds, ignoreRepeat, bigLog);
        break;

        case Method::OLED:
        this->writeOLED(level, message, seconds);
        break;

        case Method::OLED_LOG:
        this->writeOLED(level, message, seconds);
        this->writeLog(level, message, seconds, ignoreRepeat, bigLog);
        break;

        case Method::LOG:
        this->writeLog(level, message, seconds, ignoreRepeat, bigLog);
        break;

        case Method::SRL_OLED_LOG:
        this->writeSerial(level, message, seconds);
        this->writeOLED(level, message, seconds);
        this->writeLog(level, message, seconds, ignoreRepeat, bigLog);
        break;

        default:;
    }
}

// Requires no parameters. Returns true if new log available. Used to
// alert client that a new log entry is available to avoid constant polling
// of data.
bool MsgLogHandler::newLogAvail() { // No mtx req Read Only.
    return this->newLogEntry; 
}

// Sets the value of new log entry to the param. Used in conjuntion with 
// client command to reset the new log entry back to false after receiving
// msg.
void MsgLogHandler::resetNewLogFlag() {

    Threads::MutexLock guard(MsgLogHandler::mtx);
    if (guard.LOCK()) {
        this->newLogEntry = false;
    }
}

// Param data is def to nullptr. If mtx is unlocked, sets to empty string and
// returns empty string. If locked, sets data to and returns log.
// WARNING. If data is passed as a local var pointer, ensure that the size is
// 16384, as this is the log size.
const char* MsgLogHandler::getLog(char* data) {

    static const char safeEmpty[] = "";

    Threads::MutexLock guard(MsgLogHandler::mtx);

    if (!guard.LOCK()) {
        if (data != nullptr) data[0] = '\0';
        return safeEmpty;
    }

    // Locked
    if (data != nullptr) {
        memcpy(data, this->log, sizeof(this->log));
    }

    return this->log;
}

// Requires reference to OLED object. This function is to allow additon of 
// OLED to features, since this class must be init immediately since all other
// classes depend on it.
bool MsgLogHandler::addOLED(UI::IDisplay &OLED) {

    Threads::MutexLock guard(MsgLogHandler::mtx);
    if (!guard.LOCK()) {
        return false; // Block if locked.
    }

    this->OLED = &OLED;

    if (this->OLED == nullptr) {
        snprintf(this->errLog, sizeof(this->errLog), "%s OLED not added", 
            this->tag);

        this->handle(Levels::WARNING, this->errLog, Method::SRL_LOG);
        return false;
    }

    snprintf(this->errLog, sizeof(this->errLog), "%s OLED added", this->tag);
    this->handle(Levels::INFO, this->errLog, Method::SRL_LOG);

    return true;
}

}