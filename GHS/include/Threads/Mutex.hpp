#ifndef MUTEX_HPP
#define MUTEX_HPP

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "UI/MsgLogHandler.hpp"

// Mutex created for the share peripheral classes. The peripheral devices
// are all ran on a thread. A good analogy to a semphore is that it acts 
// like a traffic light alotting a certain bridge capacity. When clear,
// it signals, when not, it signals.

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

#endif // MUTEX_HPP