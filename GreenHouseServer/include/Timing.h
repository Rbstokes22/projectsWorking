#ifndef TIMING_H
#define TIMING_H

// All timed data and functions go here for preceise timing throughout
// the program.
namespace Clock {

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

}
#endif // TIMING_H