#include "Common/heartbeat.hpp"
#include "UI/MsgLogHandler.hpp"
#include "string.h"
#include "Peripherals/saveSettings.hpp"

namespace heartbeat {

Threads::Mutex Heartbeat::mtx("HeartBeat");
uint8_t Heartbeat::blockNum = 0; // Init to 0.

Heartbeat::Heartbeat(const char* tag) : tag(tag) {

    // zero out all tracking arrays.
    memset(this->reg, 0, sizeof(this->reg));
    memset(this->failures, 0, sizeof(this->failures));
    memset(this->extensions, 0, sizeof(this->extensions));
    memset(this->callers, 0, sizeof(this->callers));

    snprintf(this->log, sizeof(this->log), "%s ob created", this->tag);
    Messaging::MsgLogHandler::get()->handle(Messaging::Levels::INFO,
        this->log, Messaging::Method::SRL_LOG);
}

// Requires ID. Will not show caller, just the ID. Use the log to 
// check the caller name assigned to the ID. Currently logs the 
// unresponsiveness and restarts if unresponsive and directed, and grace period
// is unmet.
IRAM_ATTR void Heartbeat::alert(uint8_t ID) {
    snprintf(this->log, sizeof(this->log), "%s ID %u Caller %s unresponsive", 
        this->tag, ID, this->callers[ID]);

    // Log tail entry, which will be visible upon a save and restart.
    Messaging::MsgLogHandler::get()->handle(Messaging::Levels::CRITICAL,
        this->log, Messaging::Method::SRL_LOG);

    this->failures[ID]++; // Increment upon a failure

    // Extend the heartbeat to allow recovery time before immediately 
    // restarting due to a fail since the heartbeat will be at 0 to trigger 
    // this alert.
    this->extend(ID, HEARTBEAT_EXT);

    // Restart with consecutive failures. Unless consecutive, will be reset to
    // 0 upon successful heartbeat check.
    if (this->failures[ID] >= HEARTBEAT_RESET_FAILS) {
        NVS::settingSaver::get()->saveAndRestart(); // Restart after log.
    }
}

// Singleton class, returns instance pointer of this class.
IRAM_ATTR Heartbeat* Heartbeat::get() {

    // Single use of mutex lock which will ensure to protect any subsequent
    // calss made after requesting this instance.
    Threads::MutexLock(Heartbeat::mtx); 

    static Heartbeat instance(HEARTBEAT_TAG);
    return &instance;
}

// Requires the caller ID and the init value. The init value will set the array
// block to this value during the init process due to delays in order to 
// prevent unwanted behavior by the alert funciton. Returns the position in the
// uint8_t array that is assigned to the caller. WARNING: Caller must be 15
// chars or less, with the 16th char being the null terminator. Caller will be
// truncated if in violation.
IRAM_ATTR uint8_t Heartbeat::getBlockID(const char* caller, uint8_t initVal) {
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
// later iterated. ATTENTION. Uses IRAM to ensure that critical check-ins occur
// despite flash being locked wifi-if it has issues. Previous issues with 
// heartbeat failure from both routine and net tasks, affected by wifi.
IRAM_ATTR void Heartbeat::rogerUp(uint8_t chunkID, uint8_t resetSec) {
    if (resetSec == 0) resetSec = 1;

    // Add the reset seconds, plus any extensions to that time. This is because
    // if an extension is added, like a reset time of 2s + a 4s extension
    // for a total of 6 seconds, it can be over written by the next roger up.
    // Once the extension is removed, after a blocking call, it will normalize.
    this->reg[chunkID] = resetSec + this->extensions[chunkID]; 
}

// Iterates the register array. Checks if the value is equal to 0, if so, it 
// will not decrement but alert. If > 0, it will decrement until it reaches 0.
// This will only send an alert if the calling function does not roger up when
// it says it will. WARNING: In order to function properly, this should be 
// called at exactly 1hz. Uses IRAM to ensure managability.
IRAM_ATTR void Heartbeat::manage() {

    if (this->blockNum == 0) return; // Block, nothing has been assigned.

    for (int i = 0; i < this->blockNum; i++) {

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

// Requires the chunk/block ID number, and seconds to extend. Add the extension
// to the extension array, which will be added during each subsequent roger up.
// Will mimic a roger up in this function in the event roger up cannot be 
// performed during the block. Once the blocking function is complete, the 
// extension should be removed. This is to ensure that blocking functions do 
// not cause a heartbeat issue.
IRAM_ATTR void Heartbeat::extend(uint8_t chunkID, uint8_t seconds) {

    this->extensions[chunkID] = seconds; // Add ext sec to array.

    // mimic the roger up by increasing the seconds instantly, in the event
    // a roger up cannot be performed.
    this->reg[chunkID] += seconds;
}

// Requires extension seconds only. Extends all running tasks by the seconds
// passed. ATTENTION. Developed primarily for blocking calls from the alerts 
// and OTA check if the WEBURL is offline. Testing has shown heartbeat resets
// with unresolved URLs with times > 7 seconds.
IRAM_ATTR void Heartbeat::extendAll(uint8_t seconds) {

    // Iterates all block ID's in the registry and extends their time in sec.
    for (int i = 0; i < this->blockNum; i++) {
        this->extend(i, seconds);
    }
}

IRAM_ATTR void Heartbeat::clearExt(uint8_t chunkID) {
    this->extensions[chunkID] = 0; // reset extension to 0.
}

IRAM_ATTR void Heartbeat::clearExtAll() {

    // Iterate all block ID's to clear the extensions
    for (int i = 0; i < this->blockNum; i++) {
        this->clearExt(i);
    }
}

}