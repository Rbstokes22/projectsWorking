#include "Threads/Mutex.hpp"


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
    printf("mutex locked");
    while (xSemaphoreTake(this->xMutex, portMAX_DELAY) != pdTRUE) {}
}

void Mutex::unlock() {
    printf("mutex unlocked");
    xSemaphoreGive(this->xMutex);
}

Mutex::~Mutex() {vSemaphoreDelete(this->xMutex);}

}