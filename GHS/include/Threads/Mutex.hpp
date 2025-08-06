#ifndef MUTEX_HPP
#define MUTEX_HPP

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <atomic>

// Mutex created for the share peripheral classes. The peripheral devices
// are all ran on a thread. A good analogy to a semphore is that it acts 
// like a traffic light alotting a certain bridge capacity. When clear,
// it signals, when not, it signals.

// Logging, do not include msglogerr here, due to it requiring the use of a
// mutex, it will error out due to cyclic dependency. It can be used by
// the source file for logging, but no tags can be used.

namespace Threads {

#define LOCK_DELAY 100 // Delay in millis to attempt to acquire lock
#define MTX_LOG_SIZE 128 
#define MTX_REPEAT 3 // Times to repeat attempting to lock or unlock mutex.

class Mutex {
    private:
    const char* tag;
    char log[MTX_LOG_SIZE];
    SemaphoreHandle_t xMutex;
    bool init; // Prevents noise from use before mutex is properly init.

    public:
    Mutex(const char* tag);
    bool lock();
    bool unlock();
    ~Mutex();
};

// Since CPP doesn't have a finally block, this is a scope guard or
// simple Resource Acquisition Is Initialization (RAII) pattern to 
// ensure the mutex is always unlocked when it goes out of scope.
class MutexLock {
    private:
    Mutex &mtx;

    public:
    MutexLock(Mutex &mtx);
    ~MutexLock();
};

}

#endif // MUTEX_HPP