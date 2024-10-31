#include "Peripherals/Light.hpp"
#include "Drivers/AS7341/AS7341_Library.hpp"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_continuous.h"
#include "Peripherals/Relay.hpp"
#include "Threads/Mutex.hpp"
#include "UI/MsgLogHandler.hpp"

namespace Peripheral {

Light::Light(LightParams &params) : 

    readings{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    photoVal(0), isPhotoUp(false), isSpecUp(false),
    mtx(params.msglogerr), params(params) {}

Light* Light::get(LightParams* parameter) {
    static bool isInit{false};

    if (parameter == nullptr && !isInit) {
        return nullptr; // Blocks instance from being created.
    } else if (parameter != nullptr) {
        isInit = true; // Opens gate after proper init
    }

    static Light instance(*parameter);
    
    return &instance;
}

void Light::readSpectrum() {

}

bool Light::readPhoto() {
    esp_err_t err{ESP_FAIL};
    static size_t errCt{0};
    const size_t errCtMax{5};

    this->mtx.lock();
    err = adc_oneshot_read(
        this->params.handle,
        this->params.channel,
        &this->photoVal
    );
    this->mtx.unlock();

    if (err == ESP_OK) {
        errCt = 0;
        this->isPhotoUp = true;
    } else {
        errCt++;
    }

    if (errCt >= errCtMax) {
        this->isPhotoUp = false;
        errCt = 0;
    }

    return this->isPhotoUp;
    
}

void Light::handleRelay() {

}

void Light::handleAlert() {

}


}