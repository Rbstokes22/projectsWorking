#ifndef RELAY_HPP
#define RELAY_HPP

#include "driver/gpio.h"

namespace Peripheral {

// Used to 
#define RELAY_IDS 10 

enum class CONDITION : uint8_t {LESS_THAN, GTR_THAN};

class Relay {
    private:
    gpio_num_t pin;
    bool isOn;
    uint8_t IDs[RELAY_IDS];
    uint8_t clientQty;
    bool checkID(uint8_t ID);
    bool addID(uint8_t ID);
    bool delID(uint8_t ID);

    public:
    Relay(gpio_num_t pin);
    void on(uint8_t ID);
    void off(uint8_t ID);
    
};

struct RELAY_CONFIG {
    int tripVal;
    Relay* relay;
    CONDITION condition;
    uint8_t relayControlID;
};

}

#endif // RELAY_HPP