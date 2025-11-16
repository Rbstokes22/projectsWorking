#ifndef LIGHT_HPP
#define LIGHT_HPP

#include "Drivers/AS7341/AS7341_Library.hpp"
#include "Peripherals/Relay.hpp"
#include "Threads/Mutex.hpp"
#include "UI/MsgLogHandler.hpp"
#include "Common/FlagReg.hpp"
#include "Config/config.hpp"
#include "Drivers/ADC.hpp"

namespace Peripheral {

// Threshold that marks day start and end using spectral clear channel.
#define LIGHT_THRESHOLD_DEF 500 // Between 0 and 4095. Sets initial Dark bound.
#define LIGHT_CONSECUTIVE_CTS 5 // Action isnt taken until count is met.
#define LIGHT_HYSTERESIS 10 // Padding for photoresistor.
#define PHOTO_MIN 1 // Really 0, set to 1 for error purposes.
#define PHOTO_MAX 32766 // int16 max - 1, set for error purposes.
#define PHOTO_NOISE 5 // Used to filter noise from the analog read, must be > 0.
#define LIGHT_NO_RELAY 255 // Used to show no relay attached.
#define LIGHT_LOG_METHOD Messaging::Method::SRL_LOG
#define LIGHT_TAG "(LIGHT)"

struct LightParams {
    ADC_DRVR::ADC &photo;
    AS7341_DRVR::AS7341basic &as7341; // AS7341 driver reference.
};

// ALERTS not used for light, relay ONLY. Alert is used for sensor down though.

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

// Taken from the same structure as the AS7341 driver, in float and simplified
// form.
struct Color_Averages {
    float clear, violet, indigo, blue, cyan, green,
    yellow, orange, red, nir;
};

struct Light_Trends { // Prevous n hours of light values, on the hour.
    uint16_t clear[TREND_HOURS];
    uint16_t violet[TREND_HOURS];
    uint16_t indigo[TREND_HOURS];
    uint16_t blue[TREND_HOURS];
    uint16_t cyan[TREND_HOURS];
    uint16_t green[TREND_HOURS];
    uint16_t yellow[TREND_HOURS];
    uint16_t orange[TREND_HOURS];
    uint16_t red[TREND_HOURS];
    uint16_t nir[TREND_HOURS];
    int16_t photo[TREND_HOURS];
};

struct Light_Averages {
    Color_Averages color; // Current color count averages.
    Color_Averages prevColor; // Previous color count averages.
    float photoResistor; // Current photoresistor average.
    float prevPhotoResistor; // Previous photoresistor average.
    size_t pollCtClr; // Poll counts for color/AS7341.
    size_t pollCtPho; // Poll counts for photoresistor.
};

// This isn't necessary and a redundancy. This is populated upon creation of 
// the object by getting the raw values from the AS7341 driver. In order to 
// prevent a constant pinging, this struct holds the necessary values, and the
// will update upon a successful driver device update. 
struct Spec_Conf {
    uint8_t ATIME; 
    uint16_t ASTEP;
    AS7341_DRVR::AGAIN AGAIN;
};

struct LightHealth {
    float photo;
    float spec;
    bool photoReadErr;
    bool specReadErr;
    LightHealth();
};

class Light {
    private:
    static const char* tag; // Static req to use in get().
    static char log[LOG_MAX_ENTRY]; // Static req to use in get().
    LightHealth health;
    AS7341_DRVR::COLOR readings;
    Spec_Conf specConf;
    Light_Trends trends;
    Light_Averages averages; 
    RelayConfigLight conf;
    uint32_t lightDuration;
    int16_t photoVal;
    static Threads::Mutex mtx;
    LightParams &params;
    Light(LightParams &params); 
    Light(const Light&) = delete; // prevent copying
    Light &operator=(const Light&) = delete; // prevent assignment
    void computeAverages(bool isSpec);
    void computeTrends();
    void computeLightTime(size_t ct, bool isLight);
    void handleRelay(bool relayOn, size_t ct);
    static void sendErr(const char* msg, Messaging::Levels lvl = 
            Messaging::Levels::ERROR);
    void median5(int16_t &val);

    public:
    static Light* get(LightParams* parameter = nullptr);
    bool readSpectrum(); // read as7341 spectral.
    bool readPhoto(); // read photoresistor.
    AS7341_DRVR::COLOR* getSpectrum();
    int getPhoto();
    LightHealth* getHealth();
    bool checkBounds();
    RelayConfigLight* getConf();
    Spec_Conf* getSpecConf();
    Light_Averages* getAverages();
    Light_Trends* getTrends();
    void clearAverages();
    uint32_t getDuration();
    bool setATIME(uint8_t val);
    bool setASTEP(uint16_t val);
    bool setAGAIN(AS7341_DRVR::AGAIN val);
};

}

#endif // LIGHT_HPP