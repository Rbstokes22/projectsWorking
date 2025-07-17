#include "Threads/Mutex.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "UI/MsgLogHandler.hpp"
#include <atomic>

namespace Threads {

Mutex::Mutex(const char* tag) : 

    tag(tag), xMutex{xSemaphoreCreateMutex()}, isLocked(false), init(true) {

    if (this->xMutex == NULL) { // Handles uncreated Mutex.
        snprintf(this->log, sizeof(this->log), "%s MTX not created", this->tag);
        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::CRITICAL,
            this->log, Messaging::Method::SRL_LOG);

    } else {
        snprintf(this->log, sizeof(this->log), "%s MTX created", this->tag);
        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::INFO,
            this->log, Messaging::Method::SRL_LOG);
    }
}

// Requires no parameters. Attemps to take a lock, only delaying acquisition
// by the set LOCK_DELAY. If acquired, locks and returns true. If not, 
// returns false.
bool Mutex::lock() {
    if (!init) return false; // Prevents mutex noise before mutex is init.

    static uint8_t count = 0; // Used for retries.

    // Attempts to lock the mutex and delays for LOCK_DELAY amount of millis
    // allowing non-permanent-blocking code. Once acquired, changes the 
    // isLocked bool to true.
    if (xSemaphoreTake(this->xMutex, pdMS_TO_TICKS(LOCK_DELAY)) == pdTRUE) {
        this->isLocked.store(true); // Update this for lock release below.
        count = 0; // Keep at 0.
        return true;

    } else {

        // Check if count is under the repeat qty. If yes, log and attempt a
        // repeat.
        if (count < MTX_REPEAT) {
            snprintf(this->log, sizeof(this->log), 
                "%s MTX not locked. Retry # %u", this->tag, count);
            count++; // Increment by 1 to reach max attempts.
            this->lock(); // Run again until locked or max attempts reached.

        } else {
            snprintf(this->log, sizeof(this->log), 
                "%s MTX not locked after %u attempts", this->tag, MTX_REPEAT);

            count = 0; // Reset to 0 after n amount of attempts.
        }

        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::CRITICAL,
            this->log, Messaging::Method::SRL_LOG);

        return false;
    }
}

// Requires no params. Checks first to see if already unlock, will return true
// if unlocked. If locked, attempts to release lock, returning true if 
// successful and false if not.
bool Mutex::unlock() {
    if (!init) return false; // Prevents mutex noise before init.

    static uint8_t count = 0; // Used for retries.

    // Will check to see if unlocked, if unlocked, returns true, if locked,
    // proceeds to unlock and returns true once done.
    if (!this->isLocked.load()) return true; 

    if (xSemaphoreGive(this->xMutex) == pdTRUE) {
        this->isLocked.store(false);
        count = 0; // Keep at 0 if good.
        return true;

    } else {

        if (count < MTX_REPEAT) {
            snprintf(this->log, sizeof(this->log), 
                "%s MTX lock not released. Retry # %u", this->tag, count);
            count++; // Inc by 1 to reach max attempts.

            // NOTE: Recursion should not cause any heartbeat issues.
            this->unlock(); // Attempt unlock again until max attempts reached.

        } else {
            snprintf(this->log, sizeof(this->log), 
                "%s MTX lock not released after %u attempts", this->tag, 
                MTX_REPEAT);

            count = 0; // Reset counts if max attempts reached.
        }

        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::CRITICAL,
            this->log, Messaging::Method::SRL_LOG);
            
        return false;
    }
}

// Deletes the semaphore handle upon destrution.
Mutex::~Mutex() {vSemaphoreDelete(this->xMutex);}

// Since CPP doesn't have a finally block, this is a scope guard or
// simple Resource Acquisition Is Initialization (RAII) pattern to 
// ensure the mutex is always unlocked when it goes out of scope.
MutexLock::MutexLock(Mutex &mtx) : mtx(mtx) {
    this->mtx.lock();
}

// Automatically releases the mutex lock.
MutexLock::~MutexLock() {
    this->mtx.unlock();
}

}