#include "Common/Timing.h"
#include <Arduino.h>

namespace Clock {

Timer::Timer(unsigned long interval) : 
previousMillis{millis()}, interval{interval}{};

bool Timer::isReady() {
    unsigned long currentMillis = millis();
    if ((currentMillis - this->previousMillis) >= this->interval) {
      this->previousMillis = currentMillis;
      return true;
    } else {
      return false;
    }
  }

void Timer::changeInterval(unsigned long newInterval) {
  this->interval = newInterval;
}

}