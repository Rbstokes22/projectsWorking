#ifndef TEMPHUM_HPP
#define TEMPHUM_HPP

#include "driver/gpio.h"
#include "Drivers/SHT_Library.hpp"
#include "Peripherals/Relay.hpp"
#include "Peripherals/Alert.hpp"
#include "Threads/Mutex.hpp"
#include "UI/MsgLogHandler.hpp"

namespace Peripheral {

#define TEMP_HUM_HYSTERESIS 2.0f // Padding for reset value
#define TEMP_HUM_CONSECUTIVE_CTS 5 // Action isnt taken until cts are read
#define TEMP_HUM_ERR_CT_MAX 5 // Error counts to show error on display
#define TEMP_HUM_ALT_MSG_ATT 3 // Alert Message Attempts to avoid request excess (< 256)
#define TEMP_HUM_ALT_MSG_SIZE 64 // Alert message size to send to server.

// Alert configuration. The on and off counts are to ensure that consecutive
// counts are taken into consideration before sending or resetting alert.
// All the variables serve as a packet of data assigned to each sensor to 
// allow proper handling and sending to the server.
struct alertConfig {
    int tripVal; // Sends alert
    ALTCOND condition; // Alert condition.
    ALTCOND prevCondition; // ALert previous condition.
    uint32_t onCt; // Consecutive on counts to send alert.
    uint32_t offCt; // Consecutive off counts to reset alert.
    bool toggle; // Blocker to ensure that only 1 message is sent per violation.
    uint8_t attempts; // Attempt count to send alert
};

struct relayConfig {
    int tripVal; // Turns relay on.
    RECOND condition; // Relay condition.
    RECOND prevCondition; // Relays previous condition.
    Relay* relay; // Relay attached to device.
    uint8_t num; // relay index + 1, for display purposes.
    uint8_t controlID; // ID given by relay to allow this device to control it.
    uint32_t onCt; // Consecutive on counts to turn relay on.
    uint32_t offCt; // Consecutive off counts to turn relay off.
};

// Combined alert and relay configurations.
struct TH_TRIP_CONFIG { 
    alertConfig alt;
    relayConfig relay;
};

// Temperature and humidity averages. 
struct TH_Averages {
    size_t pollCt; // How many times has sensor been polled
    float temp; // temp accum / pollCt
    float hum; // hum accum / pollCt
    float prevTemp; // Previous values copied when cleared.
    float prevHum;
};

// temperature and humidity parameters.
struct TempHumParams {
    Messaging::MsgLogHandler &msglogerr;
    SHT_DRVR::SHT &sht;
};

// Serves to see if the temp and hum sensor is up or down.
struct isUpTH { 
    bool display; // Used for display after consecutive errors
    bool immediate; // used immediately to prevent relay errors
};

class TempHum {
    private:
    SHT_DRVR::SHT_VALS data;
    TH_Averages averages;
    isUpTH flags;
    Threads::Mutex mtx;
    TH_TRIP_CONFIG humConf;
    TH_TRIP_CONFIG tempConf;
    TempHumParams &params;
    TempHum(TempHumParams &params); 
    TempHum(const TempHum&) = delete; // prevent copying
    TempHum &operator=(const TempHum&) = delete; // prevent assignment
    void handleRelay(relayConfig &conf, bool relayOn, uint32_t ct);
    void handleAlert(alertConfig &config, bool alertOn, uint32_t ct);
    void relayBounds(float value, relayConfig &conf);
    void alertBounds(float value, alertConfig &conf);
    void computeAvgs();
    
    public:
    static TempHum* get(TempHumParams* parameter = nullptr);
    bool read();
    float getHum();
    float getTemp(char CorF = 'C');
    TH_TRIP_CONFIG* getHumConf();
    TH_TRIP_CONFIG* getTempConf();
    bool checkBounds();
    isUpTH getStatus();
    TH_Averages* getAverages();
    void clearAverages();
    // void test(bool isTemp, float val); // Uncomment out when testing.
};

}

#endif // TEMPHUM_HPP