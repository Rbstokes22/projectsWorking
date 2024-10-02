#ifndef THREADS_HPP
#define THREADS_HPP

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "UI/MsgLogHandler.hpp"

namespace Threads {

class Thread {
    private:
    TaskHandle_t taskHandle;
    Messaging::MsgLogHandler &msglogerr;
    const char* name;

    public:
    Thread(Messaging::MsgLogHandler &msglogerr, const char* name);
    void initThread(
        void (*taskFunc)(void*), // task function
        uint16_t stackSize, // stack size in words.
        void* parameters, // task input parameters
        UBaseType_t priority); // priority of task
    void suspendTask();
    void resumeTask();
    ~Thread();
};

}

#endif // THREADS_HPP