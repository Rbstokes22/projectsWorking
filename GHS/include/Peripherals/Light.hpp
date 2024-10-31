#ifndef LIGHT_HPP
#define LIGHT_HPP

#include "Drivers/AS7341/AS7341_Library.hpp"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_continuous.h"
#include "Peripherals/Relay.hpp"
#include "Threads/Mutex.hpp"
#include "UI/MsgLogHandler.hpp"

namespace Peripheral {

struct LightParams {
    Messaging::MsgLogHandler &msglogerr;
    adc_oneshot_unit_handle_t handle;
    adc_channel_t channel;
    AS7341_DRVR::AS7341basic &as7341;
};

class Light {
    private:
    AS7341_DRVR::COLOR readings;
    int photoVal;
    bool isPhotoUp;
    bool isSpecUp;
    Threads::Mutex mtx;
    LightParams &params;
    Light(LightParams &params); 
    Light(const Light&) = delete; // prevent copying
    Light &operator=(const Light&) = delete; // prevent assignment

    public:
    static Light* get(LightParams* parameter = nullptr);
    void readSpectrum(); // Send readings
    bool readPhoto(); // read photoresistor
    void handleRelay();
    void handleAlert();
};

}

#endif // LIGHT_HPP