#ifndef THREADTASKS_HPP
#define THREADTASKS_HPP
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace ThreadTask {

#define HWM_MIN_WORDS 512 // High water mark minimum words. 

// Heartbeats are the seconds that will trigger an alert and reset if task
// does not check in with the heartbeat class. Must be a unit8_t > 0. Recommend
// using the task frequency/delay + 1. So if the thread delay is 1000 or 1s,
// set the check to 2s. You can also set to + 2 if desired.
#define NET_HEARTBEAT 3 
#define TEMPHUM_HEARTBEAT 3
#define LIGHT_HEARTBEAT 5
#define SOIL_HEARTBEAT 3
#define ROUTINE_HEATBEAT 3

#define HB_DELAY 10 // Populates the heartbeat with this val upon registration.

// These fall into the configuration required to create a thread. These funcs
// are passed, requiring a void* param. A thread parameter struct is created
// for each, and will be recasted once created to use desired params.

TickType_t delay(TickType_t work, TickType_t period);

void highWaterMark(const char* tag, UBaseType_t HWM);
void endOfDay();
void netTask(void* parameter);
void SHTTask(void* parameter);
void LightTask(void* parameter);
void soilTask(void* parameter);
void routineTask(void* parameter);

}

#endif // THREADTASKS_HPP