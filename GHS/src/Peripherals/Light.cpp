#include "Peripherals/Light.hpp"
#include "Drivers/AS7341/AS7341_Library.hpp"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_continuous.h"
#include "Peripherals/Relay.hpp"
#include "Threads/Mutex.hpp"
#include "UI/MsgLogHandler.hpp"
#include "Common/Timing.hpp"
#include "string.h" 
#include "Common/FlagReg.hpp"

namespace Peripheral {

const char* Light::tag("(LIGHT)");
char Light::log[LOG_MAX_ENTRY]{0};
Threads::Mutex Light::mtx; // Def of static var

Light::Light(LightParams &params) : 
    
    flags("(Lightflag)"), readings{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    averages{
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0, 0, 0, 0},

    conf{0, LIGHT_THRESHOLD_DEF, RECOND::NONE, RECOND::NONE, nullptr, 
        LIGHT_NO_RELAY, 0, 0, 0},

    lightDuration(0), photoVal(0), params(params) {

        snprintf(Light::log, sizeof(Light::log), "%s Ob created", Light::tag);
        Light::sendErr(Light::log, Messaging::Levels::INFO);
    }

// Requires isSpectral bool. Computes either the spectral or photoresistors
// averages and saves them into averages variable.
void Light::computeAverages(bool isSpec) {

    // Check if photoresistor
    if (!isSpec) {
        this->averages.pollCtPho++; // Increment pollCt (never 0)

        // compute delta between current reading and averages and add that
        // difference, divided by the new poll count, to the averages.
        float delta = this->photoVal - this->averages.photoResistor;
        this->averages.photoResistor += (delta / this->averages.pollCtPho); 
        return; // Block spectral below.
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
    // continuing. Once counts are met of consecutive criteria, passes.
    if (ct < LIGHT_CONSECUTIVE_CTS) return;

    // Start on toggle at true. If it is dark when the device starts, light 
    // duration will remain at 0, and will only begin counting once light
    // is present, which will then change the toggle to false allow duration
    // to be computed. Of toggle is used to log dark start only.
    static bool onToggle = true, offToggle = true;
    static uint32_t lightStart = 0;
    Clock::TIME* dt = Clock::DateTime::get()->getTime();

    switch (isLight) {
        case true: // If light
        offToggle = true; // reset toggle for next dark period
        if (onToggle) { // If true, captures start time and sets toggle to false.
            onToggle = false;
            this->lightDuration = 0; // Reset at light start
            lightStart = Clock::DateTime::get()->seconds();
            snprintf(Light::log, sizeof(Light::log), 
                "%s Light start at time %u:%u:%u", this->tag, dt->hour, 
                dt->minute, dt->second);

            Light::sendErr(Light::log, Messaging::Levels::INFO);

        // Once toggle is set to false, the light duration can be captured.
        } else if (!onToggle) {

            this->lightDuration = 
                Clock::DateTime::get()->seconds() - lightStart;
        }

        break;

        case false: // If dark
        onToggle = true; // reset toggle for next light period
        if (offToggle) { // If true, captures and logs the dark time.
            offToggle = false;

            snprintf(Light::log, sizeof(Light::log), 
                "%s Light stoped at time %u:%u:%u", Light::tag, dt->hour, 
                dt->minute, dt->second);

            Light::sendErr(Light::log, Messaging::Levels::INFO);
        }

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
        if (this->conf.relay == nullptr || 
            !this->flags.getFlag(LIGHTFLAGS::PHOTO_NO_ERR) || 
            ct < LIGHT_CONSECUTIVE_CTS || 
            this->conf.condition == RECOND::NONE) {
            return; // Return if gate parameters are not met. Block
        }

        this->conf.relay->on(this->conf.controlID); // Logging capture in func
        break;

        case false: // Relay is set to turn off.
        if (!this->flags.getFlag(LIGHTFLAGS::PHOTO_NO_ERR) || 
            ct < LIGHT_CONSECUTIVE_CTS) {
            return; // Return if gate parameers are not met.
        }

        this->conf.relay->off(this->conf.controlID);
        break;
    }
}

// Requires message string, and level. Level is default to ERROR. Prints to
// both serial and log.
void Light::sendErr(const char* msg, Messaging::Levels lvl) {
    Messaging::MsgLogHandler::get()->handle(lvl, msg, 
        Messaging::Method::SRL_LOG);
}

// Singleton class object, require light parameters for first init. Once init
// will trun a point to the class instance for use. WARNING! Will not function
// without proper init, since nullptr will not be able to command.
Light* Light::get(LightParams* parameter) {

    // Single use of mutex lock which will ensure to protect any subsequent
    // calls made after requesting this instance.
    Threads::MutexLock(Light::mtx);

    static bool isInit{false};

    // Gate. If nullptr is passed, and hasnt been init, returns nullptr.
    if (parameter == nullptr && !isInit) {
        snprintf(Light::log, sizeof(Light::log), "%s Using uninit instance",
            Light::tag);
 
        Light::sendErr(Light::log, Messaging::Levels::CRITICAL);
        return nullptr; 

    } else if (parameter != nullptr) { 
        isInit = true;
    }

    static Light instance(*parameter); // Create param.
    
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

    if (read) { // Show no error flags if true
        this->flags.setFlag(LIGHTFLAGS::SPEC_NO_ERR_DISP);
        this->flags.setFlag(LIGHTFLAGS::SPEC_NO_ERR);
        this->computeAverages(true); // Compute avg upon success.
        errCt = 0; // resets count.
    } else { // If error, release flag to false.
        this->flags.releaseFlag(LIGHTFLAGS::SPEC_NO_ERR);
        errCt++; // inc count by 1.
    }

    // Sets the diplay to true if the error count is less than max allowed.
    // If exceeded the display will be set to false telling client of error.
    // This is to filter bad reads from constantly alerting client.
    if (errCt < LIGHT_ERR_CT_MAX) {
        this->flags.setFlag(LIGHTFLAGS::SPEC_NO_ERR_DISP);
    } else {
        this->flags.releaseFlag(LIGHTFLAGS::SPEC_NO_ERR_DISP);
    }

    return this->flags.getFlag(LIGHTFLAGS::SPEC_NO_ERR); // true if data OK
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
        this->computeAverages(false); // compute avg upon success.
        this->flags.setFlag(LIGHTFLAGS::PHOTO_NO_ERR_DISP);
        this->flags.setFlag(LIGHTFLAGS::PHOTO_NO_ERR);
        errCt = 0; // zero out
    } else {
        this->flags.releaseFlag(LIGHTFLAGS::PHOTO_NO_ERR); // false if err
        errCt++; // Increment error count.
    }

    // Sets display to true if the error ct is less than max.
    if (errCt < LIGHT_ERR_CT_MAX) {
        this->flags.setFlag(LIGHTFLAGS::PHOTO_NO_ERR_DISP);
    } else {
        this->flags.releaseFlag(LIGHTFLAGS::PHOTO_NO_ERR_DISP);
    }

    return this->flags.getFlag(LIGHTFLAGS::PHOTO_NO_ERR); // true if OK
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
Flag::FlagReg* Light::getFlags() {
    return &this->flags;
}

// Requires no parameters. Checks the boundaries of the photoresistor settings.
// If the read values are outside of the boundaries, the relay is handled
// appropriately. Returns true if successful, and false if there is an error
// reading the photoresistor.
bool Light::checkBounds() { // Acts as a gate to ensure data integ.
    if (!this->flags.getFlag(LIGHTFLAGS::PHOTO_NO_ERR)) return false;

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

    snprintf(this->log, sizeof(this->log), "%s Avg clear", this->tag);
    this->sendErr(this->log, Messaging::Levels::INFO);
}

// Returns duration of light above the trip value.
uint32_t Light::getDuration() {return this->lightDuration;}

}