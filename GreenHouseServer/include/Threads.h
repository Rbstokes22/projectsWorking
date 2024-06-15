#ifndef THREADS_H
#define THREADS_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "Peripherals.h"
#include "Timing.h"

namespace Threads {

struct ThreadData {
    Devices::Sensors &sensor;
    Clock::Timer &sampleInterval;

    ThreadData(Devices::Sensors &sensor, Clock::Timer &sampleInterval);
};

class SensorThread {
    private:
    TaskHandle_t taskHandle;
    
    public:
    SensorThread();
    void initThread(ThreadData &data);
    static void sensorTask(void* parameter);
    void suspendTask();
    void resumeTask();
};

}

#endif // THREADS_H