#include "Threads/Threads.h"

namespace Threads {

// STATIC SETTINGS
const uint8_t SensorThread::totalSensors{Peripheral::PeripheralQty}; // CHANGE TO ENUM VALUES 

// Single thread creation for this application.
SensorThread::SensorThread(Messaging::MsgLogHandler &msglogerr) : 

    taskHandle{NULL}, msglogerr(msglogerr) {}

// Passes all thread setting compilations which include all sensors and 
// their corresponding clock object. This will create the thread task.
void SensorThread::initThread(Peripheral::Sensors** allSensors) {

    xTaskCreate(
        SensorThread::sensorTask, // function to be tasked by freeRTOS
        "Sensor Task", // Name used for debugging
        2048, // allocated stack size, should be plenty
        allSensors, // all thread handlers and sample intervals
        1, // priority
        &this->taskHandle // pointer to the task handle
    );
}

void SensorThread::mutexWrap(Peripheral::Sensors* sensor) {
    sensor->lock();
    sensor->handleSensors();
    sensor->unlock();
}

// This thread task takes in the parameter which is all of the thread 
// settings from the compilation. This will allow the sensors handle
// sensor method to be called at whichever interval the clock object 
// is set to. Each handle sensor will be locked with the mutex located
// within its class to avoid corrupt data with the shared objects.
void SensorThread::sensorTask(void* parameter) {
    Peripheral::Sensors** allSensors{static_cast<Peripheral::Sensors**>(parameter)};

    auto printFreeStack = [](){
        UBaseType_t uxHighWaterMark =
        uxTaskGetStackHighWaterMark(NULL);
        printf("Minimum stack free space: %lu words\n", uxHighWaterMark);
    };

    while (true) { 
        for (size_t i = 0; i < SensorThread::totalSensors; i++) {
            if(allSensors[i]->checkIfReady()) {
                SensorThread::mutexWrap(allSensors[i]);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(100)); // delays for 100 ms to prevent constant run
    }
}

void SensorThread::suspendTask() { // Called by OTA updates
    msglogerr.handle(
        Messaging::Levels::INFO, 
        "Thread suspended", 
        Messaging::Method::SRL);
    vTaskSuspend(taskHandle);
}

void SensorThread::resumeTask() {
    msglogerr.handle(
        Messaging::Levels::INFO, 
        "Thread resumed", 
        Messaging::Method::SRL);
    vTaskResume(taskHandle);
}

}