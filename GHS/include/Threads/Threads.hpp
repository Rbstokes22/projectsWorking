#ifndef THREADS_HPP
#define THREADS_HPP

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "UI/MsgLogHandler.hpp"

namespace Threads {

#define THREAD_LOG_METHOD Messaging::Method::SRL_LOG

// ATTENTION: When choosing stack size, local variables and any function calls
// matter. Stack size is allocated to that specific task. So if that thread
// makes a function call outside of the task, that task stack sort of takes 
// ownership of the call as well as any nested function calls or a function 
// call chain. If you have threadA and threadB, and they both call, sendReport,
// which creates a 2048 byte buffer, that buffer is allocated to each thread.
// If you were to call sendReport, outside of a thread, by several functions,
// everything happens in order. Within a thread, there is concurrancy, which is
// why there is a requirement for its own stack space.

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