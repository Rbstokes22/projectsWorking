#ifndef DHT_LIBRARY_HPP
#define DHT_LIBRARY_HPP

#include "driver/gpio.h"

namespace DHT_DRVR {

enum class PVAL {
    LOW, HIGH
};

enum class TEMP {
    F, C
};

class DHT {
    private:
    gpio_num_t pin;
    uint32_t getDuration(PVAL pinVal, uint16_t timeout);
    void calcF(float &temp);

    public:
    DHT(gpio_num_t pin);
    bool read(float &temp, float &hum, TEMP scale = TEMP::C);
};

}

#endif // DHT_LIBRARY_HPP

