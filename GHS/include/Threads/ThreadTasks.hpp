#ifndef THREADTASKS_HPP
#define THREADTASKS_HPP
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace ThreadTask {

#define HWM_MIN_WORDS 500 // High water mark minimum words. 

// These fall into the configuration required to create a thread. These funcs
// are passed, requiring a void* param. A thread parameter struct is created
// for each, and will be recasted once created to use desired params.

void highWaterMark(const char* tag, UBaseType_t HWM);
void netTask(void* parameter);
void SHTTask(void* parameter);
void AS7341Task(void* parameter);
void soilTask(void* parameter);
void routineTask(void* parameter);

}

#endif // THREADTASKS_HPP