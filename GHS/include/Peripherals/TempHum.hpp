#ifndef TEMPHUM_HPP
#define TEMPHUM_HPP

#include "driver/gpio.h"
#include "Drivers/DHT_Library.hpp"
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

struct TH_Averages {
    size_t pollCt;
    float temp;
    float hum;
};

struct TempHumParams {
    Messaging::MsgLogHandler &msglogerr;
    DHT_DRVR::DHT &dht;
};

struct isUpTH { // is up Temp Hum
    bool display; // Used for display after consecutive errors
    bool immediate; // used immediately to prevent relay errors
};

class TempHum {
    private:
    float temp;
    float hum;
    TH_Averages averages;
    isUpTH flags;
    Threads::Mutex mtx;
    TH_TRIP_CONFIG humConf;
    TH_TRIP_CONFIG tempConf;
    TempHumParams &params;
    TempHum(TempHumParams &params); 
    TempHum(const TempHum&) = delete; // prevent copying
    TempHum &operator=(const TempHum&) = delete; // prevent assignment

    public:
    static TempHum* get(TempHumParams* parameter = nullptr);
    bool read();
    void getHum(float &hum);
    void getTemp(float &temp);
    TH_TRIP_CONFIG* getHumConf();
    TH_TRIP_CONFIG* getTempConf();
    void checkBounds();
    void handleRelay(TH_TRIP_CONFIG &config, bool relayOn);
    void handleAlert(TH_TRIP_CONFIG &config, bool alertOn);
    isUpTH getStatus();
    TH_Averages* getAverages(bool reset = false);
};


}

#endif // TEMPHUM_HPP