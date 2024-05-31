#include "Timing.h"
#include <Arduino.h>

Timer::Timer(unsigned long interval) : 
previousMillis(millis()), 
interval(interval),
reminderPreviousMillis(millis()),
reminderToggle(false){};

bool Timer::isReady() {
    unsigned long currentMillis = millis();
    if (currentMillis - this->previousMillis >= this->interval) {
      this->previousMillis = currentMillis;
      return true;
    } else {
      return false;
    }
  }

// This works sort of like setTimeout in JS. This will only work for the
// object at once though, you cant set another until the old one is 
// complete.
bool Timer::setReminder(unsigned long milliseconds) {
  if (this->reminderToggle == false) {
    this->reminderToggle = true;
    reminderPreviousMillis = millis();
    return false;
  } else {
    if (millis() - this->reminderPreviousMillis >= milliseconds) {
      this->reminderToggle = false;
      return true;
    } else {
      return false;
    }
  }
}

void Timer::changeInterval(unsigned long newInterval) {
  this->interval = newInterval;
}