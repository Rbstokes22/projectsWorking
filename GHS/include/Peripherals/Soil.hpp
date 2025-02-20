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
#define SOIL_ERR_MAX 3 // Max Error counts before display shows error.
#define SOIL_CONSECUTIVE_CTS 5 // consecutive counts before sending alert.
#define SOIL_ALT_MSG_SIZE 64 // Alert message size
#define SOIL_ALT_MSG_ATT 3 // Attempts to send an alert
#define SOIL_MIN 1 // 0, but cant set lower. 12 bit value.
#define SOIL_MAX 4094 // 4095, but cant set higher.

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
    adc_oneshot_unit_handle_t handle; // ADC handle
    adc_channel_t* channels; // ADC channels.
};

// Packet for capturing the read value, and if the data is good to use or there
// is an immeidate or diplay level error.
struct SoilReadings {
    int val; // Read value
    bool noDispErr; // Used for display after consecutive errors
    bool noErr; // Used immediately to prevent errors, shows datasafe.
    size_t errCt; // error count.
};

class Soil {
    private:
    SoilReadings data[SOIL_SENSORS];
    static Threads::Mutex mtx;
    AlertConfigSo conf[SOIL_SENSORS];
    SoilParams &params;
    Soil(SoilParams &params); 
    Soil(const Soil&) = delete; // prevent copying
    Soil &operator=(const Soil&) = delete; // prevent assignment
    void handleAlert(AlertConfigSo &conf, SoilReadings &data, bool alertOn, 
        size_t ct);
    
    public:
    // set to nullptr to reduce arguments when calling after init.
    static Soil* get(SoilParams* parameter = nullptr);
    AlertConfigSo* getConfig(uint8_t indexNum);
    void readAll();
    SoilReadings* getReadings(uint8_t indexNum);
    void checkBounds();
    // void test(int val, int sensorIdx); // Comment out for production
};

}

#endif // SOIL_HPP