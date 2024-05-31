#ifndef TIMING_H
#define TIMING_H

class Timer {
    private:
    unsigned long previousMillis;
    unsigned long interval;
    unsigned long reminderPreviousMillis;
    bool reminderToggle;

    public:
    Timer(unsigned long interval); // constructor
    bool isReady();
    bool setReminder(unsigned long milliseconds);
    void changeInterval(unsigned long newInterval);
};


#endif // TIMING_H