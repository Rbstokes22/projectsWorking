#include "Common/heartbeat.hpp"
#include "UI/MsgLogHandler.hpp"
#include "string.h"
#include "Peripherals/saveSettings.hpp"
#include "Network/NetSTA.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// ATTENTION. Avoid logging any heartbeat suspensions and releases. This is 
// because it has the potential to be called frequently and we do not want to
// pollute the logs.

namespace heartbeat {

Threads::Mutex Heartbeat::mtx("HeartBeat");
uint8_t Heartbeat::blockNum = 0; // Init to 0.

Heartbeat::Heartbeat(const char* tag) : tag(tag), suspensionReg(0),
    allSus(false) {

    // zero out all tracking arrays.
    memset(this->reg, 0, sizeof(this->reg));
    memset(this->failures, 0, sizeof(this->failures));
    memset(this->callers, 0, sizeof(this->callers));

    snprintf(this->log, sizeof(this->log), "%s ob created", this->tag);
    Messaging::MsgLogHandler::get()->handle(Messaging::Levels::INFO,
        this->log, Messaging::Method::SRL_LOG);
}

// Requires ID. Checks to see if all heartbeats have been suspended or the
// ID exceeds the allocated block. Returns true to block code in each function
// requiring it. Also returns true if suspension flag is active. Returning 
// false indicates that there is no suspension or error. 
bool Heartbeat::checkSus(uint8_t ID) {

    if (this->allSus || ID >= HEARTBEAT_CLIENTS_MAX) return true;

    return (this->suspensionReg >> ID) & 0x1;
}

// Requires ID. Will not show caller, just the ID. Use the log to 
// check the caller name assigned to the ID. Currently logs the 
// unresponsiveness and restarts if unresponsive and directed, and grace period
// is unmet.
void Heartbeat::alert(uint8_t ID) {

    snprintf(this->log, sizeof(this->log), "%s ID %u Caller %s unresponsive", 
        this->tag, ID, this->callers[ID]);

    // Log tail entry, which will be visible upon a save and restart.
    Messaging::MsgLogHandler::get()->handle(Messaging::Levels::CRITICAL,
        this->log, Messaging::Method::SRL_LOG);

    // WARNING.
    // If not rogered up by next check, failure will increment upon next
    // heartbeat check in this->manage(). If reset is set to 3, this will trip
    // the first time, and 1 seond later, and another second later. Adjust 
    // trips if this becomes problematic.
    this->failures[ID]++; // Increment upon a failure

    // Restart with consecutive failures. Unless consecutive, will be reset to
    // 0 upon successful heartbeat check.
    if (this->failures[ID] >= HEARTBEAT_RESET_FAILS) {
        NVS::settingSaver::get()->saveAndRestart(); // Restart after log.
    }
}

// Singleton class, returns instance pointer of this class.
Heartbeat* Heartbeat::get() {

    static Heartbeat instance(HEARTBEAT_TAG);
    return &instance;
}

// Requires the caller ID and the init value. The init value will set the array
// block to this value during the init process due to delays in order to 
// prevent unwanted behavior by the alert funciton. Returns the position in the
// uint8_t array that is assigned to the caller. WARNING: Caller must be 15
// chars or less, with the 16th char being the null terminator. Caller will be
// truncated if in violation.
uint8_t Heartbeat::getBlockID(const char* caller, uint8_t initVal) {

    Threads::MutexLock guard(this->mtx);
    if (!guard.LOCK()) {
        return HEARTBEAT_CLIENTS_FULL; // Blocks use 
    }

    if (blockNum < HEARTBEAT_CLIENTS_MAX) {

        snprintf(this->log, sizeof(this->log), 
            "%s block ID %u registered to %s", this->tag, this->blockNum,
            caller);

        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::INFO,
         this->log, Messaging::Method::SRL_LOG);

        this->rogerUp(blockNum, initVal); // Sets the init val.

        snprintf(this->callers[blockNum], HEARTBEAT_CALLER_CHAR_LEN, "%s", 
            caller); // Copy caller to later reference if needed.

        this->failures[blockNum] = 0; // Set to 0 upon assignment.
        
        return blockNum++; // Increment after return.

    } else {

        snprintf(this->log, sizeof(this->log), "%s Clients full", this->tag);
        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::ERROR,
         this->log, Messaging::Method::SRL_LOG);

        return HEARTBEAT_CLIENTS_FULL; // Return value indicating no space.
    }
}

// Requires the chunkID which is the index of the array which holds the reset
// time. The reset time will be enforced to be from 1 to 255 seconds. When 
// rogered up, the updated time will be placed into the correct chunk, to be
// later iterated. 
void Heartbeat::rogerUp(uint8_t chunkID, uint8_t resetSec) {

    Threads::MutexLock guard(this->mtx);
    if (!guard.LOCK()) {
        return; // Blocks use 
    }

    if (this->checkSus(chunkID)) return; // Blocks if suspended.

    if (resetSec == 0) resetSec = 1;
    this->reg[chunkID] = resetSec; // Set the heartbeat to its limit.
}

