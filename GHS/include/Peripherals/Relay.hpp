#ifndef RELAY_HPP
#define RELAY_HPP

#include "driver/gpio.h"

namespace Peripheral {

// Specifies the max amount of devices that can control single relay.
#define RELAY_IDS 10 

enum class CONDITION : uint8_t {LESS_THAN, GTR_THAN, NONE};
enum class STATE : uint8_t {OFF, ON, FORCED_OFF, FORCE_REMOVED};

struct Timer {
    uint32_t onTime;
    uint32_t offTime;
    bool onSet;
    bool offSet;
    bool isReady;
};

class Relay {
    private:
    gpio_num_t pin;
    STATE state;

    // Each controlling sensor, when turning relay on, will have its
    // unique ID added to this array. It will be deleted when turning
    // off.
    uint16_t IDs[RELAY_IDS];
    uint16_t clientQty;
    uint16_t IDReg;
    Timer timer;
    bool checkID(uint16_t ID);
    bool addID(uint16_t ID);
    bool delID(uint16_t ID);

    public:
    Relay(gpio_num_t pin);
    void on(uint16_t ID);
    void off(uint16_t ID);
    void forceOff();
    void removeForce();
    uint16_t getID();
    STATE getState();
    bool timerSet(bool on, int time);
    void manageTimer();
    Timer* getTimer();
};

}

#endif // RELAY_HPP