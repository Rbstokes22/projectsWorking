#include "Threads/Threads.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "UI/MsgLogHandler.hpp"
#include "string.h"

namespace Threads {

Thread::Thread(Messaging::MsgLogHandler &msglogerr, const char* name) :

    taskHandle{NULL}, msglogerr(msglogerr), name(name) {}

void Thread::initThread(
    void (*taskFunc)(void*), 
    uint16_t stackSize, 
    void* parameters, 
    UBaseType_t priority) {

    BaseType_t taskCreate = xTaskCreate(
        taskFunc, this->name, stackSize, parameters, 
        priority, &this->taskHandle
    );

    if (taskCreate == pdPASS) {
        printf("Task Handle %s Created @ %p\n", this->name, this->taskHandle);
    }
}

void Thread::suspendTask() { 
    char msg[30]{0};
    sprintf(msg, "%s suspended", this->name);

    if (this->taskHandle != NULL) {
        vTaskSuspend(this->taskHandle);
        msglogerr.handle(
        Messaging::Levels::INFO, 
        msg, 
        Messaging::Method::SRL
        );
    } 
}

void Thread::resumeTask() {
    char msg[30]{0};
    sprintf(msg, "%s resumed", this->name);
    
    if (this->taskHandle != NULL) {
        vTaskResume(this->taskHandle);
        msglogerr.handle(
        Messaging::Levels::INFO, 
        msg, 
        Messaging::Method::SRL
        );
    }
}

Thread::~Thread() {
    if (this->taskHandle != NULL) {
        vTaskDelete(this->taskHandle);
        printf("Task Handle %s Deleted @ %p\n", this->name, this->taskHandle);
    }
}

}