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
#define TEMP_HUM_CONSECUTIVE_CTS 5 // Action isnt taken until cts are read

struct TH_TRIP_CONFIG { // Config relays and alerts
    int tripValRelay; // Value that will turn relay on
    int tripValAlert; // Value that will trigger alert
    bool alertsEn; // Are alerts enabled
    CONDITION condition; // Lower than or greater than
    CONDITION prevCondition; // Used for resetting counts
    Relay* relay; // relay assigned to device
    uint8_t relayNum; // Relay 1 - 4, used for display
    uint16_t relayControlID; // Used to control relay

    // Ensures that relay and alert action is not taken at 
    // first trip value, but successive values being met.
    uint32_t relayOnCt; 
    uint32_t relayOffCt;
    uint32_t alertOnCt;
    uint32_t alertOffCt;
};

struct TH_Averages {
    size_t pollCt; // How many times has sensor been polled
    float temp; // temp accum / pollCt
    float hum; // hum accum / pollCt
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
    float getHum();
    float getTemp();
    TH_TRIP_CONFIG* getHumConf();
    TH_TRIP_CONFIG* getTempConf();
    void checkBounds();
    void handleRelay(TH_TRIP_CONFIG &config, bool relayOn, uint32_t ct);
    void handleAlert(TH_TRIP_CONFIG &config, bool alertOn, uint32_t ct);
    isUpTH getStatus();
    TH_Averages* getAverages(bool reset = false);
};


}

#endif // TEMPHUM_HPP