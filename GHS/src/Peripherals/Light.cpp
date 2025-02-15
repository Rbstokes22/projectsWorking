#include "Peripherals/Light.hpp"
#include "Drivers/AS7341/AS7341_Library.hpp"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_continuous.h"
#include "Peripherals/Relay.hpp"
#include "Threads/Mutex.hpp"
#include "UI/MsgLogHandler.hpp"
#include "Common/Timing.hpp"

namespace Peripheral {

Threads::Mutex Light::mtx; // Def of static var

Light::Light(LightParams &params) : 

    readings{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    averages{
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0, 0, 0, 0},

    conf{LIGHT_THRESHOLD_DEF, RECOND::NONE, RECOND::NONE, nullptr, 0, 0, 0, 0},
    lightDuration(0), photoVal(0), flags{false, false, false, false},
    params(params) {}

void Light::computeAverages(bool isSpec) {

    // Check if photoresistor
    if (!isSpec) {
        this->averages.pollCtPho++;

        float delta = this->photoVal - this->averages.photoResistor;
        this->averages.photoResistor += (delta / this->averages.pollCtPho); 
        return;
    }

    uint16_t readings[] = { // Deltas of current values and average values.
        this->readings.Clear, this->readings.F1_415nm_Violet,
        this->readings.F2_445nm_Indigo, this->readings.F3_480nm_Blue,
        this->readings.F4_515nm_Cyan, this->readings.F5_555nm_Green,
        this->readings.F6_590nm_Yellow, this->readings.F7_630nm_Orange,
        this->readings.F8_680nm_Red, this->readings.NIR
    };

    float* averages[] = { // needs pass by reference for mod.
        &this->averages.color.clear, &this->averages.color.violet,
        &this->averages.color.indigo, &this->averages.color.blue,
        &this->averages.color.cyan, &this->averages.color.green,
        &this->averages.color.yellow, &this->averages.color.orange,
        &this->averages.color.red, &this->averages.color.nir
    };

    this->averages.pollCtClr++; // Increment poll count by 1.

    for (int i = 0; i < (sizeof(readings) / sizeof(readings[0])); i++) {
        float delta = readings[i] - *averages[i];

        *averages[i] += (delta / this->averages.pollCtClr);
    }
}

// Requires the quantity of consecutive counts, and if that count is associated
// with being light or dark. Once the threshold is met, the duration of light
// is captured.
void Light::computeLightTime(size_t ct, bool isLight) { 

    // Main gate, prevents any count that is not at the threshold from
    // continuing.
    if (ct < LIGHT_CONSECUTIVE_CTS) return;

    static bool toggle = true;
    static uint32_t lightStart = 0;

    switch (isLight) {
        case true:
        if (toggle) { // If true, captures start time and sets toggle to false.
            toggle = false;
            this->lightDuration = 0; // Reset at light start
            lightStart = Clock::DateTime::get()->seconds();

        // Once toggle is set to false, the light duration can be captured.
        } else if (!toggle) {
            this->lightDuration = 
                Clock::DateTime::get()->seconds() - lightStart;
        }

        break;

        case false:
        toggle = true;
        break;
    }
}

void Light::handleRelay(bool relayOn, size_t ct) {

    switch (relayOn) {
        case true:
        if (this->conf.relay == nullptr || !this->flags.photoNoErr || 
        ct < LIGHT_CONSECUTIVE_CTS || this->conf.condition == RECOND::NONE) {
            return;
        }

        this->conf.relay->on(this->conf.controlID);
        break;

        case false:
        if (!this->flags.photoNoErr || ct < LIGHT_CONSECUTIVE_CTS) {
            return;
        }

        this->conf.relay->off(this->conf.controlID);
        break;
    }
}

Light* Light::get(LightParams* parameter) {

    // Single use of mutex lock which will ensure to protect any subsequent
    // calls made after requesting this instance.
    Threads::MutexLock(Light::mtx);

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

    read = this->params.as7341.readAll(this->readings);

    if (read) {
        this->flags.specNoDispErr = true;
        this->flags.specNoErr = true;
        this->computeAverages(true);
        errCt = 0;
    } else {
        this->flags.specNoErr = false;
        errCt++;
    }

    if (errCt >= errCtMax) {
        this->flags.specNoDispErr = false;
        errCt = 0;
    }

    return this->flags.specNoErr;
}

bool Light::readPhoto() {
    esp_err_t err;
    static size_t errCt{0};
    const size_t errCtMax{5};

    err = adc_oneshot_read(this->params.handle, this->params.channel,
        &this->photoVal);

    if (err == ESP_OK) {
        errCt = 0;
        this->computeAverages(false);
        this->flags.photoNoDispErr = true;
        this->flags.photoNoErr = true;

    } else {
        this->flags.photoNoErr = false;
        errCt++;
    }

    if (errCt >= errCtMax) {
        this->flags.photoNoDispErr = false;
        errCt = 0;
    }

    return this->flags.photoNoErr;
}

AS7341_DRVR::COLOR* Light::getSpectrum() {
    return &this->readings;
}

int Light::getPhoto() {
    return this->photoVal; 
}

isUpLight Light::getStatus() {
    return this->flags;
}

bool Light::checkBounds() { // Acts as a gate to ensure data integ.
    if (!this->flags.photoNoErr) return false;

    int lowerBound = this->conf.tripVal - LIGHT_HYSTERESIS;
    int upperBound = this->conf.tripVal + LIGHT_HYSTERESIS;

    // Checks to see if the relay conditions have changed. If true,
    // resets the counts and changes the previous condition to current
    // condition.
    if (this->conf.condition != this->conf.prevCondition) {
        this->conf.onCt = 0; this->conf.offCt = 0;
        this->conf.prevCondition = this->conf.condition;
    }

    // On counts incremement when the photo resistor value is higher than the
    // trip value. Off counts increment when the value is lower. This is 
    // separate from the configuration conditions.
    size_t isLightOnCt = 0;
    size_t isLightOffCt = 0;

    if (this->photoVal >= this->conf.tripVal) {
        isLightOnCt++;
        isLightOffCt = 0; // Reset if true
        this->computeLightTime(isLightOnCt, true);
    } else {
        isLightOnCt = 0; // Reset if false
        isLightOffCt++; 
        this->computeLightTime(isLightOffCt, false);
    }

    // Once the criteria has been met, the counts are incremented for each
    // consecutive count only. These counts are passed to the relay handler
    // and once conditions are met, will energize or de-energize the relays.
    switch (this->conf.condition) {
        case RECOND::LESS_THAN:
        if (this->photoVal < this->conf.tripVal) {
            this->conf.onCt++; 
            this->conf.offCt = 0;
            this->handleRelay(true, this->conf.onCt);

        } else if (this->photoVal >= upperBound) {
            this->conf.onCt = 0;
            this->conf.offCt++;
            this->handleRelay(false, this->conf.offCt);
        }        
        break;

        case RECOND::GTR_THAN:
        if (this->photoVal > this->conf.tripVal) {
            this->conf.onCt++;
            this->conf.offCt = 0;
            this->handleRelay(true, conf.onCt);

        } else if (this->photoVal <= lowerBound) {
            this->conf.onCt = 0;
            this->conf.offCt++;
            this->handleRelay(false, this->conf.offCt);
        }
        break;

        default: // Empty but required using enum class
        break;
    }

    return true;
}

RelayConfigLight* Light::getConf() {return &this->conf;}

Light_Averages* Light::getAverages() {return &this->averages;}

void Light::clearAverages() {
    this->averages.prevColor = this->averages.color;
    this->averages.prevPhotoResistor = this->averages.photoResistor;

    this->averages = { // Reset
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0, 0, 0, 0};
}

uint32_t Light::getDuration() {return this->lightDuration;}

}