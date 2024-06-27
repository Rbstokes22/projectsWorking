#ifndef THREADS_H
#define THREADS_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "UI/MsgLogHandler.h"
#include "Peripherals/Light.h"
#include "Peripherals/TempHum.h"
#include "Peripherals/Soil.h"
#include "Common/Timing.h"
 
namespace Threads {

class SensorThread {
    private:
    TaskHandle_t taskHandle;
    Messaging::MsgLogHandler &msglogerr;
    static const uint8_t totalSensors;
    
    public:
    SensorThread(Messaging::MsgLogHandler &msglogerr);
    void initThread(Peripheral::Sensors** allSensors);
    static void sensorTask(void* parameter);
    static void mutexWrap(Peripheral::Sensors* sensor);
    void suspendTask();
    void resumeTask();
};

}

#endif // THREADS_H