#include "Common/Timing.h"


namespace Clock {

Timer::Timer(uint32_t interval) : 

previousMillis{millis()}, interval{interval}{};

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