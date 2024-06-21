#ifndef TEMPHUM_H
#define TEMPHUM_H

#include <DHT.h>
#include "Peripherals/PeripheralMain.h"

namespace Peripheral {

class TempHum : public Sensors { // DHT-22
    private:
    DHT dht;
    float Temp;
    float Hum;
    static const float ERR;
    uint8_t maxRetries; // used for error handling in temp/hum

    public:
    TempHum(
        PERPIN pin, 
        uint8_t type, 
        uint8_t maxRetries,
        Messaging::MsgLogHandler &msglogerr);
    void begin();
    void setTemp();
    void setHum();
    float getTemp(char tempUnits = 'C');
    float getHum();
    void handleSensors() override;
};

}

#endif // TEMPHUM_H