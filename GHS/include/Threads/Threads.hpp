#ifndef THREADS_HPP
#define THREADS_HPP

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "UI/MsgLogHandler.hpp"
// #include "Peripherals/Light.h" // Uncomment when ready to develop
// #include "Peripherals/TempHum.h"
// #include "Peripherals/Soil.h"
// #include "Common/Timing.h"
 
namespace Threads {

class Thread {
    private:
    TaskHandle_t taskHandle;
    Messaging::MsgLogHandler &msglogerr;
    const char* name;

    public:
    Thread(Messaging::MsgLogHandler &msglogerr, const char* name);
    void initThread(
        void (*taskFunc)(void*), // task function
        uint16_t stackSize, // stack size in words.
        void* parameters, // task input parameters
        UBaseType_t priority); // priority of task
    void suspendTask();
    void resumeTask();
    ~Thread();
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