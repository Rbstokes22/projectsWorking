#include "Common/Timing.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


namespace Clock {

Timer::Timer(uint32_t interval) : 

previousMillis{millis()}, interval{interval}{};

// 10 ms increment values. Less granularity and overhead than the arduino
// millis() function. Can change the resolution if needed by modifying
// the portTICK_PERIOD_MS.
uint32_t Timer::millis() {
  return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

bool Timer::isReady() {
    uint32_t currentMillis = millis();
    if ((currentMillis - this->previousMillis) >= this->interval) {
      this->previousMillis = currentMillis;
      return true;
    } else {
      return false;
    }
  }

void Timer::changeInterval(uint32_t newInterval) {
  this->interval = newInterval;
}

}