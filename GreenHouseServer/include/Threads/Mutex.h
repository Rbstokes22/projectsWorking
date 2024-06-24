#ifndef MUTEX_H
#define MUTEX_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "UI/MsgLogHandler.h"

namespace Threads {

class Mutex {
    private:
    SemaphoreHandle_t xMutex;
    Messaging::MsgLogHandler &msglogerr;

    public:
    Mutex(Messaging::MsgLogHandler &msglogerr);
    void lock();
    void unlock();
    ~Mutex();
};

}

#endif // MUTEX_H