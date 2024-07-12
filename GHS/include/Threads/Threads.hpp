#ifndef THREADS_HPP
#define THREADS_HPP

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "UI/MsgLogHandler.hpp"
#include "Threads/Mutex.hpp"
// #include "Peripherals/Light.h"
// #include "Peripherals/TempHum.h"
// #include "Peripherals/Soil.h"
// #include "Common/Timing.h"
 
namespace Threads {

class Thread {
    private:
    TaskHandle_t taskHandle;
    Messaging::MsgLogHandler &msglogerr;

    public:
    Thread(Messaging::MsgLogHandler &msglogerr);
    void initThread(
        void (*taskFunc)(void*), 
        const char* name, 
        uint16_t stackSize, 
        void* parameters, 
        UBaseType_t priority);
    void suspendTask();
    void resumeTask();
};

// class SensorThread {
//     private:
//     TaskHandle_t taskHandle;
//     Messaging::MsgLogHandler &msglogerr;
//     static const uint8_t totalSensors;
    
//     public:
//     SensorThread(Messaging::MsgLogHandler &msglogerr);
//     void initThread(Peripheral::Sensors** allSensors);
//     static void sensorTask(void* parameter);
//     static void mutexWrap(Peripheral::Sensors* sensor);
//     void suspendTask();
//     void resumeTask();
// };

}

#endif // THREADS_HPP