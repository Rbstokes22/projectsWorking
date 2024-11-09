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
    photoVal(0), flags{false, false, false, false},
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

bool Light::readSpectrum() {
    static size_t errCt{0};
    const size_t errCtMax{5};
    bool read{false};

    this->mtx.lock();
    try {
        read = this->params.as7341.readAll(this->readings);
    } catch(...) {
        this->mtx.unlock();
        throw; // re-throw error.
    }
    
    this->mtx.unlock();

    if (read) {
        this->flags.specDisplay = true;
        this->flags.specImmediate = true;
        errCt = 0;
    } else {
        this->flags.specImmediate = false;
        errCt++;
    }

    if (errCt >= errCtMax) {
        this->flags.specDisplay = false;
        errCt = 0;
    }

    return this->flags.specImmediate;
}

bool Light::readPhoto() {
    esp_err_t err;
    static size_t errCt{0};
    const size_t errCtMax{5};

    this->mtx.lock();
    try {
        err = adc_oneshot_read(
            this->params.handle,
            this->params.channel,
            &this->photoVal
        );
    } catch(...) {
        this->mtx.unlock();
        throw; // re-throw error.
    }
    
    this->mtx.unlock();

    if (err == ESP_OK) {
        errCt = 0;
        this->flags.photoDisplay = true;
        this->flags.photoImmediate = true;

    } else {
        this->flags.photoImmediate = false;
        errCt++;
    }

    if (errCt >= errCtMax) {
        this->flags.photoDisplay = false;
        errCt = 0;
    }

    return this->flags.photoImmediate;
}

void Light::getSpectrum(AS7341_DRVR::COLOR &readings) {
    this->mtx.lock();
    try {
        readings = this->readings; // copy current readings.
    } catch(...) {
        this->mtx.unlock();
        throw; // re-throw exception
    }
    
    this->mtx.unlock();
}

void Light::getPhoto(int &photoVal) {
    this->mtx.lock();
    try {
        photoVal = this->photoVal; // copy current readings.
    } catch(...) {
        this->mtx.unlock();
        throw; // rethrow exception
    }

    this->mtx.unlock();
}

void Light::handleRelay() {

}

void Light::handleAlert() {

}

isUpLight Light::getStatus() {
    return this->flags;
}


}