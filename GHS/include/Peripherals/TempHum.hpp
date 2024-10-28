#ifndef TEMPHUM_HPP
#define TEMPHUM_HPP

#include "driver/gpio.h"
#include "Peripherals/Relay.hpp"
#include "Peripherals/Alert.hpp"
#include "Threads/Mutex.hpp"
#include "UI/MsgLogHandler.hpp"

namespace Peripheral {

#define TEMP_HUM_PADDING 1.0f

struct TH_TRIP_CONFIG { // Config relays and alerts
    int tripValRelay;
    int tripValAlert;
    bool alertsEn;
    CONDITION condition;
    Relay* relay;
    uint8_t relayNum;
    uint16_t relayControlID;
};

struct TempHumParams {
    Messaging::MsgLogHandler &msglogerr;
};

class TempHum {
    private:
    float temp;
    float hum;
    bool isUp;
    Threads::Mutex mtx;
    TH_TRIP_CONFIG humConf;
    TH_TRIP_CONFIG tempConf;
    TempHum(TempHumParams &params); 
    TempHum(const TempHum&) = delete; // prevent copying
    TempHum &operator=(const TempHum&) = delete; // prevent assignment

    public:
    static TempHum* get(void* parameter = nullptr);
    void getHum(float &hum);
    void getTemp(float &temp);
    void setHum(float val);
    void setTemp(float val);
    TH_TRIP_CONFIG* getHumConf();
    TH_TRIP_CONFIG* getTempConf();
    void checkBounds();
    void handleRelay(TH_TRIP_CONFIG &config, bool relayOn);
    void handleAlert(TH_TRIP_CONFIG &config, bool alertOn);
    void setStatus(bool isUp);
    bool getStatus();
};


}

#endif // TEMPHUM_HPP