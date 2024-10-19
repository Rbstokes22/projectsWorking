#ifndef RELAY_HPP
#define RELAY_HPP

#include "driver/gpio.h"

namespace Peripheral {

// Specifies the max amount of devices that can control single relay.
#define RELAY_IDS 10 

enum class CONDITION : uint8_t {LESS_THAN, GTR_THAN, NONE};

class Relay {
    private:
    gpio_num_t pin;
    bool _isOn;

    // Each controlling sensor, when turning relay on, will have its
    // unique ID added to this array. It will be deleted when turning
    // off.
    uint16_t IDs[RELAY_IDS];
    uint16_t clientQty;
    static uint16_t IDReg;
    bool checkID(uint16_t ID);
    bool addID(uint16_t ID);
    bool delID(uint16_t ID);

    public:
    Relay(gpio_num_t pin);
    void on(uint16_t ID);
    void off(uint16_t ID);
    uint16_t getID();
    bool isOn();
};

struct RELAY_CONFIG {
    int tripVal;
    Relay* relay;
    CONDITION condition;
    uint16_t relayControlID;
};

}

#endif // RELAY_HPP