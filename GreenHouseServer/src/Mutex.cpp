#include "Mutex.h"
#include <Arduino.h>

namespace Threads {

Mutex::Mutex() : xMutex{xSemaphoreCreateMutex()}{
    if (this->xMutex == NULL) {
        Serial.println("MUTEX NOT CREATED");
    } else {Serial.println("MUTEX CREATED");}
}

void Mutex::lock() {
    while (xSemaphoreTake(this->xMutex, portMAX_DELAY) != pdTRUE) {}
}

void Mutex::unlock() {
    xSemaphoreGive(this->xMutex);
}

Mutex::~Mutex() {vSemaphoreDelete(this->xMutex);}

}