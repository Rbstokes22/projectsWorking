#ifndef SOIL_HPP
#define SOIL_HPP

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_continuous.h"
#include "Peripherals/Relay.hpp"
#include "Threads/Mutex.hpp"
#include "UI/MsgLogHandler.hpp"

namespace Peripheral {

#define SOIL_SENSORS 4

struct SOIL_TRIP_CONFIG {
    int tripValAlert;
    bool alertsEn;
    CONDITION condition;
};

struct SoilParams {
    Messaging::MsgLogHandler &msglogerr;
    adc_oneshot_unit_handle_t handle;
    adc_channel_t* channels;
};

struct isUpSoil {
    bool display;
    bool immediate;
};

class Soil {
    private:
    int readings[SOIL_SENSORS];
    isUpSoil flags[SOIL_SENSORS];
    Threads::Mutex mtx;
    SOIL_TRIP_CONFIG settings[SOIL_SENSORS];
    SoilParams &params;
    Soil(SoilParams &params); 
    Soil(const Soil&) = delete; // prevent copying
    Soil &operator=(const Soil&) = delete; // prevent assignment
    
    public:
    // set to nullptr to reduce arguments when calling after init.
    static Soil* get(SoilParams* parameter = nullptr);
    SOIL_TRIP_CONFIG* getConfig(uint8_t indexNum);
    void readAll();
    void getAll(int* readings, size_t bytes);
    void checkBounds();
    void handleAlert(SOIL_TRIP_CONFIG &config, bool alertOn);
    isUpSoil* getStatus(uint8_t indexNum);
};

}

#endif // SOIL_HPP