// Requires referene to the station details. Some of this information will be
// included when pinging the UDP server, to communicate its status. Run at 1 Hz,
// the receiving server will keep devices alive as long as they have checked in
// the past 5 seconds.
void Heartbeat::pingServer(Comms::STAdetails &details) {

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP); // Create socket.

    if (sock < 0) return;

    // Prep JSON message to send UDP server with details about the current
    // connection status.
    char msg[128] = {0};
    snprintf(msg, sizeof(msg), 
        "{\"mdns\": \"%s\", \"rssi\": \"%s\", \"mem\": \"%s\"}", 
        details.mdns, details.signalStrength, details.heap);  

    struct sockaddr_in dest; 
    dest.sin_family = AF_INET;
    dest.sin_port = htons(HEARTBEAT_UDP_PORT);
    dest.sin_addr.s_addr = inet_addr(UDP_URL);

    sendto(sock, msg, strlen(msg), 0, (struct sockaddr *)&dest, sizeof(dest));

    close(sock);
}

// Iterates the register array. Checks if the value is equal to 0, if so, it 
// will not decrement but alert. If > 0, it will decrement until it reaches 0.
// This will only send an alert if the calling function does not roger up when
// it says it will. WARNING: In order to function properly, this should be 
// called at exactly 1hz. 
void Heartbeat::manage() {

    Threads::MutexLock guard(this->mtx);
    if (!guard.LOCK()) {
        return; // Blocks use 
    }
   
    // If the blockNum is 0, this means there have been no registrations at the
    // time. Checks both that and if all has been suspended. Block if either
    // are true.
    if (this->blockNum == 0 || this->allSus) return; 

    // Iterate each active heartbeat client. Will only occur if there are 
    // registered clients and all heartbeats have not been suspended.
    for (int i = 0; i < this->blockNum; i++) {

        if (this->checkSus(i)) continue;

        // Will not drop below zero, when decremented, once zero is reached
        // locks in here.
        if (this->reg[i] == 0) {
            this->alert(i);
        } else {
            this->reg[i]--; // Decrement by 1.
            this->failures[i] = 0; // set to 0 based on success.
        }
    }
}

// Requires chunk/block ID and reason for suspension to be logged. Flags the 
// suspension registry, prevent normal heartbeat behavior.
void Heartbeat::suspend(uint8_t chunkID, const char* reason) {

    Threads::MutexLock guard(this->mtx);
    if (!guard.LOCK()) {
        return; // Blocks use 
    }

    this->suspensionReg |= (1 << chunkID);

    const char* _reason = strlen(reason) > HEARTBEAT_MAX_REASON ? 
        HEARTBEAT_DEFAULT_REASON : reason;
    
    snprintf(this->log, sizeof(this->log), "%s ID %u (%s) suspended for %s", 
        this->tag, chunkID, this->callers[chunkID], _reason);

    Messaging::MsgLogHandler::get()->handle(Messaging::Levels::INFO,
        this->log, Messaging::Method::SRL_LOG);
}

// Requires the reason for suspension to be logged. Sets the suspended flag
// to true, the ultimate flag, and overrides individual flag checks.
void Heartbeat::suspendAll(const char* reason) {

    Threads::MutexLock guard(this->mtx);
    if (!guard.LOCK()) {
        return; // Blocks use 
    }

    this->allSus = true;

    const char* _reason = strlen(reason) > HEARTBEAT_MAX_REASON ? 
        HEARTBEAT_DEFAULT_REASON : reason;
    
    snprintf(this->log, sizeof(this->log), "%s (ALL) suspended for %s", 
        this->tag, _reason);

    Messaging::MsgLogHandler::get()->handle(Messaging::Levels::INFO,
        this->log, Messaging::Method::SRL_LOG);
}

// Requires chunk/block ID. Unflags the suspension registry, allowing normal
// heartbeat behavior.
void Heartbeat::release(uint8_t chunkID) {

    Threads::MutexLock guard(this->mtx);
    if (!guard.LOCK()) {
        return; // Blocks use 
    }

    // Upon release, add extension to heartbeat due to sync up errors between
    // the threads rogering up, and the routine manager function due to 
    // differences in the thread execution times.
    this->reg[chunkID] += HEARTBEAT_REL_EXT;

    this->suspensionReg &= ~(1 << chunkID); // Release register.
    
    snprintf(this->log, sizeof(this->log), "%s ID %u (%s) released", 
        this->tag, chunkID, this->callers[chunkID]);

    Messaging::MsgLogHandler::get()->handle(Messaging::Levels::INFO,
        this->log, Messaging::Method::SRL_LOG);
}

// Requires no params. Removes suspended flag by setting to false.
void Heartbeat::releaseAll() {

    Threads::MutexLock guard(this->mtx);
    if (!guard.LOCK()) {
        return; // Blocks use 
    }

    // Upon release, add extension to heartbeat due to sync up errors between
    // the threads rogering up, and the routine manager function due to 
    // differences in the thread execution times. Iterate each register to add
    // the extension to.
    for (int i = 0; i < this->blockNum; i++) {
        this->reg[i] += HEARTBEAT_REL_EXT;
    }

    this->allSus = false; // Unsuspend.

    snprintf(this->log, sizeof(this->log), "%s (ALL) released", this->tag);

    Messaging::MsgLogHandler::get()->handle(Messaging::Levels::INFO,
        this->log, Messaging::Method::SRL_LOG);
}

}