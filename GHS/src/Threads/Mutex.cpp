#include "Threads/Mutex.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "UI/MsgLogHandler.hpp"
#include <atomic>

namespace Threads {

// A mutex will be created for each peripheral to prevent its data from access
// during a read write. This is due to each peripheral from being on a 
// separate thread.
Mutex::Mutex(Messaging::MsgLogHandler &msglogerr) : 

    xMutex{xSemaphoreCreateMutex()}, msglogerr(msglogerr), isLocked(false) {

    if (this->xMutex == NULL) { // Handles uncreated Mutex.
        msglogerr.handle(
            Messaging::Levels::CRITICAL,
            "Thread Mutex not created",
            Messaging::Method::SRL, Messaging::Method::OLED
        );
    } else {
        msglogerr.handle(
            Messaging::Levels::INFO, 
            "Thread Mutex created", 
            Messaging::Method::SRL);
    }
}

// Requires no parameters. Checks if there is a lock on the mutex, if yes,
// returns false preventing attempt to lock, and returns true upon success.
bool Mutex::lock() {

    // Attempts to lock the mutex and delays for LOCK_DELAY amount of millis
    // allowing non-permanent-blocking code. Once acquired, changes the 
    // isLocked bool to true.
    if (xSemaphoreTake(this->xMutex, pdMS_TO_TICKS(LOCK_DELAY)) == pdTRUE) {
        this->isLocked.store(true);
        return true;
    } else {
        printf("Unable to acquire MUTEX lock\n");
        return false;
    }
}

// Requires no parameters. Checks if there is no lock on the muted, if true,
// returns false before attempting to unlock, and returns true upon success.
bool Mutex::unlock() {

    // Will check to see if unlocked, if unlocked, returns true, if locked,
    // proceeds to unlock and returns true once done.
    if (!this->isLocked.load()) return true; 

    if (xSemaphoreGive(this->xMutex) == pdTRUE) {
        this->isLocked.store(false);
        return true;
    } else {
        printf("Unable to release MUTEX lock\n");
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