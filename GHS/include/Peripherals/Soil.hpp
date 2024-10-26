#ifndef SOIL_HPP
#define SOIL_HPP

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_continuous.h"
#include "Peripherals/Relay.hpp"
#include "Threads/Mutex.hpp"
#include "UI/MsgLogHandler.hpp"

namespace Peripheral {

#define SOIL_SENSORS 4

struct soilFlags {
    bool msgLogErr;
    bool handle;
    bool channels;
};

class Soil {
    private:
    soilFlags flags;
    int readings[SOIL_SENSORS];
    Threads::Mutex mtx;
    Messaging::MsgLogHandler* msglogerr;
    BOUNDARY_CONFIG settings[SOIL_SENSORS];
    Soil(Messaging::MsgLogHandler* msglogerr); 
    Soil(const Soil&) = delete; // prevent copying
    Soil &operator=(const Soil&) = delete; // prevent assignment
    adc_oneshot_unit_handle_t handle;
    adc_channel_t* channels;

    public:
    // set to nullptr to reduce arguments when calling after init.
    static Soil* getInstance(Messaging::MsgLogHandler* msglogerr = nullptr);
    void setHandle(adc_oneshot_unit_handle_t handle);
    void setChannels(adc_channel_t* channels);
    BOUNDARY_CONFIG* getConfig(uint8_t indexNum);
    void readAll();
    void getAll(int* readings, size_t size);
    void checkBounds();
    void handleRelay();
    void handleAlert();
    bool isInit();
};

}

#endif // SOIL_HPP