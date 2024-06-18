#ifndef MUTEX_H
#define MUTEX_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

namespace Threads {

class Mutex {
    private:
    SemaphoreHandle_t xMutex;

    public:
    Mutex();
    void lock();
    void unlock();
    ~Mutex();
};

}

#endif // MUTEX_H