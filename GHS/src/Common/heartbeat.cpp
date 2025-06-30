#include "Common/heartbeat.hpp"
#include "UI/MsgLogHandler.hpp"
#include "string.h"
#include "Peripherals/saveSettings.hpp"

namespace heartbeat {

Threads::Mutex Heartbeat::mtx("HeartBeat");
uint8_t Heartbeat::blockNum = 0;

Heartbeat::Heartbeat(const char* tag) : tag(tag) {

    snprintf(this->log, sizeof(this->log), "%s ob created", this->tag);
    Messaging::MsgLogHandler::get()->handle(Messaging::Levels::INFO,
        this->log, Messaging::Method::SRL_LOG);
}

// Requires ID. Will not show caller, just the ID. Use the log to check the
// caller name assigned to the ID. Currently logs the unresponsiveness and 
// restarts if unresponsive.
void Heartbeat::alert(uint8_t ID) {
    snprintf(this->log, sizeof(this->log), "%s ID %u unresponsive", 
        this->tag, ID);

    // Log tail entry, which will be visible upon a save and restart.
    Messaging::MsgLogHandler::get()->handle(Messaging::Levels::CRITICAL,
        this->log, Messaging::Method::SRL_LOG);

    NVS::settingSaver::get()->saveAndRestart(); // Restart after log.
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
// uint8_t array that is assigned to the caller.
uint8_t Heartbeat::getBlockID(const char* caller, uint8_t initVal) {
    if (blockNum < HEARTBEAT_CLIENTS_MAX) {

        snprintf(this->log, sizeof(this->log), 
            "%s block ID %u registered to %s", this->tag, this->blockNum,
            caller);

        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::INFO,
         this->log, Messaging::Method::SRL_LOG);

        this->rogerUp(blockNum, initVal); // Sets the init val.
        
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
        }
    }
}



}