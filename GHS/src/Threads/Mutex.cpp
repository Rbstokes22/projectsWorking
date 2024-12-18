#include "Threads/Mutex.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "UI/MsgLogHandler.hpp"

namespace Threads {

// A mutex will be created for each peripheral to prevent its data from access
// during a read write. This is due to each peripheral from being on a 
// separate thread.
Mutex::Mutex(Messaging::MsgLogHandler &msglogerr) : 

    xMutex{xSemaphoreCreateMutex()}, msglogerr(msglogerr) {

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

void Mutex::lock() {
    while (xSemaphoreTake(this->xMutex, portMAX_DELAY) != pdTRUE) {}
}

void Mutex::unlock() {
    xSemaphoreGive(this->xMutex);
}

Mutex::~Mutex() {vSemaphoreDelete(this->xMutex);}

// Since CPP doesn't have a finally block, this is a scope guard or
// simple Resource Acquisition Is Initialization (RAII) pattern to 
// ensure the mutex is always unlocked when it goes out of scope.
MutexLock::MutexLock(Mutex &mtx) : mtx(mtx) {
    mtx.lock();
}

MutexLock::~MutexLock() {
    mtx.unlock();
}

}