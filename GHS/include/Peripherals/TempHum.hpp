#ifndef TEMPHUM_HPP
#define TEMPHUM_HPP

#include "driver/gpio.h"
#include "Peripherals/Relay.hpp"

namespace Peripheral {

#define TEMP_HUM_PADDING 1.0f

class TempHum {
    private:
    static float temp;
    static float hum;
    static gpio_num_t humPin;
    static gpio_num_t tempPin;
    static bool isUp;

    public:
    static RELAY_CONFIG humConf;
    static RELAY_CONFIG tempConf;
    static float getHum();
    static float getTemp();
    void setHum(float val);
    void setTemp(float val);
    // static void setHumConf(RELAY_CONFIG &config);
    // static void setTempConf(RELAY_CONFIG &config);
    static RELAY_CONFIG* getHumConf();
    static RELAY_CONFIG* getTempConf();
    void checkBounds();
    void handleRelay(RELAY_CONFIG &config, bool relayOn);
    static void setStatus(bool isUp);
    static bool getStatus();



};


}

#endif // TEMPHUM_HPP