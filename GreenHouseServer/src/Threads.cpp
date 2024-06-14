#include "Threads.h"

namespace Threads {

SensorThread::SensorThread(Devices::SensorObjects &sensorObjects) : 
    taskHandle{NULL}, 
    sensorObjects{sensorObjects},
    isThreadSuspended{false} {}

void SensorThread::setupThread() {

    xTaskCreate(
        this->sensorTask, // function to be tasked by freeRTOS
        "Sensor Task", // Name used for debugging
        2048, // allocated stack size
        &this->sensorObjects, // struct with all parameters
        1, // priority
        &this->taskHandle // pointer to the task handle
    );
}

void SensorThread::sensorTask(void* parameter) {
    Devices::SensorObjects* sensorObjects = (Devices::SensorObjects*) parameter;
    Clock::Timer checkSensors = sensorObjects->checkSensors;
    
    // Clock::Timer* checkSensors = (Clock::Timer*) parameter;

    // Devices::Light light{Photoresistor_PIN};
    while (true) {
        if (checkSensors.isReady()) {
            handleSensors(
                sensorObjects->tempHum,
                sensorObjects->light
            );
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // delays for 100 ms to prevent constant run
    }
}

void SensorThread::suspendTask() { // Called by OTA updates
    Serial.println("Task suspended");
    vTaskSuspend(taskHandle);
}

void SensorThread::resumeTask() {
    Serial.println("Task resumed");
    vTaskResume(taskHandle);
}

}