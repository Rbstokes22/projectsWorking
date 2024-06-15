#include "Threads.h"

namespace Threads {

ThreadData::ThreadData(Devices::Sensors &sensor, Clock::Timer &sampleInterval) : 
    sensor{sensor}, sampleInterval{sampleInterval}{}

SensorThread::SensorThread() : 
    taskHandle{NULL}{}

void SensorThread::initThread(ThreadData &data) {

    xTaskCreate(
        this->sensorTask, // function to be tasked by freeRTOS
        "Sensor Task", // Name used for debugging
        2048, // allocated stack size, should be plenty
        &data, // Timing object for samples
        1, // priority
        &this->taskHandle // pointer to the task handle
    );
}

void SensorThread::sensorTask(void* parameter) {
    ThreadData* data{(ThreadData*) parameter};

    auto printFreeStack = [](){
        UBaseType_t uxHighWaterMark =
        uxTaskGetStackHighWaterMark(NULL);
        printf("Minimum stack free space: %lu words\n", uxHighWaterMark);
    };
    
    while (true) {
        if (data->sampleInterval.isReady()) {
            data->sensor.handleSensors();
            printFreeStack();
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