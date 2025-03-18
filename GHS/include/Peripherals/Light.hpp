#ifndef LIGHT_HPP
#define LIGHT_HPP

#include "Drivers/AS7341/AS7341_Library.hpp"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_continuous.h"
#include "Peripherals/Relay.hpp"
#include "Threads/Mutex.hpp"
#include "UI/MsgLogHandler.hpp"
#include "Common/FlagReg.hpp"

namespace Peripheral {

// Threshold that marks day start and end using spectral clear channel.
#define LIGHT_THRESHOLD_DEF 500 // Between 0 and 4095. Sets initial Dark bound.
#define LIGHT_CONSECUTIVE_CTS 5 // Action isnt taken until count is met.
#define LIGHT_HYSTERESIS 10 // Padding for photoresistor.
#define LIGHT_ERR_CT_MAX 3 // Error counts to show error on display
#define PHOTO_MIN 1 // Really 0, set to 1 for error purposes.
#define PHOTO_MAX 4094 // 12 bit max - 1, set for error purposes.
#define LIGHT_NO_RELAY 255 // Used to show no relay attached.
#define LIGHT_LOG_METHOD Messaging::Method::SRL_LOG

struct LightParams {
    adc_oneshot_unit_handle_t handle; // analog-digital conv handle
    adc_channel_t channel; // Channel for photoresistor
    AS7341_DRVR::AS7341basic &as7341; // AS7341 driver reference.
};

// ALERTS not used for light, relay ONLY.

// Used for photo resistor
struct RelayConfigLight {
    uint16_t tripVal; // Photoresistor trip value.
    uint16_t darkVal; // Used to compute light duration. Default set.
    RECOND condition; // Relay condition
    RECOND prevCondition; // Previous relay condition.
    Relay* relay; // Relay attached to sensor.
    uint8_t num; // relay index, relay 1 is 0.
    uint8_t controlID; // ID given by relay to allow this device to control it.
    size_t onCt; // Consecutive on counts to turn relay on.
    size_t offCt; // Consecutive off counts to turn relay off.
};

enum LIGHTFLAGS : uint8_t {
    PHOTO_NO_ERR_DISP, // Photoresistor no error displayed to client.
    PHOTO_NO_ERR, // Photoresistor no err, read ok.
    SPEC_NO_ERR_DISP, // Spectral no error displayed to client.
    SPEC_NO_ERR // Spectral no error, read ok.
};

// Taken from the same structure as the AS7341 driver, in float and simplified
// form.
struct Color_Averages {
    float clear, violet, indigo, blue, cyan, green,
    yellow, orange, red, nir;
};

struct Light_Averages {
    Color_Averages color; // Current color count averages.
    Color_Averages prevColor; // Previous color count averages.
    float photoResistor; // Current photoresistor average.
    float prevPhotoResistor; // Previous photoresistor average.
    size_t pollCtClr; // Poll counts for color/AS7341.
    size_t pollCtPho; // Poll counts for photoresistor.
};

class Light {
    private:
    static const char* tag; // Static req to use in get().
    static char log[LOG_MAX_ENTRY]; // Static req to use in get().
    Flag::FlagReg flags;
    AS7341_DRVR::COLOR readings;
    Light_Averages averages; 
    RelayConfigLight conf;
    uint32_t lightDuration;
    int photoVal;
    static Threads::Mutex mtx;
    LightParams &params;
    Light(LightParams &params); 
    Light(const Light&) = delete; // prevent copying
    Light &operator=(const Light&) = delete; // prevent assignment
    void computeAverages(bool isSpec);
    void computeLightTime(size_t ct, bool isLight);
    void handleRelay(bool relayOn, size_t ct);
    static void sendErr(const char* msg, Messaging::Levels lvl = 
            Messaging::Levels::ERROR);

    public:
    static Light* get(LightParams* parameter = nullptr);
    bool readSpectrum(); // read as7341
    bool readPhoto(); // read photoresistor
    AS7341_DRVR::COLOR* getSpectrum();
    int getPhoto();
    Flag::FlagReg* getFlags();
    bool checkBounds();
    RelayConfigLight* getConf();
    Light_Averages* getAverages();
    void clearAverages();
    uint32_t getDuration();
};

}

#endif // LIGHT_HPP