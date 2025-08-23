#ifndef SOIL_HPP
#define SOIL_HPP

#include "Peripherals/Alert.hpp"
#include "Threads/Mutex.hpp"
#include "UI/MsgLogHandler.hpp"
#include "Drivers/ADC.hpp"

namespace Peripheral {

#define SOIL_SENSORS 4 // total soil sensors
#define SOIL_HYSTERESIS 20 // padding for reset value
#define SOIL_NOISE 10 // Used to prevent noise in the analog read. Must be > 0
#define SOIL_ERR_MAX 3 // Max Error counts before display shows error.
#define SOIL_CONSECUTIVE_CTS 5 // consecutive counts before sending alert.
#define SOIL_ALT_MSG_SIZE 64 // Alert message size
#define SOIL_ALT_MSG_ATT 3 // Attempts to send an alert
#define SOIL_MIN 1 // 0, but cant set lower. 12 bit value.
#define SOIL_MAX 32766 // int 16 max - 1, due to unable to set abv max.
#define SOIL_LOG_METHOD Messaging::Method::SRL_LOG
#define SOIL_TAG "(SOIL)"

// Alert configuration, relay configurations are omitted for soil sensors due
// to potential to overwater if there are capacitance issues, this is a 
// liability thing. All variables serve as a packet of data assigned to
// each sensor to allow proper handling and sending to the server.
struct AlertConfigSo {
    int tripVal; // value which trips the soil alert.
    ALTCOND condition; // Alert condition.
    ALTCOND prevCondition; // Alert previous condition.
    size_t onCt; // Consecutive on counts to send alert.
    size_t offCt; // Consecutive off counts to reset alert.
    bool toggle; // Blocker to ensure that only 1 message is sent per violation.
    const uint8_t ID; // ID number of soil sensor
    uint8_t attempts; // Attempts to send alert before timeout
};

// All soil parameters required for init.
struct SoilParams {
    ADC_DRVR::ADC &soil; // Soil ADC.
};

// Packet for capturing the read value, and if the data is good to use or there
// is an immeidate or diplay level error.
struct SoilReadings {
    int16_t val; // Read value
    bool noDispErr; // Used for display after consecutive errors
    bool noErr; // Used immediately to prevent errors, shows datasafe.
    size_t errCt; // error count.
};

class Soil {
    private:
    static const char* tag;
    static char log[LOG_MAX_ENTRY];
    SoilReadings data[SOIL_SENSORS];
    int16_t trends[SOIL_SENSORS][TREND_HOURS]; // Trends of each soil sensor.
    static Threads::Mutex mtx;
    AlertConfigSo conf[SOIL_SENSORS];
    SoilParams &params;
    Soil(SoilParams &params); 
    Soil(const Soil&) = delete; // prevent copying
    Soil &operator=(const Soil&) = delete; // prevent assignment
    static void handleAlert(AlertConfigSo &conf, SoilReadings &data, 
        bool alertOn, size_t ct);

    static void sendErr(const char* msg, Messaging::Levels lvl =
        Messaging::Levels::ERROR);

    void computeTrends(uint8_t indexNum);
    
    public:
    // set to nullptr to reduce arguments when calling after init.
    static Soil* get(SoilParams* parameter = nullptr);
    AlertConfigSo* getConfig(uint8_t indexNum);
    void readAll();
    SoilReadings* getReadings(uint8_t indexNum);
    void checkBounds();
    int16_t* getTrends(uint8_t indexNum);
    // void test(int val, int sensorIdx); // Comment out for production
};

}

#endif // SOIL_HPP