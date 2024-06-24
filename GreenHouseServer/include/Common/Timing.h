#ifndef TIMING_H
#define TIMING_H

// All timed data and functions go here for preceise timing throughout
// the program.
namespace Clock {

class Timer {
    private:
    unsigned long previousMillis;
    unsigned long interval;

    public:
    Timer(unsigned long interval); 
    bool isReady();
    void changeInterval(unsigned long newInterval);
};

}
#endif // TIMING_H