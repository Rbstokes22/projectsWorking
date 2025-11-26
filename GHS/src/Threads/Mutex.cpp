#include "Threads/Mutex.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "UI/MsgLogHandler.hpp"
#include <atomic>

namespace Threads {

// ATTENTION. Removed all logging from lock and unlock functions due to 
// recursion affect stack frame and haste.

// ATTENTION. Use recursive semaphore to prevent issues with lock and unlock 
// task ownership. If manual implementation is desired, use xSemaphoreTake and
// give, but track ownership by emplacing a successful semaphore take with
// this->owner = xTaskGetCurrentTaskHandle(), and before giving the mutex
// back ensure that owner == xTaskGetCurrentTaskHandle, and if so, release
// mutex and set ownder to nullptr.

Mutex::Mutex(const char* tag) : 

    tag(tag), xMutex{xSemaphoreCreateRecursiveMutex()}, init(true) {

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
// by the set LOCK_TIMEOUT. If acquired, locks and returns true. If not, 
// returns false.
bool Mutex::lock() {
    if (!init) return false; // Prevents mutex noise before mutex is init.

    // Attempt to take the semaphore recursively.
    BaseType_t semph = xSemaphoreTakeRecursive(this->xMutex, 
        pdMS_TO_TICKS(LOCK_TIMEOUT));

    // Attempts to lock the mutex and delays for LOCK_TIMEOUT amount of millis
    // allowing non-permanent-blocking code. 
    if (semph == pdTRUE) {return true;}
    else {
        // printf("%s LOCK ERR\n", caller);
        // Attempt repeats to try to lock.
        for (uint8_t attempt = 0; attempt < MTX_REPEAT; ++attempt) {

            semph = xSemaphoreTakeRecursive(this->xMutex, 
                pdMS_TO_TICKS(LOCK_TIMEOUT));

            if (semph == pdTRUE) {return true;}
        }

        snprintf(this->log, sizeof(this->log),
            "%s Lock release timeout after %d retries", this->tag, MTX_REPEAT);

        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::WARNING,
                this->log, Messaging::Method::SRL_LOG);

        return false;
    }
}

// Requires no params. Checks first to see if already unlock, will return true
// if unlocked. If locked, attempts to release lock, returning true if 
// successful and false if not.
bool Mutex::unlock() {
    if (!init) return false; // Prevents mutex noise before init.

    // Attempt to give semaphore recursively.
    BaseType_t semph = xSemaphoreGiveRecursive(this->xMutex);

    if (semph == pdTRUE) {return true;} 
    else {

        // Attempt repeats to try to unlock.
        for (uint8_t attempt = 0; attempt < MTX_REPEAT; ++attempt) {

            semph = xSemaphoreGiveRecursive(this->xMutex);

            if (semph == pdTRUE) {return true;}
        }

        snprintf(this->log, sizeof(this->log),
            "%s Unlock release timeout after %d retries", this->tag, 
            MTX_REPEAT);

        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::WARNING,
                this->log, Messaging::Method::SRL_LOG);

        return false;
    }
}

// Deletes the semaphore handle upon destrution.
Mutex::~Mutex() {vSemaphoreDelete(this->xMutex);}

// Since CPP doesn't have a finally block, this is a scope guard or
// simple Resource Acquisition Is Initialization (RAII) pattern to 
// ensure the mutex is always unlocked when it goes out of scope.
MutexLock::MutexLock(Mutex &mtx) : mtx(mtx), isLocked(false) {}

bool MutexLock::LOCK() {
    if (this->isLocked) return true; // Already locked.

    if (this->mtx.lock()) {
        this->isLocked = true;
        return true;
    }

    return false;
}

bool MutexLock::UNLOCK() {
    if (!this->isLocked) return true; // Already unlocked.

    if (this->mtx.unlock()) {
        this->isLocked = false;
        return true;
    }

    return false;
}

// Automatically releases the mutex lock.
MutexLock::~MutexLock() {
    if (this->isLocked) {
        this->mtx.unlock();
        this->isLocked = false;
    }
}

}