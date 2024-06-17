#ifndef THREADS_H
#define THREADS_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "Peripherals.h"
#include "Timing.h"

namespace Threads {

// The Device::Sensors is an abstract class, and the thread will be used to call
// the handle sensors method only. This will also pass the clock associated 
// with that sensor, used for sampling interval.
struct ThreadSetting {
    Devices::Sensors &sensor;
    Clock::Timer &sampleInterval;

    ThreadSetting(Devices::Sensors &sensor, Clock::Timer &sampleInterval);
};

// A compilation of all of the thread settings for a single thread.
struct ThreadSettingCompilation {
    ThreadSetting &tempHum; ThreadSetting &light;
    // ThreadSetting &soil1; ThreadSetting &soil2;
    // ThreadSetting &soil3; ThreadSetting &soil4;

    ThreadSettingCompilation(
        ThreadSetting &tempHum, ThreadSetting &light
        // ThreadSetting &soil1, ThreadSetting &soil2, // Dont exist yet
        // ThreadSetting &soil3, ThreadSetting &soil4
    );
};

class SensorThread {
    private:
    TaskHandle_t taskHandle;
    
    public:
    SensorThread();
    void initThread(ThreadSettingCompilation &settings);
    static void sensorTask(void* parameter);
    void suspendTask();
    void resumeTask();
};

}

#endif // THREADS_H