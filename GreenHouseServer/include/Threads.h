#ifndef THREADS_H
#define THREADS_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "MsgLogHandler.h"
#include "Peripherals/Light.h"
#include "Peripherals/TempHum.h"
#include "Peripherals/Soil.h"
#include "Timing.h"
 
namespace Threads {

// The Device::Sensors is an abstract class, and the thread will be used to call
// the handle sensors method only. This will also pass the clock associated 
// with that sensor, used for sampling interval.
struct ThreadSetting {
    Peripheral::Sensors &sensor;
    Clock::Timer &sampleInterval;

    ThreadSetting(Peripheral::Sensors &sensor, Clock::Timer &sampleInterval);
};

// A compilation of all of the thread settings for a single thread.
struct ThreadSettingCompilation {
    ThreadSetting &tempHum; ThreadSetting &light;
    ThreadSetting &soil1; 

    ThreadSettingCompilation(
        ThreadSetting &tempHum, ThreadSetting &light, ThreadSetting &soil1);
};

class SensorThread {
    private:
    TaskHandle_t taskHandle;
    Messaging::MsgLogHandler &msglogerr;
    
    public:
    SensorThread(Messaging::MsgLogHandler &msglogerr);
    void initThread(ThreadSettingCompilation &settings);
    static void sensorTask(void* parameter);
    void suspendTask();
    void resumeTask();
};

}

#endif // THREADS_H