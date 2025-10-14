#include "Common/heartbeat.hpp"
#include "UI/MsgLogHandler.hpp"
#include "string.h"
#include "Peripherals/saveSettings.hpp"

namespace heartbeat {

Threads::Mutex Heartbeat::mtx("HeartBeat");
uint8_t Heartbeat::blockNum = 0;

Heartbeat::Heartbeat(const char* tag) : tag(tag) {

    memset(this->callers, 0, sizeof(this->callers));

    snprintf(this->log, sizeof(this->log), "%s ob created", this->tag);
    Messaging::MsgLogHandler::get()->handle(Messaging::Levels::INFO,
        this->log, Messaging::Method::SRL_LOG);
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

    this->failures[ID]++; // Increment upon a failure

    // Restart with consecutive failures. Unless consecutive, will be reset to
    // 0 upon successful heartbeat check.
    if (this->failures[ID] >= HEARTBEAT_RESET_FAILS) {
        NVS::settingSaver::get()->saveAndRestart(); // Restart after log.
    }
    
}

// Singleton class, returns instance pointer of this class.
Heartbeat* Heartbeat::get() {

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
uint8_t Heartbeat::getBlockID(const char* caller, uint8_t initVal) {
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
    this->reg[chunkID] = resetSec; 
}

// Iterates the register array. Checks if the value is equal to 0, if so, it 
// will not decrement but alert. If > 0, it will decrement until it reaches 0.
// This will only send an alert if the calling function does not roger up when
// it says it will. WARNING: In order to function properly, this should be 
// called at exactly 1hz.
void Heartbeat::manage() {

    if (this->blockNum == 0) return; // Block, nothing has been assigned.

    for (int i = 0; i < this->blockNum; i++) {

        if (this->reg[i] == 0) {
            this->alert(i);
        } else {
            this->reg[i]--; // Decrement by 1.
            this->failures[i] = 0; // set to 0 based on success.
        }
    }
}

}