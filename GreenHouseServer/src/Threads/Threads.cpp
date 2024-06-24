#include "Threads/Threads.h"

namespace Threads {

// This is the sensor and its corresponding timing object. Relatively const
ThreadSetting::ThreadSetting(Peripheral::Sensors &sensor, Clock::Timer &sampleInterval) : 
    sensor{sensor}, sampleInterval{sampleInterval}{}

// This is a compilation of each peripheral device's Thread setting
// structures. Add or remove all changes in this struct.
ThreadSettingCompilation::ThreadSettingCompilation (
    ThreadSetting &tempHum, ThreadSetting &light,
    ThreadSetting &soil1
    ) : tempHum(tempHum), light(light), soil1(soil1){}

// Single thread creation for this application.
SensorThread::SensorThread(Messaging::MsgLogHandler &msglogerr) : 
    taskHandle{NULL}, msglogerr(msglogerr) {}

// Passes all thread setting compilations which include all sensors and 
// their corresponding clock object. This will create the thread task.
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

// This thread task takes in the parameter which is all of the thread 
// settings from the compilation. This will allow the sensors handle
// sensor method to be called at whichever interval the clock object 
// is set to. Each handle sensor will be locked with the mutex located
// within its class to avoid corrupt data with the shared objects.
void SensorThread::sensorTask(void* parameter) {
    ThreadSettingCompilation* settings{(ThreadSettingCompilation*) parameter};

    auto printFreeStack = [](){
        UBaseType_t uxHighWaterMark =
        uxTaskGetStackHighWaterMark(NULL);
        printf("Minimum stack free space: %lu words\n", uxHighWaterMark);
    };

    while (true) { 
        if (settings->tempHum.sampleInterval.isReady()) {
            settings->tempHum.sensor.lock(); // Mutex Lock
            settings->tempHum.sensor.handleSensors();
            settings->tempHum.sensor.unlock();
            // printFreeStack(); // just use in here to see periodic usage
        }

        if (settings->light.sampleInterval.isReady()) {
            settings->light.sensor.lock(); // Mutex Lock
            settings->light.sensor.handleSensors();
            settings->light.sensor.unlock();
        }

        if (settings->soil1.sampleInterval.isReady()) {
            settings->soil1.sensor.lock();
            settings->soil1.sensor.handleSensors();
            settings->soil1.sensor.unlock();
        }

        vTaskDelay(pdMS_TO_TICKS(100)); // delays for 100 ms to prevent constant run
    }
}

void SensorThread::suspendTask() { // Called by OTA updates
    msglogerr.handle(Levels::INFO, "Thread suspended", Method::SRL);
    vTaskSuspend(taskHandle);
}

void SensorThread::resumeTask() {
    msglogerr.handle(Levels::INFO, "Thread resumed", Method::SRL);
    vTaskResume(taskHandle);
}

}