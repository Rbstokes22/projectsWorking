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

struct SOIL_TRIP_CONFIG {
    int tripVal;
    ALTCOND condition;
    ALTCOND prevCondition;
    uint32_t onCt;
    uint32_t offCt;
    bool toggle;
    const uint8_t ID; // ID number of soil sensor
    uint8_t attempts; // Attempts to send alert before timeout
};

struct SoilParams {
    Messaging::MsgLogHandler &msglogerr;
    adc_oneshot_unit_handle_t handle;
    adc_channel_t* channels;
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
    Threads::Mutex mtx;
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