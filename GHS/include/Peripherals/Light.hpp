#ifndef LIGHT_HPP
#define LIGHT_HPP

#include "Drivers/AS7341/AS7341_Library.hpp"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_continuous.h"
#include "Peripherals/Relay.hpp"
#include "Threads/Mutex.hpp"

namespace Peripheral {

// Threshold that marks day start and end using spectral clear channel.
#define LIGHT_THRESHOLD_DEF 500 // Between 0 and 4095
#define LIGHT_CONSECUTIVE_CTS 5 // Action isnt taken until count is met.
#define LIGHT_HYSTERESIS 10 // Padding for photoresistor.
#define PHOTO_MIN 1 // Really 0, set to 1 for error purposes.
#define PHOTO_MAX 4094 // 12 bit max - 1, set for error purposes.
#define LIGHT_NO_RELAY 99 // Used to show no relay attached.

struct LightParams {
    adc_oneshot_unit_handle_t handle;
    adc_channel_t channel;
    AS7341_DRVR::AS7341basic &as7341;
};

// ALERTS not used for light.

// Used for photo resistor
struct RelayConfigLight {
    uint16_t tripVal; // Photoresistor trip value, value is dark/light boundary.
    RECOND condition; // Relay condition
    RECOND prevCondition; // Previous relay condition.
    Relay* relay; // Relay attached to sensor.
    uint8_t num; // relay index, relay 1 is 0.
    uint8_t controlID; // ID given by relay to allow this device to control it.
    size_t onCt; // Consecutive on counts to turn relay on.
    size_t offCt; // Consecutive off counts to turn relay off.
};

struct isUpLight { // 4 bytes, return as value.
    bool photoNoDispErr;
    bool photoNoErr;
    bool specNoDispErr;
    bool specNoErr;
};

// Taken from the same structure as the AS7341 driver, in float and simplified
// form.
struct Color_Averages {
    float clear, violet, indigo, blue, cyan, green,
    yellow, orange, red, nir;
};

struct Light_Averages {
    Color_Averages color;
    Color_Averages prevColor;
    float photoResistor;
    float prevPhotoResistor;
    size_t pollCtClr;
    size_t pollCtPho;
};

class Light {
    private:
    AS7341_DRVR::COLOR readings;
    Light_Averages averages; // Add a clear averages just like the temphum
    RelayConfigLight conf;
    uint32_t lightDuration;
    int photoVal;
    isUpLight flags;
    static Threads::Mutex mtx;
    LightParams &params;
    Light(LightParams &params); 
    Light(const Light&) = delete; // prevent copying
    Light &operator=(const Light&) = delete; // prevent assignment
    void computeAverages(bool isSpec);
    void computeLightTime(size_t ct, bool isLight);
    void handleRelay(bool relayOn, size_t ct);

    public:
    static Light* get(LightParams* parameter = nullptr);
    bool readSpectrum(); // read as7341
    bool readPhoto(); // read photoresistor
    AS7341_DRVR::COLOR* getSpectrum();
    int getPhoto();
    isUpLight getStatus();
    bool checkBounds();
    RelayConfigLight* getConf();
    Light_Averages* getAverages();
    void clearAverages();
    uint32_t getDuration();
};

}

#endif // LIGHT_HPP