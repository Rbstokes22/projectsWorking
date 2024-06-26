#include "Threads/Mutex.h"
#include <Arduino.h>

namespace Threads {

Mutex::Mutex(Messaging::MsgLogHandler &msglogerr) : 

    xMutex{xSemaphoreCreateMutex()}, msglogerr(msglogerr) {
    if (this->xMutex == NULL) {
        msglogerr.handle(
            Levels::CRITICAL,
            "Thread Mutex not created",
            Method::SRL, Method::OLED
        );
    } else {
        msglogerr.handle(Levels::INFO, "Thread Mutex created", Method::SRL);
    }
}

void Mutex::lock() {
    while (xSemaphoreTake(this->xMutex, portMAX_DELAY) != pdTRUE) {}
}

void Mutex::unlock() {
    xSemaphoreGive(this->xMutex);
}

Mutex::~Mutex() {vSemaphoreDelete(this->xMutex);}

}