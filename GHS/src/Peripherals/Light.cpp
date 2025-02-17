#include "Peripherals/Light.hpp"
#include "Drivers/AS7341/AS7341_Library.hpp"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_continuous.h"
#include "Peripherals/Relay.hpp"
#include "Threads/Mutex.hpp"
#include "UI/MsgLogHandler.hpp"
#include "Common/Timing.hpp"
#include "string.h" 

namespace Peripheral {

Threads::Mutex Light::mtx; // Def of static var

Light::Light(LightParams &params) : 

    readings{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    averages{
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0, 0, 0, 0},

    conf{0, LIGHT_THRESHOLD_DEF, RECOND::NONE, RECOND::NONE, nullptr, 
        LIGHT_NO_RELAY, 0, 0, 0},

    lightDuration(0), photoVal(0), flags{false, false, false, false},
    params(params) {}

// Requires isSpectral bool. Computes either the spectral or photoresistors
// averages.
void Light::computeAverages(bool isSpec) {

    // Check if photoresistor
    if (!isSpec) {
        this->averages.pollCtPho++; // Increment pollCt (never 0)

        // compute delta between current reading and averages and add that
        // difference, divided by the new poll count, to the averages.
        float delta = this->photoVal - this->averages.photoResistor;
        this->averages.photoResistor += (delta / this->averages.pollCtPho); 
        return;
    }

    // To efficiently process everything, created two arrays that can be
    // iterated together, have their delta computed like above, and the 
    // difference added to the average reading.

    uint16_t readings[] = { // Deltas of current values and average values.
        this->readings.Clear, this->readings.F1_415nm_Violet,
        this->readings.F2_445nm_Indigo, this->readings.F3_480nm_Blue,
        this->readings.F4_515nm_Cyan, this->readings.F5_555nm_Green,
        this->readings.F6_590nm_Yellow, this->readings.F7_630nm_Orange,
        this->readings.F8_680nm_Red, this->readings.NIR
    };

    float* averages[] = { // needs pass by reference for mod capability.
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
// is captured and stored as a class variable.
void Light::computeLightTime(size_t ct, bool isLight) { 
    
    // Main gate, prevents any count that is not at the threshold from
    // continuing.
    if (ct < LIGHT_CONSECUTIVE_CTS) return;

    // Start toggle at true. If it is dark when the device starts, light 
    // duration will remain at 0, and will only begin counting once light
    // is present, which will then change the toggle to false allow duration
    // to be computed.
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

        case false: // If dark
        toggle = true; // reset toggle for next light period
        break;
    }
}

// Requires relayOn or off boolean, and consecutive counts. If 5 consecutive
// counts are required, and the light trip value is 500, The value must be
// exceeded for at least 5 consecutive readings. The relay will either 
// energize or de-energize depending on the settings.
void Light::handleRelay(bool relayOn, size_t ct) {

    switch (relayOn) {
        case true: // Relay is set to turn on.
        if (this->conf.relay == nullptr || !this->flags.photoNoErr || 
        ct < LIGHT_CONSECUTIVE_CTS || this->conf.condition == RECOND::NONE) {
            return; // Return if gate parameters are not met.
        }

        this->conf.relay->on(this->conf.controlID);
        break;

        case false: // Relay is set to turn off.
        if (!this->flags.photoNoErr || ct < LIGHT_CONSECUTIVE_CTS) {
            return; // Return if gate parameers are not met.
        }

        this->conf.relay->off(this->conf.controlID);
        break;
    }
}

// Singleton class object, require light parameters for first init. Once init
// will trun a point to the class instance for use.
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

// Requires no parameters. Uses the AS7341 driver to read all spectral channels.
// Returns true for a successful read, and false if not.
bool Light::readSpectrum() {
    static size_t errCt{0};
    bool read{false};

    // Upon success, updates the averages and changes the flags to true.
    // If not, will change the noErr flag to false indicating an immediate
    // error, which means the data is garbale. Upon a pre-set consecutive
    // error read, display flag is set to false allowing the clients display to
    // show the light reading to be down.
    read = this->params.as7341.readAll(this->readings);

    if (read) {
        this->flags.specNoDispErr = true;
        this->flags.specNoErr = true;
        this->computeAverages(true); // Compute avg upon success.
        errCt = 0; // resets count.
    } else {
        this->flags.specNoErr = false; // Indicates error.
        errCt++; // inc count by 1.
    }

    // Sets the diplay to true if the error count is less than max allowed.
    this->flags.specNoDispErr = (errCt < LIGHT_ERR_CT_MAX);

    return this->flags.specNoErr; // Returns true if data is OK.
}

// Requires no parameters. Reads the analog reading of the photoresistor and
// stores it to the class variable. Returns true upon a successful read, and
// false if not.
bool Light::readPhoto() {
    esp_err_t err;
    static size_t errCt{0};

    // Reads the analog value of the photoresistor and stores value to
    // class variable.
    err = adc_oneshot_read(this->params.handle, this->params.channel,
        &this->photoVal);

    // Upon sucess, changes the flags to true indicating no error.
    if (err == ESP_OK) {
        errCt = 0; // zero out
        this->computeAverages(false); // compute avg upon success.
        this->flags.photoNoDispErr = true;
        this->flags.photoNoErr = true;

    } else {
        this->flags.photoNoErr = false; // If false, set flag to false.
        errCt++; // Increment error count.
    }

    // Sets display to true if the error ct is less than max allowed.
    this->flags.photoNoDispErr = (errCt < LIGHT_ERR_CT_MAX);

    return this->flags.photoNoErr; // Returns true if OK.
}

// Returns pointer to spectrum data.
AS7341_DRVR::COLOR* Light::getSpectrum() {
    return &this->readings;
}

// Returns value of photoresistor analog read.
int Light::getPhoto() {
    return this->photoVal; 
}

// Returns the flags about error status.
isUpLight Light::getStatus() {
    return this->flags;
}

// Requires no parameters. Checks the boundaries of the photoresistor settings.
// If the read values are outside of the boundaries, the relay is handled
// appropriately. Returns true if successful, and false if there is an error
// reading the photoresistor.
bool Light::checkBounds() { // Acts as a gate to ensure data integ.
    if (!this->flags.photoNoErr) return false;

    // Bounds of the allowable range.
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
    // dark value. Off counts increment when the value is lower. This is 
    // separate from the configuration conditions.
    static size_t isLightOnCt = 0;
    static size_t isLightOffCt = 0;

    if (this->photoVal >= this->conf.darkVal) {
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

// returns a pointer to the light relay configuration for the photoresistor.
RelayConfigLight* Light::getConf() {return &this->conf;}

// Returns a pointer to the spectral, and photoresistor averages.
Light_Averages* Light::getAverages() {return &this->averages;}

// Requires no parameters. Clears the current data after moving current values
// over to previous values.
void Light::clearAverages() {
    this->averages.prevColor = this->averages.color; // copies over
    this->averages.prevPhotoResistor = this->averages.photoResistor;

    // Resets, the color and photoResistor only.
    memset(&this->averages, 0, sizeof(this->averages.color));
    this->averages.photoResistor = 0.0f;
    this->averages.pollCtClr = this->averages.pollCtPho = 0;
}

// Returns duration of light above the trip value.
uint32_t Light::getDuration() {return this->lightDuration;}

}