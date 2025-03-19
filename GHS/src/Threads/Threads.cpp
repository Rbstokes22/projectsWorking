#include "Threads/Threads.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "UI/MsgLogHandler.hpp"
#include "string.h"

namespace Threads {

Thread::Thread(const char* tag) :

    taskHandle{NULL}, tag(tag) {}

// Requires the task function, the stacksize in words (4 bytes = 1 word 32-bit),
// the parameters being passed to the function, and the priority. Creates a
// thread and returns true if successful, and false if not.
bool Thread::initThread(void (*taskFunc)(void*), uint16_t stackSize, 
    void* parameters, UBaseType_t priority) {

    // Attempt to create a thread. Returns pdPASS if successful.
    BaseType_t taskCreate = xTaskCreate(taskFunc, this->tag, stackSize, 
        parameters, priority, &this->taskHandle);

    if (taskCreate == pdPASS) { // Success
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
        snprintf(this->log, sizeof(this->log), "%s suspended", this->tag);
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
        snprintf(this->log, sizeof(this->log), "%s resumed", this->tag);
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