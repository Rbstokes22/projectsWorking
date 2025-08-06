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

// ATTENTION: Words are considered as 1 byte for some reason on this device.
// High water mark is still measured in words (4 bytes), but the initial alloc
// must be done with bytes.

class Thread {
    private:
    TaskHandle_t taskHandle;
    const char* tag;
    char log[LOG_MAX_ENTRY];

    public:
    Thread(const char* tag);
    bool initThread(
        void (*taskFunc)(void*), // task function
        uint32_t stackSize, // stack size in bytes, see ATTN above.
        void* parameters, // task input parameters
        UBaseType_t priority, // Priority of task
        StackType_t* stackBuffer, // Must be a static or global allocation
        StaticTask_t &TCB, // Task control block, must be static or global.
        uint8_t taskCore // Core to pin task to, 0 = PRO, 1 = APP.
    ); // priority of task
    bool suspendTask();
    bool resumeTask();
    ~Thread();
};

}

#endif // THREADS_HPP