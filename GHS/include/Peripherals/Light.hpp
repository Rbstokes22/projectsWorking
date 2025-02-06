#ifndef LIGHT_HPP
#define LIGHT_HPP

#include "Drivers/AS7341/AS7341_Library.hpp"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_continuous.h"
#include "Peripherals/Relay.hpp"
#include "Threads/Mutex.hpp"

namespace Peripheral {

struct LightParams {
    adc_oneshot_unit_handle_t handle;
    adc_channel_t channel;
    AS7341_DRVR::AS7341basic &as7341;
};

struct isUpLight {
    bool photoDisplay;
    bool photoImmediate;
    bool specDisplay;
    bool specImmediate;
};

class Light {
    private:
    AS7341_DRVR::COLOR readings;
    int photoVal;
    isUpLight flags;
    Threads::Mutex mtx;
    LightParams &params;
    Light(LightParams &params); 
    Light(const Light&) = delete; // prevent copying
    Light &operator=(const Light&) = delete; // prevent assignment

    public:
    static Light* get(LightParams* parameter = nullptr);
    bool readSpectrum(); // read as7341
    bool readPhoto(); // read photoresistor
    void getSpectrum(AS7341_DRVR::COLOR &readings);
    void getPhoto(int &photVal);
    void handleRelay();
    void handleAlert();
    isUpLight getStatus();
};

}

#endif // LIGHT_HPP