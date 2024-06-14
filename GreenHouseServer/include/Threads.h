#ifndef THREADS_H
#define THREADS_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "Peripherals.h"

namespace Threads {

class SensorThread {
    private:
    TaskHandle_t taskHandle;
    Devices::SensorObjects &sensorObjects;
    bool isThreadSuspended;
    
    public:
    SensorThread(Devices::SensorObjects &sensorObjects);
    void setupThread();
    static void sensorTask(void* parameter);
    void suspendTask();
    void resumeTask();
};

}

#endif // THREADS_H