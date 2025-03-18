#ifndef TEMPHUM_HPP
#define TEMPHUM_HPP

#include "driver/gpio.h"
#include "Drivers/SHT_Library.hpp"
#include "Peripherals/Relay.hpp"
#include "Peripherals/Alert.hpp"
#include "Threads/Mutex.hpp"
#include "Common/FlagReg.hpp"
#include "UI/MsgLogHandler.hpp"

namespace Peripheral {

#define TEMP_HUM_HYSTERESIS 2.0f // Padding for reset value
#define TEMP_HUM_CONSECUTIVE_CTS 5 // Action isnt taken until cts are read
#define TEMP_HUM_ERR_CT_MAX 3 // Error counts to show error on display
#define TEMP_HUM_ALT_MSG_ATT 3 // Alert Message Attempts to avoid request excess
#define TEMP_HUM_ALT_MSG_SIZE 64 // Alert message size to send to server.
#define TEMP_HUM_NO_RELAY 255 // Used to show no relay attached.
#define TEMP_HUM_LOG_METHOD Messaging::Method::SRL_LOG

// Alert configuration. All variables serve as a packet of data assigned to
// each sensor to allow proper handling and sending to the server.
struct alertConfigTH {
    int tripVal; // Sends alert
    ALTCOND condition; // Alert condition.
    ALTCOND prevCondition; // Alert previous condition.
    size_t onCt; // Consecutive on counts to send alert.
    size_t offCt; // Consecutive off counts to reset alert.
    bool toggle; // Blocker to ensure that only 1 message is sent per violation.
    uint8_t attempts; // Attempt count to send alert
};

// Relay configuration. All variables server as a packet of data assigned to
// each sensor to allow proper handling of relays.
struct relayConfigTH {
    int tripVal; // Turns relay on.
    RECOND condition; // Relay condition.
    RECOND prevCondition; // Relays previous condition.
    Relay* relay; // Relay attached to device.
    uint8_t num; // relay index, relay 1 will show 0.
    uint8_t controlID; // ID given by relay to allow this device to control it.
    size_t onCt; // Consecutive on counts to turn relay on.
    size_t offCt; // Consecutive off counts to turn relay off.
};

// Combined alert and relay configurations.
struct TH_TRIP_CONFIG { 
    alertConfigTH alt; // alert
    relayConfigTH relay; // relay
};

// Temperature and humidity averages. 
struct TH_Averages {
    size_t pollCt; // How many times has sensor been polled
    float temp; // temp accum / pollCt
    float hum; // hum accum / pollCt
    float prevTemp; // Previous values copied when cleared.
    float prevHum; // Previous values copied when cleared.
};

// temperature and humidity parameters required for init.
struct TempHumParams {
    SHT_DRVR::SHT &sht;
};

enum THFLAGS : uint8_t {
    NO_ERR, // Used for display after consecutive errors
    NO_ERR_DISP // Used for immediate error to preven pre-mature relay/alt act
};

class TempHum {
    private:
    static const char* tag;
    static char log[LOG_MAX_ENTRY];
    SHT_DRVR::SHT_VALS data;
    TH_Averages averages;
    Flag::FlagReg flags;
    static Threads::Mutex mtx;
    TH_TRIP_CONFIG humConf;
    TH_TRIP_CONFIG tempConf;
    TempHumParams &params;
    TempHum(TempHumParams &params); 
    TempHum(const TempHum&) = delete; // prevent copying
    TempHum &operator=(const TempHum&) = delete; // prevent assignment
    void handleRelay(relayConfigTH &conf, bool relayOn, size_t ct);
    void handleAlert(alertConfigTH &config, bool alertOn, size_t ct);
    void relayBounds(float value, relayConfigTH &conf, bool isTemp);
    void alertBounds(float value, alertConfigTH &conf, bool isTemp);
    void computeAvgs();
    static void sendErr(const char* msg, Messaging::Levels lvl = 
        Messaging::Levels::ERROR);

    public:
    static TempHum* get(TempHumParams* parameter = nullptr);
    bool read();
    float getHum();
    float getTemp(char CorF = 'C');
    TH_TRIP_CONFIG* getHumConf();
    TH_TRIP_CONFIG* getTempConf();
    bool checkBounds();
    Flag::FlagReg* getFlags();
    TH_Averages* getAverages();
    void clearAverages();
    // void test(bool isTemp, float val); // Uncomment out when testing.
};

}

#endif // TEMPHUM_HPP