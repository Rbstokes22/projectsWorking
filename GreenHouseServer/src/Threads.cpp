#include "Threads.h"

namespace Threads {

ThreadSetting::ThreadSetting(Devices::Sensors &sensor, Clock::Timer &sampleInterval) : 
    sensor{sensor}, sampleInterval{sampleInterval}{}

ThreadSettingCompilation::ThreadSettingCompilation (
    ThreadSetting &tempHum, ThreadSetting &light
        // ThreadSetting &soil1, ThreadSetting &soil2, // Dont exist yet
        // ThreadSetting &soil3, ThreadSetting &soil4
    ) : tempHum(tempHum), light(light){}

SensorThread::SensorThread() : 
    taskHandle{NULL}{}

void SensorThread::initThread(ThreadSettingCompilation &settings) {

    xTaskCreate(
        this->sensorTask, // function to be tasked by freeRTOS
        "Sensor Task", // Name used for debugging
        2048, // allocated stack size, should be plenty
        &settings, // all thread handlers and sample intervals
        1, // priority
        &this->taskHandle // pointer to the task handle
    );
}

void SensorThread::sensorTask(void* parameter) {
    ThreadSettingCompilation* settings{(ThreadSettingCompilation*) parameter};

    auto printFreeStack = [](){
        UBaseType_t uxHighWaterMark =
        uxTaskGetStackHighWaterMark(NULL);
        printf("Minimum stack free space: %lu words\n", uxHighWaterMark);
    };
    
    while (true) { // FIX FOR EACH SENSOR
        if (settings->tempHum.sampleInterval.isReady()) {
            settings->tempHum.sensor.handleSensors();
            printFreeStack(); // just use in here to see periodic usage
        }

        if (settings->light.sampleInterval.isReady()) {
            settings->light.sensor.handleSensors();
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