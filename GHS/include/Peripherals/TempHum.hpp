#ifndef TEMPHUM_HPP
#define TEMPHUM_HPP

#include "driver/gpio.h"
#include "Peripherals/Relay.hpp"
#include "Peripherals/Alert.hpp"

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
    static BOUNDARY_CONFIG humConf;
    static BOUNDARY_CONFIG tempConf;
    static float getHum();
    static float getTemp();
    void setHum(float val);
    void setTemp(float val);
    static BOUNDARY_CONFIG* getHumConf();
    static BOUNDARY_CONFIG* getTempConf();
    void checkBounds();
    void handleRelay(BOUNDARY_CONFIG &config, bool relayOn);
    void handleAlert();
    static void setStatus(bool isUp);
    static bool getStatus();
};


}

#endif // TEMPHUM_HPP