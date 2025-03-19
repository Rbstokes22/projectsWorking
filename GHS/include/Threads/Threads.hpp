#ifndef THREADS_HPP
#define THREADS_HPP

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "UI/MsgLogHandler.hpp"

namespace Threads {

#define THREAD_LOG_METHOD Messaging::Method::SRL_LOG

class Thread {
    private:
    TaskHandle_t taskHandle;
    const char* tag;
    char log[LOG_MAX_ENTRY];

    public:
    Thread(const char* tag);
    bool initThread(
        void (*taskFunc)(void*), // task function
        uint16_t stackSize, // stack size in words. 4 bytes = word in 32-bit
        void* parameters, // task input parameters
        UBaseType_t priority); // priority of task
    bool suspendTask();
    bool resumeTask();
    ~Thread();
};

}

#endif // THREADS_HPP