#ifndef RELAY_HPP
#define RELAY_HPP

#include "driver/gpio.h"
#include "Threads/Mutex.hpp"
#include <cstdint>

namespace Peripheral {

// Specifies the max amount of devices that can control single relay.
#define RELAY_IDS 10 
#define RELAY_TIMER_OFF 99999

enum class RECOND : uint8_t {LESS_THAN, GTR_THAN, NONE}; // Relay condition
enum class RESTATE : uint8_t {OFF, ON, FORCED_OFF, FORCE_REMOVED};

// Client ID availability/state. RESERVED is reserved for initial
// acquisition.
enum class IDSTATE : uint8_t {AVAILABLE, RESERVED, ON, OFF};

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
    uint8_t ReNum;
    RESTATE relayState;
    IDSTATE clients[RELAY_IDS];
    uint8_t clientQty; // Clients currently energizing relay.
    Timer timer;
    Threads::Mutex mtx;
    bool isAttached(uint8_t ID);
    bool changeIDState(uint8_t ID, IDSTATE newState);

    public:
    Relay(gpio_num_t pin, uint8_t ReNum);
    bool on(uint8_t ID);
    bool off(uint8_t ID);
    void forceOff();
    void removeForce();
    uint8_t getID();
    bool removeID(uint8_t ID);
    RESTATE getState();
    bool timerSet(bool on, uint32_t time);
    void manageTimer();
    Timer* getTimer();
};

}

#endif // RELAY_HPP