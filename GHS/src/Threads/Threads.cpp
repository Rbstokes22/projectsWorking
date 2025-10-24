#include "Threads/Threads.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "UI/MsgLogHandler.hpp"
#include "string.h"

namespace Threads {

Thread::Thread(const char* tag) :

    taskHandle{NULL}, tag(tag) {}

// Requires the task function, the stack size in bytes, the params being passed
// to the fucntion, priority, stack buffer, Task Control Block, and the core to
// in the task to with 0 being for PRO and 1 being for APP. Returns true if 
// properly init.
bool Thread::initThread(void (*taskFunc)(void*), uint32_t stackSize, 
    void* parameters, UBaseType_t priority, StackType_t* stackBuffer,
    StaticTask_t &TCB, uint8_t taskCore) {

    // Before creating task, pre-color ot 0xAA if manual highwater marks are
    // required for future troubleshooting.
    memset(stackBuffer, 0xAA, stackSize); // in bytes

    // Create static task to allow stack allocated task stack, vs heap alloc.
    this->taskHandle = xTaskCreateStaticPinnedToCore(taskFunc, this->tag, 
        stackSize, parameters, priority, stackBuffer, &TCB, 
        static_cast<BaseType_t>(taskCore));

    // Uncomment if you want to not pin to core.
    // this->taskHandle = xTaskCreateStatic(taskFunc, this->tag, stackSize,
    //     parameters, priority, stackBuffer, &TCB);

    if (this->taskHandle != NULL) { // Success

        snprintf(this->log, sizeof(this->log), 
            "Task handle (%s) created @ addr: %p", this->tag, this->taskHandle);

        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::INFO,
            this->log, THREAD_LOG_METHOD);

        return true;

    } else { // Failure.

        snprintf(this->log, sizeof(this->log), 
            "Task handle (%s) not created", this->tag);

        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::CRITICAL,
            this->log, THREAD_LOG_METHOD);

        this->taskHandle = NULL; // Ensure null for future error handling.
        return false;
    }
}

// Requires no parameters. If thread was properly init, this will suspend
// the thread. If successful, returns true, if not, returns false.
bool Thread::suspendTask() { 
   
    if (this->taskHandle != NULL) {

        vTaskSuspend(this->taskHandle);
        
        snprintf(this->log, sizeof(this->log), "%s @ addr %p suspended", 
            this->tag, this->taskHandle);

        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::INFO, 
            this->log, Messaging::Method::SRL_LOG);

        return true;
    }

    return false;
}

// Requires no params. If thread was properly init, and suspended, it will
// be resumed. If successful, return true, false if not.
bool Thread::resumeTask() {

    if (this->taskHandle != NULL) {

        vTaskResume(this->taskHandle);
        
        snprintf(this->log, sizeof(this->log), "%s @ addr: %p resumed", 
            this->tag, this->taskHandle);

        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::INFO, 
            this->log, Messaging::Method::SRL_LOG);

        return true;
    }

    return false;
}

Thread::~Thread() {
    if (this->taskHandle != NULL) {
        vTaskDelete(this->taskHandle);
        snprintf(this->log,sizeof(this->log), 
            "Task Handle (%s) Deleted @ addr: %p\n", this->tag, 
            this->taskHandle);

        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::INFO, 
            this->log, Messaging::Method::SRL_LOG);
    }
}

}