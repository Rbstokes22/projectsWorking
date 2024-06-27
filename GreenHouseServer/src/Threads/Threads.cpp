#include "Threads/Threads.h"

namespace Threads {

// STATIC SETTINGS
const uint8_t SensorThread::totalSensors{Peripheral::PeripheralQty}; 

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

// Allows each peripheral to lock and uplock when handling its sensor data.
void SensorThread::mutexWrap(Peripheral::Sensors* sensor) {
    sensor->lock();
    sensor->handleSensors();
    sensor->unlock();
}

// The parameter is a pointer to an array of pointers that contain the address
// to each sensor. This is cast back to a sensor array of pointers.
void SensorThread::sensorTask(void* parameter) {
    Peripheral::Sensors** allSensors{static_cast<Peripheral::Sensors**>(parameter)};

    // use if needed to check stack size
    auto printFreeStack = [](){
        UBaseType_t uxHighWaterMark =
        uxTaskGetStackHighWaterMark(NULL);
        printf("Minimum stack free space: %lu words\n", uxHighWaterMark);
    };

    // While loop equivalent for the thread environment. 
    while (true) { 

        // Iterates through each passed sensor, and if ready, sends the 
        // sensor data to the mutex wrap to execute its handleSensors 
        // function.
        for (size_t i = 0; i < SensorThread::totalSensors; i++) {
            if(allSensors[i]->checkIfReady()) {
                SensorThread::mutexWrap(allSensors[i]);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(100)); // delays for 100 ms to prevent constant run
    }
}

// Suspends and resums the thread, called by OTA updates.
void SensorThread::suspendTask() { 
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