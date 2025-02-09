#ifndef SOIL_HPP
#define SOIL_HPP

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_continuous.h"
#include "Peripherals/Alert.hpp"
#include "Threads/Mutex.hpp"
#include "UI/MsgLogHandler.hpp"

namespace Peripheral {

#define SOIL_SENSORS 4 // total soil sensors
#define SOIL_HYSTERESIS 20 // padding for reset value
#define SOIL_ERR_MAX 5 // Max Error counts before display shows error.
#define SOIL_CONSECUTIVE_CTS 5 // consecutive counts before sending alert.
#define SOIL_ALT_MSG_SIZE 64 // Alert message size
#define SOIL_ALT_MSG_ATT 3 // Attempts to send an alert
#define SOIL_MIN_READ 1 // 0, but cant set lower. 12 bit value.
#define SOIL_MAX_READ 4094 // 4095, but cant set higher.

// Alert configuration, relay configurations are omitted for soil sensors due
// to potential to overwater if there are capacitance issues, this is a 
// liability thing. All variables serve as a packet of data assigned to
// each sensor to allow proper handling and sending to the server.
struct SOIL_TRIP_CONFIG {
    int tripVal; // value which trips the soil alert.
    ALTCOND condition; // Alert condition.
    ALTCOND prevCondition; // Alert previous condition.
    uint32_t onCt; // Consecutive on counts to send alert.
    uint32_t offCt; // Consecutive off counts to reset alert.
    bool toggle; // Blocker to ensure that only 1 message is sent per violation.
    const uint8_t ID; // ID number of soil sensor
    uint8_t attempts; // Attempts to send alert before timeout
};

// All soil parameters required for init.
struct SoilParams {
    adc_oneshot_unit_handle_t handle; // ADC handle
    adc_channel_t* channels; // ADC channels.
};

// Packet for capturing the read value, and if the data is good to use or there
// is an immeidate or diplay level error.
struct SoilReadings {
    int val; // Read value
    bool display; // Used for display after consecutive errors
    bool immediate; // Used immediately to prevent relay errors, shows datasafe
    size_t errCt; // error count.
};

class Soil {
    private:
    SoilReadings data[SOIL_SENSORS];
    static Threads::Mutex mtx;
    SOIL_TRIP_CONFIG conf[SOIL_SENSORS];
    SoilParams &params;
    Soil(SoilParams &params); 
    Soil(const Soil&) = delete; // prevent copying
    Soil &operator=(const Soil&) = delete; // prevent assignment
    void handleAlert(SOIL_TRIP_CONFIG &conf, SoilReadings &data, bool alertOn, 
        uint32_t ct);
    
    public:
    // set to nullptr to reduce arguments when calling after init.
    static Soil* get(SoilParams* parameter = nullptr);
    SOIL_TRIP_CONFIG* getConfig(uint8_t indexNum);
    void readAll();
    SoilReadings* getReadings(uint8_t indexNum);
    void checkBounds();
};

}

#endif // SOIL_HPP